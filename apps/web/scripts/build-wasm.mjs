import { mkdirSync, existsSync } from "node:fs";
import { resolve } from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const appRoot = fileURLToPath(new URL("..", import.meta.url));
const repoRoot = resolve(appRoot, "../..");
const outputDir = resolve(appRoot, "src/wasm");
const output = resolve(outputDir, "refusion_core.js");
const empp = process.env.EMXX ?? "em++";

mkdirSync(outputDir, { recursive: true });

const coreSources = [
  "AgentIntrospection.cpp",
  "CanonicalCoordinates.cpp",
  "CanonicalText.cpp",
  "ColorContract.cpp",
  "ContentDigest.cpp",
  "ProjectCreation.cpp",
  "ProjectDocument.cpp",
  "ProjectRfx.cpp",
  "ProjectAuthority.cpp",
  "ProjectClock.cpp",
  "SemanticAuthoring.cpp",
  "TextLayout.cpp",
  "VisualMeasurement.cpp",
  "VisualPropertyRegistry.cpp",
  "VisualContributionRegistry.cpp",
].map((source) => resolve(repoRoot, "src/core", source));

const bridge = resolve(appRoot, "wasm/refusion_web_bridge.cpp");
const args = [
  "-std=c++20",
  "-O2",
  "-fexceptions",
  "-I", resolve(repoRoot, "src/core/include"),
  ...coreSources,
  bridge,
  "--no-entry",
  "-sMODULARIZE=1",
  "-sEXPORT_ES6=1",
  "-sEXPORT_NAME=RefusionCore",
  "-sENVIRONMENT=web,worker",
  "-sALLOW_MEMORY_GROWTH=1",
  "-sFILESYSTEM=0",
  "-sEXIT_RUNTIME=0",
  "-sEXPORTED_FUNCTIONS=[\"_rf_web_create_project\",\"_rf_web_open_project\",\"_rf_web_capabilities\",\"_malloc\",\"_free\"]",
  "-sEXPORTED_RUNTIME_METHODS=[\"UTF8ToString\",\"lengthBytesUTF8\",\"stringToUTF8\"]",
  "-o", output,
];

const result = spawnSync(empp, args, { cwd: repoRoot, stdio: "inherit" });
if (result.error?.code === "ENOENT") {
  if (existsSync(output) && existsSync(output.replace(/\.js$/, ".wasm"))) {
    console.warn("Emscripten is unavailable; using the checked-in WebCore artifact.");
    process.exit(0);
  }
  console.error("Emscripten em++ is required to build the WebCore artifact.");
  process.exit(1);
}
if (result.status !== 0) process.exit(result.status ?? 1);
