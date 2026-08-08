#include "AtomicProjectFileNative.hpp"

#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <system_error>

namespace refusion::cli {

AtomicReplaceResult native_replace_project_file(
    const std::filesystem::path& target,
    const std::filesystem::path& temporary,
    const std::string_view bytes) {
  const HANDLE handle = CreateFileW(
      temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    return {false, "RFX-AGENT-COMMIT-IO-002",
            "cannot create the atomic candidate file"};
  }
  std::size_t offset = 0;
  bool write_ok = true;
  while (offset < bytes.size()) {
    const auto remaining = bytes.size() - offset;
    const auto chunk = static_cast<DWORD>(
        remaining > 0x7fffffffU ? 0x7fffffffU : remaining);
    DWORD written = 0;
    if (!WriteFile(handle, bytes.data() + offset, chunk, &written, nullptr) ||
        written == 0) {
      write_ok = false;
      break;
    }
    offset += written;
  }
  const bool flush_ok = write_ok && FlushFileBuffers(handle) != 0;
  CloseHandle(handle);
  if (!flush_ok) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return {false, "RFX-AGENT-COMMIT-IO-003",
            "cannot flush the atomic candidate file"};
  }
  if (!MoveFileExW(temporary.c_str(), target.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return {false, "RFX-AGENT-COMMIT-IO-004",
            "cannot atomically replace Project.rfx"};
  }
  return {true, {}, {}};
}

}  // namespace refusion::cli
