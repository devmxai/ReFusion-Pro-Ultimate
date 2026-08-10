#include "refusion/core/ProjectCreation.hpp"
#include "refusion/core/ProjectRfx.hpp"

#include <cstdint>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>

namespace {

std::string response;

std::string json_string(const std::string_view value) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << '"';
  for (const unsigned char character : value) {
    switch (character) {
      case '"': output << "\\\""; break;
      case '\\': output << "\\\\"; break;
      case '\n': output << "\\n"; break;
      case '\r': output << "\\r"; break;
      case '\t': output << "\\t"; break;
      default:
        if (character < 0x20U) {
          output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                 << static_cast<unsigned int>(character) << std::dec;
        } else {
          output << static_cast<char>(character);
        }
    }
  }
  output << '"';
  return output.str();
}

const char* failure(const std::string_view code, const std::string_view message) {
  response = "{\"ok\":false,\"code\":" + json_string(code) +
             ",\"message\":" + json_string(message) + "}";
  return response.c_str();
}

std::string project_json(const refusion::core::ProjectSnapshot& project,
                         const std::string_view rfx) {
  const auto& composition = *project.composition;
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "{\"ok\":true,\"projectId\":"
         << json_string(project.project_id.value)
         << ",\"revision\":" << project.revision_id.value
         << ",\"projectName\":" << json_string(project.display_name)
         << ",\"compositionId\":"
         << json_string(composition.composition_id.value)
         << ",\"width\":" << composition.canvas.width_pixels
         << ",\"height\":" << composition.canvas.height_pixels
         << ",\"frameRate\":" << composition.frame_rate.numerator
         << ",\"durationSeconds\":"
         << (composition.duration / 1'000'000'000ULL)
         << ",\"rfx\":" << json_string(rfx) << '}';
  return output.str();
}

}  // namespace

extern "C" {

const char* rf_web_create_project(const char* display_name,
                                  const char* preset_id,
                                  const char* resolution_id,
                                  const std::uint32_t frame_rate,
                                  const std::uint32_t duration_seconds) {
  if (display_name == nullptr || preset_id == nullptr || resolution_id == nullptr) {
    return failure("RFX-WEB-ABI-001", "project creation received a null argument");
  }
  const auto result = refusion::core::create_initial_project(
      refusion::core::CreateProjectRequest{
          .display_name = display_name,
          .composition_preset_id = preset_id,
          .resolution_id = resolution_id,
          .frame_rate = frame_rate,
          .duration_seconds = duration_seconds,
      });
  if (!result.succeeded()) {
    return failure(result.code, result.message);
  }
  try {
    response = project_json(*result.project,
                            refusion::core::serialize_project_rfx(*result.project));
    return response.c_str();
  } catch (const std::exception& error) {
    return failure("RFX-WEB-SERIALIZE-001", error.what());
  }
}

const char* rf_web_open_project(const char* source) {
  if (source == nullptr) {
    return failure("RFX-WEB-ABI-002", "project open received a null source");
  }
  const auto compiled = refusion::core::compile_project_rfx(source);
  if (!compiled.succeeded()) {
    if (compiled.diagnostics.empty()) {
      return failure("RFX-WEB-RFX-001", "Project.rfx compilation failed");
    }
    return failure(compiled.diagnostics.front().code,
                   compiled.diagnostics.front().message);
  }
  try {
    response = project_json(*compiled.project,
                            refusion::core::serialize_project_rfx(*compiled.project));
    return response.c_str();
  } catch (const std::exception& error) {
    return failure("RFX-WEB-SERIALIZE-002", error.what());
  }
}

const char* rf_web_capabilities() {
  response = "{\"ok\":true,\"abi\":\"refusion-web-core-v1\",\"rfx\":5}";
  return response.c_str();
}

}
