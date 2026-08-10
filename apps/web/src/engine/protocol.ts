export type EngineCommand =
  | { type: "request_snapshot" }
  | { type: "toggle_transport" }
  | { type: "request_capabilities" };

export type EngineDiagnostic = {
  code: string;
  message: string;
  severity: "info" | "warning" | "error";
};

export type EngineSnapshot = {
  kind: "engine_snapshot";
  accepted: false;
  revision: null;
  projectName: null;
  composition: null;
  playing: false;
  diagnostic: EngineDiagnostic;
};

export type EngineMessage =
  | EngineSnapshot
  | { kind: "engine_ready"; backend: "wasm"; build: string }
  | { kind: "diagnostic"; diagnostic: EngineDiagnostic };

export const webEngineUnavailable: EngineDiagnostic = {
  code: "RFX-WEB-WASM-001",
  message:
    "The Web shell is ready, but the ReFusion WASM engine is not materialized yet. No project or video fallback is being presented.",
  severity: "info",
};
