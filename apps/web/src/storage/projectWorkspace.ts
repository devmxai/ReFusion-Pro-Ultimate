export type WritableFile = {
  getFile(): Promise<File>;
  createWritable(): Promise<{ write(data: string | Blob | ArrayBuffer): Promise<void>; close(): Promise<void> }>;
};

export type WritableDirectory = {
  name: string;
  getDirectoryHandle(name: string, options?: { create?: boolean }): Promise<WritableDirectory>;
  getFileHandle(name: string, options?: { create?: boolean }): Promise<WritableFile>;
  queryPermission?: (options?: { mode?: "read" | "readwrite" }) => Promise<PermissionState>;
  requestPermission?: (options?: { mode?: "read" | "readwrite" }) => Promise<PermissionState>;
};

export type ProjectWorkspace = {
  directory: WritableDirectory;
  kind: "desktop-folder" | "browser-opfs";
  label: string;
  writable: boolean;
};

type DirectoryPickerWindow = Window & {
  showDirectoryPicker?: (options?: { mode?: "read" | "readwrite" }) => Promise<WritableDirectory>;
};

const projectsRootName = "ReFusion Web Projects";

export function supportsDesktopFolderPicker(): boolean {
  return typeof (window as DirectoryPickerWindow).showDirectoryPicker === "function";
}

async function ensureWritable(directory: WritableDirectory): Promise<void> {
  if (!directory.queryPermission || !directory.requestPermission) return;
  const options = { mode: "readwrite" as const };
  const current = await directory.queryPermission(options);
  if (current === "granted") return;
  const requested = await directory.requestPermission(options);
  if (requested !== "granted") {
    throw new Error("RFX-WEB-STORAGE-003: write permission was not granted");
  }
}

export async function chooseProjectWorkspace(): Promise<ProjectWorkspace> {
  const picker = (window as DirectoryPickerWindow).showDirectoryPicker;
  if (picker) {
    const directory = await picker({ mode: "readwrite" });
    await ensureWritable(directory);
    return {
      directory,
      kind: "desktop-folder",
      label: directory.name || "Desktop project folder",
      writable: true,
    };
  }

  // Safari and other browsers without the File System Access API receive a
  // durable, origin-private workspace. The UI states this explicitly; no
  // browser URL or opaque handle enters Project.rfx.
  if (!navigator.storage || typeof navigator.storage.getDirectory !== "function") {
    throw new Error("RFX-WEB-STORAGE-004: this browser cannot grant a writable folder or OPFS workspace; use Chrome, Edge or Brave for the desktop-folder flow.");
  }
  const opfsRoot = (await navigator.storage.getDirectory()) as unknown as WritableDirectory;
  const projects = await opfsRoot.getDirectoryHandle(projectsRootName, { create: true });
  const workspaceName = `project-${Date.now().toString(36)}`;
  const directory = await projects.getDirectoryHandle(workspaceName, { create: true });
  return {
    directory,
    kind: "browser-opfs",
    label: `Browser workspace / ${workspaceName}`,
    writable: true,
  };
}

export async function readText(directory: WritableDirectory, name: string): Promise<string> {
  const file = await (await directory.getFileHandle(name)).getFile();
  return file.text();
}

export async function writeText(
  directory: WritableDirectory,
  name: string,
  contents: string,
): Promise<void> {
  const file = await directory.getFileHandle(name, { create: true });
  const writable = await file.createWritable();
  await writable.write(contents);
  await writable.close();
}

export async function ensureMediaDirectory(directory: WritableDirectory): Promise<WritableDirectory> {
  const media = await directory.getDirectoryHandle("Media", { create: true });
  return media.getDirectoryHandle("Originals", { create: true });
}

export async function persistProject(
  workspace: ProjectWorkspace,
  projectRfx: string,
  metadata: Record<string, unknown>,
): Promise<void> {
  await writeText(workspace.directory, "Project.rfx", projectRfx);
  await writeText(
    workspace.directory,
    "refusion-web.json",
    `${JSON.stringify({ schema: "refusion-web-workspace-v1", ...metadata }, null, 2)}\n`,
  );
  await ensureMediaDirectory(workspace.directory);
}

export async function persistVideoAsset(
  workspace: ProjectWorkspace,
  file: File,
  assetId: string,
  digest: string,
): Promise<{ storedName: string; mediaPath: string }> {
  const originals = await ensureMediaDirectory(workspace.directory);
  const storedName = `${assetId}-${file.name.replace(/[^a-zA-Z0-9._-]+/g, "_")}`;
  const destination = await originals.getFileHandle(storedName, { create: true });
  const writable = await destination.createWritable();
  await writable.write(await file.arrayBuffer());
  await writable.close();
  await writeText(
    workspace.directory,
    "media-index.json",
    `${JSON.stringify({
      schema: "refusion-web-media-index-v1",
      assetId,
      digest,
      originalName: file.name,
      mediaPath: `Media/Originals/${storedName}`,
      byteLength: file.size,
      mimeType: file.type || "application/octet-stream",
    }, null, 2)}\n`,
  );
  return { storedName, mediaPath: `Media/Originals/${storedName}` };
}

export async function digestFile(file: File): Promise<string> {
  const bytes = await file.arrayBuffer();
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest), (byte) => byte.toString(16).padStart(2, "0")).join("");
}
