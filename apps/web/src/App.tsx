import { useCallback, useEffect, useRef, useState } from "react";
import { CanvasHost } from "./components/CanvasHost";
import { EngineClient } from "./engine/EngineClient";
import type { EngineDiagnostic, EngineMessage, ProjectRequest, ProjectSnapshot } from "./engine/protocol";
import { probeWebGpu, type WebGpuProbe } from "./platform/webgpuProbe";
import {
  chooseProjectWorkspace,
  digestFile,
  persistProject,
  persistVideoAsset,
  readText,
  type ProjectWorkspace,
} from "./storage/projectWorkspace";

const tools = ["VID", "IMG", "TXT", "BG", "SHP", "AUD", "SVG"];
const presets = [
  { id: "reels-9x16", name: "Reels", aspect: "9:16", resolutions: [["1080p", "1080p", 1080, 1920], ["2k", "2K", 1440, 2560], ["4k", "4K", 2160, 3840]] },
  { id: "portrait-4x5", name: "Portrait", aspect: "4:5", resolutions: [["1080p", "1080p", 1080, 1350], ["2k", "2K", 1440, 1800], ["4k", "4K", 2160, 2700]] },
  { id: "youtube-16x9", name: "YouTube", aspect: "16:9", resolutions: [["1080p", "1080p", 1920, 1080], ["2k", "2K", 2560, 1440], ["4k", "4K", 3840, 2160]] },
  { id: "cinematic-239x100", name: "Cinematic", aspect: "2.39:1", resolutions: [["1080p", "1080p", 1920, 804], ["2k", "2K", 2560, 1072], ["4k", "4K", 3840, 1608]] },
] as const;
const frameRates = [24, 25, 30, 50, 60, 90];
type ImportedVideo = {
  file: File;
  assetId: string;
  digest: string;
  mediaPath: string;
  width: number;
  height: number;
  duration: number;
  currentTime: number;
};

function App() {
  const [workspace, setWorkspace] = useState<"launcher" | "studio">("launcher");
  const [probe, setProbe] = useState<WebGpuProbe>({ state: "checking", adapter: "Checking browser capability…", limits: "—", diagnostic: "Requesting a browser-mediated high-performance adapter." });
  const [engineReady, setEngineReady] = useState(false);
  const [snapshot, setSnapshot] = useState<ProjectSnapshot | null>(null);
  const [diagnostics, setDiagnostics] = useState<EngineDiagnostic[]>([]);
  const [playing, setPlaying] = useState(false);
  const [busy, setBusy] = useState(false);
  const [workspaceRef, setWorkspaceRef] = useState<ProjectWorkspace | null>(null);
  const [video, setVideo] = useState<ImportedVideo | null>(null);
  const [projectName, setProjectName] = useState("My ReFusion Project");
  const [presetId, setPresetId] = useState("reels-9x16");
  const [resolutionId, setResolutionId] = useState("1080p");
  const [frameRate, setFrameRate] = useState(30);
  const [durationSeconds, setDurationSeconds] = useState(30);
  const [formError, setFormError] = useState("");
  const engineRef = useRef<EngineClient | null>(null);
  const videoInputRef = useRef<HTMLInputElement>(null);

  const publishDiagnostic = useCallback((diagnostic: EngineDiagnostic) => {
    setDiagnostics((items) => [diagnostic, ...items].slice(0, 5));
  }, []);

  useEffect(() => {
    let active = true;
    const engine = new EngineClient();
    engineRef.current = engine;
    void probeWebGpu().then((result) => active && setProbe(result));
    const unsubscribe = engine.subscribe((message: EngineMessage) => {
      if (message.kind === "engine_ready") setEngineReady(true);
      if (message.kind === "engine_snapshot") setSnapshot(message);
      if (message.kind === "diagnostic") publishDiagnostic(message.diagnostic);
    });
    engine.send({ type: "request_capabilities" });
    return () => {
      active = false;
      unsubscribe();
      engine.dispose();
      if (engineRef.current === engine) engineRef.current = null;
    };
  }, [publishDiagnostic]);

  const requestWorkspace = useCallback(async () => {
    try {
      return await chooseProjectWorkspace();
    } catch (error) {
      throw error instanceof Error ? error : new Error("RFX-WEB-STORAGE-001: folder access failed");
    }
  }, []);

  const createProject = useCallback(async () => {
    const engine = engineRef.current;
    if (!engine || !engineReady || busy) return;
    setBusy(true);
    setFormError("");
    try {
      const selectedWorkspace = await requestWorkspace();
      const request: ProjectRequest = { displayName: projectName.trim(), presetId, resolutionId, frameRate, durationSeconds };
      const created = await engine.createProject(request);
      await persistProject(selectedWorkspace, created.rfx, {
        projectId: created.projectId,
        projectName: created.projectName,
        revision: created.revision,
        workspaceKind: selectedWorkspace.kind,
      });
      setWorkspaceRef(selectedWorkspace);
      setSnapshot(created);
      setWorkspace("studio");
      publishDiagnostic({ code: "RFX-WEB-PROJECT-001", message: `Project.rfx committed to ${selectedWorkspace.label}.`, severity: "info" });
    } catch (error) {
      setFormError(error instanceof Error ? error.message : "RFX-WEB-PROJECT-002: project creation failed");
    } finally {
      setBusy(false);
    }
  }, [busy, durationSeconds, engineReady, frameRate, presetId, projectName, publishDiagnostic, requestWorkspace, resolutionId]);

  const openProject = useCallback(async () => {
    const engine = engineRef.current;
    if (!engine || !engineReady || busy) return;
    setBusy(true);
    setFormError("");
    try {
      const selectedWorkspace = await requestWorkspace();
      const source = await readText(selectedWorkspace.directory, "Project.rfx");
      const opened = await engine.openProject(source);
      setWorkspaceRef(selectedWorkspace);
      setSnapshot(opened);
      setWorkspace("studio");
      publishDiagnostic({ code: "RFX-WEB-PROJECT-OPEN", message: `Revision ${opened.revision} opened from ${selectedWorkspace.label}.`, severity: "info" });
    } catch (error) {
      setFormError(error instanceof Error ? error.message : "RFX-WEB-PROJECT-OPEN-001: project open failed");
    } finally {
      setBusy(false);
    }
  }, [busy, engineReady, publishDiagnostic, requestWorkspace]);

  const importVideo = useCallback(async (file: File) => {
    if (!workspaceRef || !snapshot) return;
    setBusy(true);
    try {
      const digest = await digestFile(file);
      const assetId = `asset_${digest.slice(0, 20)}`;
      const stored = await persistVideoAsset(workspaceRef, file, assetId, digest);
      setVideo({ file, assetId, digest, mediaPath: stored.mediaPath, width: 0, height: 0, duration: 0, currentTime: 0 });
      publishDiagnostic({ code: "RFX-WEB-MEDIA-001", message: `${file.name} copied to ${stored.mediaPath}; awaiting browser metadata.`, severity: "info" });
    } catch (error) {
      publishDiagnostic({ code: "RFX-WEB-MEDIA-002", message: error instanceof Error ? error.message : "Video import failed", severity: "error" });
    } finally {
      setBusy(false);
    }
  }, [publishDiagnostic, snapshot, workspaceRef]);

  const onVideoMetadata = useCallback((metadata: { width: number; height: number; duration: number }) => {
    setVideo((current) => current ? { ...current, ...metadata } : current);
    publishDiagnostic({ code: "RFX-WEB-MEDIA-003", message: `Real video admitted: ${metadata.width}×${metadata.height}, ${metadata.duration.toFixed(2)}s.`, severity: "info" });
  }, [publishDiagnostic]);

  const onVideoError = useCallback((message: string) => publishDiagnostic({ code: "RFX-WEB-MEDIA-004", message, severity: "error" }), [publishDiagnostic]);

  const onVideoTime = useCallback((currentTime: number) => {
    setVideo((current) => current ? { ...current, currentTime } : current);
  }, []);

  const onVideoEnded = useCallback(() => setPlaying(false), []);

  const togglePlayback = useCallback(() => {
    if (!video) return;
    setPlaying((value) => !value);
  }, [video]);

  return (
    <div className="app-frame">
      {workspace === "launcher" ? (
        <Launcher
          engineReady={engineReady}
          busy={busy}
          probe={probe}
          projectName={projectName}
          setProjectName={setProjectName}
          presetId={presetId}
          setPresetId={(value) => { setPresetId(value); setResolutionId("1080p"); }}
          resolutionId={resolutionId}
          setResolutionId={setResolutionId}
          frameRate={frameRate}
          setFrameRate={setFrameRate}
          durationSeconds={durationSeconds}
          setDurationSeconds={setDurationSeconds}
          formError={formError}
          onCreate={createProject}
          onOpen={openProject}
        />
      ) : (
        <Studio
          snapshot={snapshot}
          probe={probe}
          diagnostics={diagnostics}
          playing={playing}
          video={video}
          busy={busy}
          onTogglePlayback={togglePlayback}
          onImportVideo={() => videoInputRef.current?.click()}
          onVideoMetadata={onVideoMetadata}
          onVideoTime={onVideoTime}
          onVideoEnded={onVideoEnded}
          onVideoError={onVideoError}
          onBack={() => setWorkspace("launcher")}
        />
      )}
      <input ref={videoInputRef} className="visually-hidden" type="file" accept="video/*,.mp4,.mov,.webm,.mkv" onChange={(event) => { const file = event.target.files?.[0]; if (file) void importVideo(file); event.currentTarget.value = ""; }} />
    </div>
  );
}

function Brand({ suffix }: { suffix?: string }) {
  return <div className="brand"><span>ReFusion</span>{suffix && <small>{suffix}</small>}</div>;
}

type LauncherProps = {
  engineReady: boolean;
  busy: boolean;
  probe: WebGpuProbe;
  projectName: string;
  setProjectName: (value: string) => void;
  presetId: string;
  setPresetId: (value: string) => void;
  resolutionId: string;
  setResolutionId: (value: string) => void;
  frameRate: number;
  setFrameRate: (value: number) => void;
  durationSeconds: number;
  setDurationSeconds: (value: number) => void;
  formError: string;
  onCreate: () => void;
  onOpen: () => void;
};

function Launcher(props: LauncherProps) {
  const preset = presets.find((item) => item.id === props.presetId) ?? presets[0];
  return (
    <section className="launcher-page">
      <header className="launcher-header"><Brand /><span className="launcher-kicker">WEBGPU STUDIO</span><div className="spacer" /><button className="button quiet" onClick={props.onOpen} disabled={!props.engineReady || props.busy}>Open Existing Project</button></header>
      <main className="launcher-content">
        <article className="launcher-card create-card">
          <div className="eyebrow">PROJECT LAUNCHER</div>
          <h1>Create a real project</h1>
          <p>The WASM Core creates canonical Project.rfx bytes. The next step lets you choose where the workspace is stored.</p>
          <label>Project name<input placeholder="My ReFusion Project" value={props.projectName} onChange={(event) => props.setProjectName(event.target.value)} disabled={!props.engineReady || props.busy} /></label>
          <div className="field-row"><label>Frame rate<select value={props.frameRate} onChange={(event) => props.setFrameRate(Number(event.target.value))} disabled={!props.engineReady || props.busy}>{frameRates.map((rate) => <option key={rate} value={rate}>{rate} fps</option>)}</select></label><label>Duration<input type="number" min="1" max="86400" value={props.durationSeconds} onChange={(event) => props.setDurationSeconds(Math.max(1, Number(event.target.value) || 1))} disabled={!props.engineReady || props.busy} /></label></div>
          <button className="button primary" onClick={props.onCreate} disabled={!props.engineReady || props.busy || !props.projectName.trim()}>{props.busy ? "Preparing workspace…" : "Create project and choose folder"}</button>
          {props.formError && <p className="form-error">{props.formError}</p>}
          <p className="card-note">Chrome, Edge and Brave can write to a selected desktop folder. Safari uses a browser-private workspace because it does not expose a writable desktop-folder picker.</p>
        </article>
        <article className="launcher-card composition-card">
          <div className="card-heading"><div><div className="eyebrow">COMPOSITION</div><h2>Choose the project canvas</h2></div><span className="profile-pill">{preset.aspect}</span></div>
          <div className="preset-grid">{presets.map((item) => <button key={item.id} className={`preset-card ${props.presetId === item.id ? "selected" : ""}`} onClick={() => props.setPresetId(item.id)} disabled={!props.engineReady || props.busy}><span className={`preset-shape ${item.id}`} /><strong>{item.name}</strong><small>{item.aspect}</small></button>)}</div>
          <label className="resolution-field">Resolution<select value={props.resolutionId} onChange={(event) => props.setResolutionId(event.target.value)} disabled={!props.engineReady || props.busy}>{preset.resolutions.map(([id, name, width, height]) => <option key={id} value={id}>{name} — {width}×{height}</option>)}</select></label>
          <div className="capability-panel"><div className="capability-title"><span className={`status-dot ${props.probe.state}`} />WebGPU capability</div><div className="capability-values"><span>{props.probe.adapter}</span><span>{props.probe.limits}</span></div><p>{props.engineReady ? "WASM Core ready for project commands." : "Loading the C++ Core WASM module…"} {props.probe.diagnostic}</p></div>
        </article>
      </main>
    </section>
  );
}

function Studio({ snapshot, probe, diagnostics, playing, video, busy, onTogglePlayback, onImportVideo, onVideoMetadata, onVideoTime, onVideoEnded, onVideoError, onBack }: { snapshot: ProjectSnapshot | null; probe: WebGpuProbe; diagnostics: EngineDiagnostic[]; playing: boolean; video: ImportedVideo | null; busy: boolean; onTogglePlayback: () => void; onImportVideo: () => void; onVideoMetadata: (metadata: { width: number; height: number; duration: number }) => void; onVideoTime: (time: number) => void; onVideoEnded: () => void; onVideoError: (message: string) => void; onBack: () => void }) {
  return <section className="studio-page"><header className="studio-header"><button className="brand-button" onClick={onBack} aria-label="Back to launcher"><Brand /></button><div className="spacer" /><span className="project-meta">{snapshot?.projectName ?? "No accepted project"} <b>•</b> Revision {snapshot?.revision ?? "—"}</span></header><div className="studio-layout"><aside className="tool-rail" aria-label="Command surface">{tools.map((tool) => <button key={tool} disabled={tool !== "VID" || !snapshot || busy} onClick={tool === "VID" ? onImportVideo : undefined} title={tool === "VID" ? "Insert a real video file" : `Command surface: ${tool}`}>{tool}</button>)}</aside><main className="center-column"><section className="canvas-panel"><CanvasHost probe={probe} videoFile={video?.file ?? null} canvasWidth={snapshot?.width ?? 16} canvasHeight={snapshot?.height ?? 9} playing={playing} onVideoMetadata={onVideoMetadata} onVideoTime={onVideoTime} onVideoEnded={onVideoEnded} onVideoError={onVideoError} /><div className="canvas-transport"><button onClick={onTogglePlayback} disabled={!video || busy} aria-label="Toggle playback">{playing ? "❚❚" : "▶"}</button><button disabled>FIT</button><button disabled>ACTUAL</button><span>{video ? `${formatTime(video.currentTime)} / ${formatTime(video.duration)}` : "00:00 / 00:00"}</span></div></section><Timeline playing={playing} video={video} onTogglePlayback={onTogglePlayback} /></main><Inspector diagnostics={diagnostics} probe={probe} snapshot={snapshot} video={video} /></div></section>;
}

function formatTime(seconds: number) {
  if (!Number.isFinite(seconds) || seconds <= 0) return "00:00";
  const total = Math.floor(seconds);
  return `${String(Math.floor(total / 60)).padStart(2, "0")}:${String(total % 60).padStart(2, "0")}`;
}

function Timeline({ playing, video, onTogglePlayback }: { playing: boolean; video: ImportedVideo | null; onTogglePlayback: () => void }) {
  const duration = video?.duration ?? 0;
  const ratio = duration > 0 && video ? Math.min(1, Math.max(0, video.currentTime / duration)) : 0;
  const marks = duration > 0 ? Array.from({ length: 7 }, (_, index) => Math.round((duration * index) / 6)) : [0, 5, 10, 15, 20, 25, 30];
  return <section className="timeline-panel"><div className="timeline-toolbar"><div><button disabled={!video}>◀</button><button onClick={onTogglePlayback} disabled={!video}>{playing ? "❚❚" : "▶"}</button><button disabled={!video}>▶</button></div><span className="transport-state">{video ? (playing ? "PLAYING REAL MEDIA" : "MEDIA READY") : "NO MEDIA"}</span><span className="timecode">{video ? `${formatTime(video.currentTime)} / ${formatTime(duration)}` : "00:00:00:00"}</span></div><div className="timeline-body"><div className="ruler">{marks.map((mark, index) => <span key={`${mark}-${index}`}>{mark}s</span>)}</div>{video ? <><div className="playhead" style={{ left: `calc(142px + (100% - 160px) * ${ratio})` }} aria-label={`Playhead ${formatTime(video.currentTime)}`} /><div className="track-row"><div className="track-label"><span style={{ color: "#3975c6" }}>▶</span>Video</div><div className="track-lane"><div className="empty-track accepted-track" style={{ width: "100%", borderColor: "#3975c6" }}><span>{video.file.name} · {video.width ? `${video.width}×${video.height}` : "probing"}</span></div></div></div></> : <div className="timeline-empty">Select VID to insert a real video file into this project workspace.</div>}<div className="timeline-empty">{video ? "Real media track. Semantic VideoClip admission into the shared RenderPlan is the next Core gate." : ""}</div></div></section>;
}

function Inspector({ diagnostics, probe, snapshot, video }: { diagnostics: EngineDiagnostic[]; probe: WebGpuProbe; snapshot: ProjectSnapshot | null; video: ImportedVideo | null }) {
  return <aside className="inspector-panel"><div className="inspector-scroll"><div className="eyebrow">INSPECTOR</div><h2>{video ? video.file.name : "No media selected"}</h2><p className="muted">{video ? "The browser has admitted this real media asset to the WebGPU presenter." : "Choose VID to import a real video into the selected project workspace."}</p><div className="rule" /><div className="inspector-section"><span className="section-label">PROJECT</span><dl><dt>Project ID</dt><dd>{snapshot?.projectId ?? "Unavailable"}</dd><dt>Revision</dt><dd>{snapshot?.revision ?? "—"}</dd><dt>Canvas</dt><dd>{snapshot ? `${snapshot.width}×${snapshot.height}` : "—"}</dd></dl></div><div className="inspector-section"><span className="section-label">GPU PROFILE</span><dl><dt>WebGPU</dt><dd className={probe.state === "ready" ? "good" : "warn"}>{probe.state}</dd><dt>Skia/WASM</dt><dd className="good">Core WASM ready</dd><dt>Video path</dt><dd className={video ? "good" : "warn"}>{video ? "GPU external copy" : "VID pending"}</dd></dl></div><div className="rule" /><div className="inspector-section diagnostics"><span className="section-label">DIAGNOSTICS</span>{diagnostics.length === 0 ? <p className="muted">No diagnostics published.</p> : diagnostics.map((item, index) => <div className="diagnostic" key={`${item.code}-${index}`}><strong>{item.code}</strong><span>{item.message}</span></div>)}</div></div></aside>;
}

export default App;
