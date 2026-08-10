import { useEffect, useRef } from "react";
import type { WebGpuProbe } from "../platform/webgpuProbe";

type CanvasHostProps = { probe: WebGpuProbe };

export function CanvasHost({ probe }: CanvasHostProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (probe.state !== "ready" || !probe.device || !canvasRef.current) return;
    const canvas = canvasRef.current;
    const context = canvas.getContext("webgpu");
    if (!context) return;

    const format = navigator.gpu.getPreferredCanvasFormat();
    context.configure({ device: probe.device, format, alphaMode: "opaque" });
    const encoder = probe.device.createCommandEncoder({ label: "viewport-probe" });
    const view = context.getCurrentTexture().createView();
    const pass = encoder.beginRenderPass({
      colorAttachments: [{
        view,
        clearValue: { r: 0.02, g: 0.025, b: 0.04, a: 1 },
        loadOp: "clear",
        storeOp: "store",
      }],
    });
    pass.end();
    probe.device.queue.submit([encoder.finish()]);
  }, [probe]);

  return (
    <div className="viewport-shell">
      <canvas ref={canvasRef} className="gpu-canvas" aria-label="ReFusion GPU viewport" />
      <div className="viewport-status">
        <span className={`status-dot ${probe.state}`} />
        <div>
          <strong>
            {probe.state === "ready" ? "WebGPU target admitted" : "Native GPU viewport boundary"}
          </strong>
          <p>{probe.state === "ready" ? "Canvas clear probe only — waiting for Skia/WASM RenderPlan." : probe.diagnostic}</p>
        </div>
      </div>
    </div>
  );
}
