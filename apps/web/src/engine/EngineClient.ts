import type { EngineCommand, EngineDiagnostic, EngineMessage, ProjectRequest, ProjectSnapshot } from "./protocol";
import { webEngineUnavailable } from "./protocol";

export class EngineClient {
  private readonly worker: Worker;
  private readonly listeners = new Set<(message: EngineMessage) => void>();

  constructor() {
    this.worker = new Worker(
      new URL("./engine-worker.ts", import.meta.url),
      { type: "module", name: "refusion-engine" },
    );
    this.worker.addEventListener("message", (event: MessageEvent<EngineMessage>) => {
      this.listeners.forEach((listener) => listener(event.data));
    });
    this.worker.addEventListener("error", () => {
      this.emitDiagnostic({
        code: "RFX-WEB-WASM-002",
        message: "The engine Worker stopped before it could publish a revision.",
        severity: "error",
      });
    });
  }

  subscribe(listener: (message: EngineMessage) => void) {
    this.listeners.add(listener);
    return () => this.listeners.delete(listener);
  }

  send(command: EngineCommand) {
    this.worker.postMessage(command);
  }

  requestSnapshot(command: Extract<EngineCommand, { type: "create_project" | "open_project" }>): Promise<ProjectSnapshot> {
    return new Promise((resolve, reject) => {
      let settled = false;
      const unsubscribe = this.subscribe((message) => {
        if (message.kind === "engine_snapshot") {
          settled = true;
          unsubscribe();
          resolve(message);
        } else if (message.kind === "diagnostic" && message.diagnostic.severity === "error") {
          settled = true;
          unsubscribe();
          reject(new Error(`${message.diagnostic.code}: ${message.diagnostic.message}`));
        }
      });
      window.setTimeout(() => {
        if (settled) return;
        unsubscribe();
        reject(new Error("RFX-WEB-WASM-004: WebCore did not publish a project snapshot"));
      }, 30_000);
      this.send(command);
    });
  }

  createProject(request: ProjectRequest): Promise<ProjectSnapshot> {
    return this.requestSnapshot({ type: "create_project", request });
  }

  openProject(source: string): Promise<ProjectSnapshot> {
    return this.requestSnapshot({ type: "open_project", source });
  }

  dispose() {
    this.worker.terminate();
    this.listeners.clear();
  }

  private emitDiagnostic(diagnostic: EngineDiagnostic) {
    this.listeners.forEach((listener) =>
      listener({ kind: "diagnostic", diagnostic }),
    );
  }
}

export { webEngineUnavailable };
