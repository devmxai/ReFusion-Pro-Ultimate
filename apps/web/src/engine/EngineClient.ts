import type {
  EngineCommand,
  EngineDiagnostic,
  EngineMessage,
} from "./protocol";
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
