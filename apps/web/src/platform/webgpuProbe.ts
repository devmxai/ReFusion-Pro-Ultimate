export type WebGpuProbe = {
  state: "checking" | "ready" | "unsupported" | "error";
  adapter: string;
  limits: string;
  diagnostic: string;
  device?: GPUDevice;
};

export async function probeWebGpu(): Promise<WebGpuProbe> {
  if (!("gpu" in navigator) || !navigator.gpu) {
    return {
      state: "unsupported",
      adapter: "Unavailable",
      limits: "—",
      diagnostic: "This browser does not expose WebGPU in a secure context.",
    };
  }

  try {
    const adapter = await navigator.gpu.requestAdapter({
      powerPreference: "high-performance",
    });
    if (!adapter) {
      return {
        state: "unsupported",
        adapter: "No adapter admitted",
        limits: "—",
        diagnostic: "The user agent could not admit a WebGPU adapter.",
      };
    }

    const device = await adapter.requestDevice();
    const info = adapter.info;
    const adapterName = [info.vendor, info.architecture]
      .filter(Boolean)
      .join(" / ") || "Browser-mediated GPU adapter";
    const limits = `${device.limits.maxTextureDimension2D.toLocaleString()}px textures · ${device.limits.maxBufferSize > 1_000_000_000 ? "large buffers" : "bounded buffers"}`;
    return {
      state: "ready",
      adapter: adapterName,
      limits,
      diagnostic: "WebGPU device admitted. Skia/WASM render backend is the next gate.",
      device,
    };
  } catch (error) {
    return {
      state: "error",
      adapter: "Probe failed",
      limits: "—",
      diagnostic: error instanceof Error ? error.message : "Unknown WebGPU failure.",
    };
  }
}
