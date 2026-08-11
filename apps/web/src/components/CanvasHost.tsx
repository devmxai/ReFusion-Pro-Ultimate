import { useEffect, useRef, useState } from "react";
import type { WebGpuProbe } from "../platform/webgpuProbe";

type CanvasHostProps = {
  probe: WebGpuProbe;
  videoFile: File | null;
  canvasWidth: number;
  canvasHeight: number;
  playing: boolean;
  onVideoMetadata: (metadata: { width: number; height: number; duration: number }) => void;
  onVideoTime: (time: number) => void;
  onVideoEnded: () => void;
  onVideoError: (message: string) => void;
};

type GpuVideoState = "idle" | "ready" | "unsupported";

export function CanvasHost({ probe, videoFile, canvasWidth, canvasHeight, playing, onVideoMetadata, onVideoTime, onVideoEnded, onVideoError }: CanvasHostProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const [videoState, setVideoState] = useState<GpuVideoState>("idle");

  useEffect(() => {
    if (!videoFile) {
      videoRef.current?.pause();
      videoRef.current?.removeAttribute("src");
      videoRef.current = null;
      setVideoState("idle");
      return;
    }
    const video = document.createElement("video");
    const objectUrl = URL.createObjectURL(videoFile);
    video.preload = "auto";
    video.muted = true;
    video.playsInline = true;
    video.src = objectUrl;
    videoRef.current = video;
    const onMetadata = () => {
      onVideoMetadata({ width: video.videoWidth, height: video.videoHeight, duration: video.duration });
    };
    const onLoaded = () => {
      setVideoState("ready");
      void video.play().then(() => video.pause()).catch(() => undefined);
    };
    const onError = () => {
      setVideoState("unsupported");
      onVideoError(`The browser could not decode ${videoFile.name} through its native media pipeline.`);
    };
    video.addEventListener("loadedmetadata", onMetadata);
    video.addEventListener("loadeddata", onLoaded);
    video.addEventListener("ended", onVideoEnded);
    video.addEventListener("error", onError);
    video.load();
    return () => {
      video.pause();
      video.removeEventListener("loadedmetadata", onMetadata);
      video.removeEventListener("loadeddata", onLoaded);
      video.removeEventListener("ended", onVideoEnded);
      video.removeEventListener("error", onError);
      URL.revokeObjectURL(objectUrl);
      if (videoRef.current === video) videoRef.current = null;
    };
  }, [videoFile, onVideoEnded, onVideoError, onVideoMetadata]);

  useEffect(() => {
    const video = videoRef.current;
    if (!video || videoState !== "ready") return;
    if (playing) {
      void video.play().catch(() => onVideoError("Playback was blocked; press Play again after the media is admitted."));
    } else {
      video.pause();
    }
  }, [playing, videoState, onVideoError]);

  useEffect(() => {
    if (probe.state !== "ready" || !probe.device || !canvasRef.current || !videoFile) return;
    const canvas = canvasRef.current;
    const context = canvas.getContext("webgpu");
    if (!context) return;
    const device = probe.device;

    const format = navigator.gpu.getPreferredCanvasFormat();
    const width = Math.max(1, Math.floor(canvas.clientWidth * window.devicePixelRatio));
    const height = Math.max(1, Math.floor(canvas.clientHeight * window.devicePixelRatio));
    canvas.width = width;
    canvas.height = height;
    context.configure({ device, format, alphaMode: "opaque" });
    const video = videoRef.current;
    if (!video || videoState !== "ready") return;
    const textureWidth = video.videoWidth;
    const textureHeight = video.videoHeight;
    if (!textureWidth || !textureHeight) return;
    if (textureWidth > probe.device.limits.maxTextureDimension2D || textureHeight > probe.device.limits.maxTextureDimension2D) {
      onVideoError(`Video dimensions ${textureWidth}×${textureHeight} exceed this adapter's WebGPU texture limit.`);
      return;
    }
    const texture = device.createTexture({
      label: "refusion-video-frame",
      size: [textureWidth, textureHeight],
      format: "rgba8unorm",
      usage: GPUTextureUsage.COPY_DST | GPUTextureUsage.TEXTURE_BINDING,
    });
    const sampler = device.createSampler({ magFilter: "linear", minFilter: "linear" });
    const pipeline = device.createRenderPipeline({
      label: "refusion-video-presenter",
      layout: "auto",
      vertex: {
        module: device.createShaderModule({ code: `
          @vertex fn main(@builtin(vertex_index) index: u32) -> @builtin(position) vec4f {
            var positions = array<vec2f, 3>(vec2f(-1.0, -1.0), vec2f(3.0, -1.0), vec2f(-1.0, 3.0));
            return vec4f(positions[index], 0.0, 1.0);
          }
        ` }),
        entryPoint: "main",
      },
      fragment: {
        module: device.createShaderModule({ code: `
          @group(0) @binding(0) var frameSampler: sampler;
          @group(0) @binding(1) var frameTexture: texture_2d<f32>;
          @fragment fn main(@builtin(position) position: vec4f) -> @location(0) vec4f {
            let uv = position.xy / vec2f(f32(${width}), f32(${height}));
            return textureSample(frameTexture, frameSampler, uv);
          }
        ` }),
        entryPoint: "main",
        targets: [{ format }],
      },
      primitive: { topology: "triangle-list" },
    });
    const bindGroup = device.createBindGroup({
      layout: pipeline.getBindGroupLayout(0),
      entries: [{ binding: 0, resource: sampler }, { binding: 1, resource: texture.createView() }],
    });
    const onUncapturedError = (event: Event) => {
      const error = (event as GPUUncapturedErrorEvent).error;
      onVideoError(`WebGPU rejected a decoded video frame: ${error.message}`);
    };
    device.addEventListener("uncapturederror", onUncapturedError);
    let frameRequest = 0;
    let stopped = false;
    const render = () => {
      if (stopped || !videoRef.current || video.readyState < HTMLMediaElement.HAVE_CURRENT_DATA) return;
      onVideoTime(video.currentTime);
      let frame: VideoFrame | null = null;
      try {
        // VideoFrame makes the browser's decoded frame explicit before the
        // external-image copy. Older engines can still use the HTMLVideoElement.
        let source: CanvasImageSource = video;
        if (typeof VideoFrame === "function") {
          try {
            frame = new VideoFrame(video);
            source = frame;
          } catch {
            // Some WebKit builds expose VideoFrame but do not allow constructing
            // one from an HTMLVideoElement; the native source remains valid.
          }
        }
        device.queue.copyExternalImageToTexture({ source, flipY: true }, { texture }, [textureWidth, textureHeight]);
      } catch (error) {
        stopped = true;
        onVideoError(error instanceof Error ? `Video frame presentation failed: ${error.message}` : "Video frame presentation failed.");
      } finally {
        frame?.close();
      }
      if (stopped) return;
      const encoder = device.createCommandEncoder({ label: "refusion-video-frame" });
      const pass = encoder.beginRenderPass({
        colorAttachments: [{
          view: context.getCurrentTexture().createView(),
          clearValue: { r: 0.02, g: 0.025, b: 0.04, a: 1 },
          loadOp: "clear",
          storeOp: "store",
        }],
      });
      pass.setPipeline(pipeline);
      pass.setBindGroup(0, bindGroup);
      pass.draw(3);
      pass.end();
      device.queue.submit([encoder.finish()]);
    };
    const tick = () => {
      render();
      frameRequest = requestAnimationFrame(tick);
    };
    frameRequest = requestAnimationFrame(tick);
    return () => {
      stopped = true;
      cancelAnimationFrame(frameRequest);
      device.removeEventListener("uncapturederror", onUncapturedError);
      texture.destroy();
    };
  }, [canvasHeight, canvasWidth, onVideoError, onVideoTime, probe, videoFile, videoState]);

  return (
    <div className="viewport-shell">
      <canvas ref={canvasRef} width={canvasWidth} height={canvasHeight} className="gpu-canvas" style={{ aspectRatio: `${canvasWidth} / ${canvasHeight}` }} aria-label="ReFusion GPU viewport" />
      <div className="viewport-status">
        <span className={`status-dot ${probe.state}`} />
        <div>
          <strong>
            {videoFile && videoState === "ready" ? "Real video frame admitted" : probe.state === "ready" ? "WebGPU target admitted" : "Native GPU viewport boundary"}
          </strong>
          <p>{videoFile && videoState === "ready" ? "Video element decoded by the browser and copied directly to the WebGPU texture path." : probe.state === "ready" ? "Canvas clear probe only — waiting for Skia/WASM RenderPlan." : probe.diagnostic}</p>
        </div>
      </div>
    </div>
  );
}
