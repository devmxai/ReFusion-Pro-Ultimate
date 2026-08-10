import type { EngineCommand, EngineMessage, ProjectRequest } from "./protocol";
import { webEngineUnavailable } from "./protocol";
import createCore from "../wasm/refusion_core.js";

type WebCoreModule = {
  _rf_web_create_project: (displayName: number, presetId: number, resolutionId: number, frameRate: number, durationSeconds: number) => number;
  _rf_web_open_project: (source: number) => number;
  _rf_web_capabilities: () => number;
  _malloc: (size: number) => number;
  _free: (pointer: number) => void;
  UTF8ToString: (pointer: number) => string;
  lengthBytesUTF8: (value: string) => number;
  stringToUTF8: (value: string, pointer: number, maxBytes: number) => void;
};

const post = (message: EngineMessage) => self.postMessage(message);
let corePromise: Promise<WebCoreModule> | null = null;
let activeSnapshot: Extract<EngineMessage, { kind: "engine_snapshot" }> | null = null;

async function loadCore(): Promise<WebCoreModule> {
  if (!corePromise) {
    corePromise = Promise.resolve(createCore()).then(async (core) => {
      post({ kind: "engine_ready", backend: "wasm", build: "refusion-core-v1" });
      return core;
    });
  }
  return corePromise;
}

function withUtf8(core: WebCoreModule, value: string, fn: (pointer: number) => number): string {
  const bytes = core.lengthBytesUTF8(value) + 1;
  const pointer = core._malloc(bytes);
  try {
    core.stringToUTF8(value, pointer, bytes);
    return core.UTF8ToString(fn(pointer));
  } finally {
    core._free(pointer);
  }
}

function parseResponse(json: string): void {
  const result = JSON.parse(json) as Record<string, unknown>;
  if (!result.ok) {
    post({
      kind: "diagnostic",
      diagnostic: {
        code: String(result.code ?? "RFX-WEB-CORE-001"),
        message: String(result.message ?? "WebCore rejected the command"),
        severity: "error",
      },
    });
    return;
  }
  const snapshot = { kind: "engine_snapshot", accepted: true, ...result } as Extract<EngineMessage, { kind: "engine_snapshot" }>;
  activeSnapshot = snapshot;
  post(snapshot);
}

async function createProject(request: ProjectRequest): Promise<void> {
  const core = await loadCore();
  const pointers: number[] = [];
  const pointer = (value: string) => {
    const bytes = core.lengthBytesUTF8(value) + 1;
    const result = core._malloc(bytes);
    core.stringToUTF8(value, result, bytes);
    pointers.push(result);
    return result;
  };
  try {
    const result = core._rf_web_create_project(
      pointer(request.displayName),
      pointer(request.presetId),
      pointer(request.resolutionId),
      request.frameRate,
      request.durationSeconds,
    );
    parseResponse(core.UTF8ToString(result));
  } finally {
    pointers.forEach((item) => core._free(item));
  }
}

async function openProject(source: string): Promise<void> {
  const core = await loadCore();
  parseResponse(withUtf8(core, source, (pointer) => core._rf_web_open_project(pointer)));
}

self.onmessage = (event: MessageEvent<EngineCommand>) => {
  void (async () => {
    try {
      switch (event.data.type) {
        case "request_snapshot":
          if (activeSnapshot) post(activeSnapshot);
          else await loadCore();
          break;
        case "request_capabilities":
          await loadCore();
          break;
        case "create_project":
          await createProject(event.data.request);
          break;
        case "open_project":
          await openProject(event.data.source);
          break;
        case "toggle_transport":
          post({
            kind: "diagnostic",
            diagnostic: {
              code: activeSnapshot ? "RFX-WEB-TRANSPORT-001" : webEngineUnavailable.code,
              message: activeSnapshot ? "Transport clock wiring is the next media gate." : webEngineUnavailable.message,
              severity: "info",
            },
          });
          break;
      }
    } catch (error) {
      post({
        kind: "diagnostic",
        diagnostic: {
          code: "RFX-WEB-WASM-003",
          message: error instanceof Error ? error.message : "WebCore Worker failed",
          severity: "error",
        },
      });
    }
  })();
};
