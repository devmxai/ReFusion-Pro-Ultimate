import { useEffect, useRef, useState } from "react";
import { CanvasHost } from "./components/CanvasHost";
import { EngineClient } from "./engine/EngineClient";
import type { EngineDiagnostic, EngineMessage } from "./engine/protocol";
import { probeWebGpu, type WebGpuProbe } from "./platform/webgpuProbe";

const tools = ["VID", "IMG", "TXT", "BG", "SHP", "AUD", "SVG"];
const tracks = [
  { type: "video", icon: "▶", name: "Video", color: "#3975c6" },
  { type: "audio", icon: "♪", name: "Audio", color: "#2b9c72" },
  { type: "text", icon: "T", name: "Text", color: "#d8873f" },
  { type: "shape", icon: "◇", name: "Shape", color: "#765bd6" },
];

function App() {
  const [workspace, setWorkspace] = useState<"launcher" | "studio">("launcher");
  const [probe, setProbe] = useState<WebGpuProbe>({
    state: "checking",
    adapter: "Checking browser capability…",
    limits: "—",
    diagnostic: "Requesting a browser-mediated high-performance adapter.",
  });
  const [diagnostics, setDiagnostics] = useState<EngineDiagnostic[]>([]);
  const [playing, setPlaying] = useState(false);
  const engineRef = useRef<EngineClient | null>(null);

  useEffect(() => {
    let active = true;
    const engine = new EngineClient();
    engineRef.current = engine;
    void probeWebGpu().then((result) => active && setProbe(result));
    const unsubscribe = engine.subscribe((message: EngineMessage) => {
      if (message.kind === "diagnostic") {
        setDiagnostics((items) => [message.diagnostic, ...items].slice(0, 5));
      }
    });
    engine.send({ type: "request_snapshot" });
    return () => {
      active = false;
      unsubscribe();
      engine.dispose();
      if (engineRef.current === engine) {
        engineRef.current = null;
      }
    };
  }, []);

  const togglePlayback = () => {
    setPlaying((value) => !value);
    engineRef.current?.send({ type: "toggle_transport" });
  };

  return (
    <div className="app-frame">
      {workspace === "launcher" ? (
        <Launcher onEnter={() => setWorkspace("studio")} probe={probe} />
      ) : (
        <Studio
          probe={probe}
          diagnostics={diagnostics}
          playing={playing}
          onTogglePlayback={togglePlayback}
          onBack={() => setWorkspace("launcher")}
        />
      )}
    </div>
  );
}

function Brand({ suffix }: { suffix?: string }) {
  return <div className="brand"><span>ReFusion</span>{suffix && <small>{suffix}</small>}</div>;
}

function Launcher({ onEnter, probe }: { onEnter: () => void; probe: WebGpuProbe }) {
  return (
    <section className="launcher-page">
      <header className="launcher-header">
        <Brand />
        <span className="launcher-kicker">WEBGPU STUDIO</span>
        <div className="spacer" />
        <button className="button quiet" onClick={onEnter}>Open Studio shell</button>
      </header>
      <main className="launcher-content">
        <article className="launcher-card create-card">
          <div className="eyebrow">PROJECT LAUNCHER</div>
          <h1>Build a real composition</h1>
          <p>The browser shell is connected to the engine boundary. Project creation becomes available when the WASM authority is admitted.</p>
          <label>Project name<input placeholder="My ReFusion Project" disabled /></label>
          <div className="field-row"><label>Duration<input value="30" disabled /></label><span className="unit">seconds</span></div>
          <button className="button primary" disabled title="Waiting for WEB-WP01 WASM semantic closure">Create real project</button>
          <p className="card-note">No project is fabricated in this experiment. The next gate wires this form to the existing Application command service.</p>
        </article>
        <article className="launcher-card composition-card">
          <div className="card-heading"><div><div className="eyebrow">COMPOSITION</div><h2>Professional Web target</h2></div><span className="profile-pill">PROPOSED</span></div>
          <div className="preset-grid">
            {[["reels-9x16", "Reels", "34 / 61"], ["portrait-4x5", "Portrait", "45 / 56"], ["youtube-16x9", "YouTube", "72 / 41"], ["cinematic-239x100", "Cinematic", "82 / 34"]].map(([id, name, size]) => (
              <button key={id} className="preset-card" disabled><span className={`preset-shape ${id}`} style={{ width: `${size.split(" /")[0]}px`, height: `${size.split(" /")[1]}px` }} /><strong>{name}</strong><small>{id}</small></button>
            ))}
          </div>
          <div className="capability-panel"><div className="capability-title"><span className={`status-dot ${probe.state}`} />WebGPU capability</div><div className="capability-values"><span>{probe.adapter}</span><span>{probe.limits}</span></div><p>{probe.diagnostic}</p></div>
        </article>
      </main>
    </section>
  );
}

function Studio({ probe, diagnostics, playing, onTogglePlayback, onBack }: { probe: WebGpuProbe; diagnostics: EngineDiagnostic[]; playing: boolean; onTogglePlayback: () => void; onBack: () => void }) {
  return (
    <section className="studio-page">
      <header className="studio-header"><button className="brand-button" onClick={onBack} aria-label="Back to launcher"><Brand /></button><div className="spacer" /><span className="project-meta">No accepted project <b>•</b> Web profile proposed</span></header>
      <div className="studio-layout">
        <aside className="tool-rail" aria-label="Command surface">{tools.map((tool) => <button key={tool} disabled title={`Command surface: ${tool}`}>{tool}</button>)}</aside>
        <main className="center-column">
          <section className="canvas-panel"><CanvasHost probe={probe} /><div className="canvas-transport"><button onClick={onTogglePlayback} disabled={!probe.device} aria-label="Toggle playback">{playing ? "❚❚" : "▶"}</button><button disabled>FIT</button><button disabled>ACTUAL</button><span>00:00 / 00:00</span></div></section>
          <Timeline playing={playing} />
        </main>
        <Inspector diagnostics={diagnostics} probe={probe} />
      </div>
    </section>
  );
}

function Timeline({ playing }: { playing: boolean }) {
  return <section className="timeline-panel"><div className="timeline-toolbar"><div><button onClick={() => undefined} disabled={!playing}>◀</button><button disabled>▶</button><button disabled>▶</button></div><span className="transport-state">{playing ? "TRANSPORT REQUESTED" : "PAUSED"}</span><span className="timecode">00:00:00:00</span></div><div className="timeline-body"><div className="ruler"><span>0s</span><span>5s</span><span>10s</span><span>15s</span><span>20s</span><span>25s</span><span>30s</span></div>{tracks.map((track, index) => <div className="track-row" key={track.type}><div className="track-label"><span style={{ color: track.color }}>{track.icon}</span>{track.name}</div><div className="track-lane"><div className="empty-track" style={{ borderColor: track.color }}><span>{index === 0 ? "Awaiting accepted VideoClip" : "No accepted clip"}</span></div></div></div>)}<div className="timeline-empty">Timeline projection awaits an accepted Revision from the WASM engine.</div></div></section>;
}

function Inspector({ diagnostics, probe }: { diagnostics: EngineDiagnostic[]; probe: WebGpuProbe }) {
  return <aside className="inspector-panel"><div className="inspector-scroll"><div className="eyebrow">INSPECTOR</div><h2>No selection</h2><p className="muted">Select a Layer or Group on the Timeline after a project revision is accepted.</p><div className="rule" /><div className="inspector-section"><span className="section-label">PROJECT</span><dl><dt>Project ID</dt><dd>Unavailable</dd><dt>Revision</dt><dd>—</dd><dt>Canvas</dt><dd>—</dd></dl></div><div className="inspector-section"><span className="section-label">GPU PROFILE</span><dl><dt>WebGPU</dt><dd className={probe.state === "ready" ? "good" : "warn"}>{probe.state}</dd><dt>Skia/WASM</dt><dd className="warn">WEB-WP01 pending</dd><dt>Video decode</dt><dd className="warn">WEB-WP05 pending</dd></dl></div><div className="rule" /><div className="inspector-section diagnostics"><span className="section-label">DIAGNOSTICS</span>{diagnostics.length === 0 ? <p className="muted">No diagnostics published.</p> : diagnostics.map((item, index) => <div className="diagnostic" key={`${item.code}-${index}`}><strong>{item.code}</strong><span>{item.message}</span></div>)}</div></div></aside>;
}

export default App;
