export type ProjectRequest = {
  displayName: string;
  presetId: string;
  resolutionId: string;
  frameRate: number;
  durationSeconds: number;
};

export type ProjectSnapshot = {
  kind: "engine_snapshot";
  accepted: true;
  revision: number;
  projectId: string;
  projectName: string;
  compositionId: string;
  width: number;
  height: number;
  frameRate: number;
  durationSeconds: number;
  rfx: string;
};

export type EngineCommand =
  | { type: "request_snapshot" }
  | { type: "request_capabilities" }
  | { type: "create_project"; request: ProjectRequest }
  | { type: "open_project"; source: string }
  | { type: "toggle_transport" };

export type EngineDiagnostic = {
  code: string;
  message: string;
  severity: "info" | "warning" | "error";
};

export type EngineMessage =
  | ProjectSnapshot
  | { kind: "engine_ready"; backend: "wasm"; build: string }
  | { kind: "diagnostic"; diagnostic: EngineDiagnostic };

export const webEngineUnavailable: EngineDiagnostic = {
  code: "RFX-WEB-WASM-001",
  message: "The WebCore WASM module is not available; project commands are disabled.",
  severity: "error",
};
