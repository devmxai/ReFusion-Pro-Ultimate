#include "AtomicProjectFileNative.hpp"

#include <cstdio>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

namespace refusion::cli {

AtomicReplaceResult native_replace_project_file(
    const std::filesystem::path& target,
    const std::filesystem::path& temporary,
    const std::string_view bytes) {
  const int descriptor =
      ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
  if (descriptor < 0) {
    return {false, "RFX-AGENT-COMMIT-IO-002",
            "cannot create the atomic candidate file"};
  }
  std::size_t offset = 0;
  bool write_ok = true;
  while (offset < bytes.size()) {
    const auto written =
        ::write(descriptor, bytes.data() + offset, bytes.size() - offset);
    if (written <= 0) {
      write_ok = false;
      break;
    }
    offset += static_cast<std::size_t>(written);
  }
  const bool flush_ok = write_ok && ::fsync(descriptor) == 0;
  ::close(descriptor);
  if (!flush_ok) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return {false, "RFX-AGENT-COMMIT-IO-003",
            "cannot flush the atomic candidate file"};
  }
  if (::rename(temporary.c_str(), target.c_str()) != 0) {
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);
    return {false, "RFX-AGENT-COMMIT-IO-004",
            "cannot atomically replace Project.rfx"};
  }
  const auto directory_path =
      target.parent_path().empty() ? std::filesystem::path{"."}
                                   : target.parent_path();
  const int directory = ::open(directory_path.c_str(), O_RDONLY);
  if (directory >= 0) {
    (void)::fsync(directory);
    ::close(directory);
  }
  return {true, {}, {}};
}

}  // namespace refusion::cli
