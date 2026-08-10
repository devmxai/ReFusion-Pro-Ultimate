import type { EngineCommand, EngineMessage } from "./protocol";
import { webEngineUnavailable } from "./protocol";

// WEB-WP01 owns loading the digest-pinned C++ WASM module. Until that module is
// admitted, the worker must expose an explicit unavailable state rather than
// inventing a project snapshot or a second JavaScript evaluator.
const post = (message: EngineMessage) => self.postMessage(message);

post({ kind: "diagnostic", diagnostic: webEngineUnavailable });

self.onmessage = (event: MessageEvent<EngineCommand>) => {
  switch (event.data.type) {
    case "request_snapshot":
      post({ kind: "diagnostic", diagnostic: webEngineUnavailable });
      break;
    case "toggle_transport":
      post({
        kind: "diagnostic",
        diagnostic: {
          code: "RFX-WEB-ENGINE-002",
          message:
            "Transport is disabled until an accepted WASM revision and ProjectClock are available.",
          severity: "info",
        },
      });
      break;
    case "request_capabilities":
      post({ kind: "diagnostic", diagnostic: webEngineUnavailable });
      break;
  }
};
