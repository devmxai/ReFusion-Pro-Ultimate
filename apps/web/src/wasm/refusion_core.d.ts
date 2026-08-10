type RefusionWebCore = {
  _rf_web_create_project: (displayName: number, presetId: number, resolutionId: number, frameRate: number, durationSeconds: number) => number;
  _rf_web_open_project: (source: number) => number;
  _rf_web_capabilities: () => number;
  _malloc: (size: number) => number;
  _free: (pointer: number) => void;
  UTF8ToString: (pointer: number) => string;
  lengthBytesUTF8: (value: string) => number;
  stringToUTF8: (value: string, pointer: number, maxBytes: number) => void;
};

declare const createCore: (options?: { locateFile?: (path: string) => string }) => Promise<RefusionWebCore>;
export default createCore;
