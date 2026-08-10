#include "refusion/runtime/media/HardwareVideoDecode.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace refusion::runtime::media {

bool HardwareDecodeRequest::valid() const noexcept {
  return !source_path.empty() && expected_profile.valid() &&
         packet_timing.valid();
}

bool CompressedSampleDescriptor::valid() const noexcept {
  return timing.valid() && decode_time.valid();
}

bool HardwareDecodeSequenceRequest::valid() const noexcept {
  constexpr std::size_t kMaximumBoundedSamples = 64;
  if (source_path.empty() || !expected_profile.valid() || samples.empty() ||
      samples.size() > kMaximumBoundedSamples) {
    return false;
  }
  for (std::size_t index = 0; index < samples.size(); ++index) {
    if (!samples[index].valid()) {
      return false;
    }
    if (index > 0 && samples[index - 1].access_unit_index >=
                         samples[index].access_unit_index) {
      return false;
    }
    for (std::size_t previous = 0; previous < index; ++previous) {
      if (samples[previous].source_frame_index ==
          samples[index].source_frame_index) {
        return false;
      }
    }
  }
  return true;
}

bool DecodedSurfaceInfo::valid() const noexcept {
  return lease_id != 0 && profile.valid() && timing.valid() &&
         device.adapter_id != 0 && device.generation != 0 && plane_count > 0;
}

namespace {

[[nodiscard]] std::uint64_t magnitude(const std::int64_t value) noexcept {
  return value >= 0 ? static_cast<std::uint64_t>(value)
                    : 0U - static_cast<std::uint64_t>(value);
}

[[nodiscard]] int compare_unsigned_fractions(std::uint64_t left_numerator,
                                             std::uint64_t left_denominator,
                                             std::uint64_t right_numerator,
                                             std::uint64_t right_denominator) {
  bool reverse = false;
  for (;;) {
    const auto left_quotient = left_numerator / left_denominator;
    const auto right_quotient = right_numerator / right_denominator;
    if (left_quotient != right_quotient) {
      const int result = left_quotient < right_quotient ? -1 : 1;
      return reverse ? -result : result;
    }

    const auto left_remainder = left_numerator % left_denominator;
    const auto right_remainder = right_numerator % right_denominator;
    if (left_remainder == 0 || right_remainder == 0) {
      if (left_remainder == right_remainder) {
        return 0;
      }
      const int result = left_remainder == 0 ? -1 : 1;
      return reverse ? -result : result;
    }

    left_numerator = left_denominator;
    left_denominator = left_remainder;
    right_numerator = right_denominator;
    right_denominator = right_remainder;
    reverse = !reverse;
  }
}

[[nodiscard]] bool profiles_match(const StrictDecodeProfile& left,
                                  const StrictDecodeProfile& right) noexcept {
  return left.codec == right.codec && left.pixel_format == right.pixel_format &&
         left.coded_width == right.coded_width &&
         left.coded_height == right.coded_height &&
         left.color.primaries == right.color.primaries &&
         left.color.transfer == right.color.transfer &&
         left.color.matrix == right.color.matrix &&
         left.color.full_range == right.color.full_range;
}

[[nodiscard]] bool devices_match(const gpu::DeviceIdentity& left,
                                 const gpu::DeviceIdentity& right) noexcept {
  return left.backend == right.backend && left.adapter_id == right.adapter_id &&
         left.generation == right.generation;
}

}  // namespace

std::strong_ordering compare_exact_media_time(const ExactMediaTime left,
                                              const ExactMediaTime right) {
  if (!left.valid() || !right.valid()) {
    throw std::invalid_argument("cannot compare invalid exact media time");
  }
  if (left.value < 0 && right.value >= 0) {
    return std::strong_ordering::less;
  }
  if (left.value >= 0 && right.value < 0) {
    return std::strong_ordering::greater;
  }

  int comparison = compare_unsigned_fractions(
      magnitude(left.value), static_cast<std::uint64_t>(left.timescale),
      magnitude(right.value), static_cast<std::uint64_t>(right.timescale));
  if (left.value < 0) {
    comparison = -comparison;
  }
  if (comparison < 0) {
    return std::strong_ordering::less;
  }
  if (comparison > 0) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

std::shared_ptr<const DecodedSurfaceQueue> DecodedSurfaceQueue::create(
    std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces) {
  return std::shared_ptr<const DecodedSurfaceQueue>(
      new DecodedSurfaceQueue(std::move(surfaces)));
}

DecodedSurfaceQueue::DecodedSurfaceQueue(
    std::vector<std::shared_ptr<const NativeVideoSurfaceLease>> surfaces)
    : surfaces_(std::move(surfaces)) {
  if (surfaces_.empty()) {
    throw std::invalid_argument("decoded surface queue cannot be empty");
  }
  for (const auto& surface : surfaces_) {
    if (!surface || !surface->info().valid()) {
      throw std::invalid_argument(
          "decoded surface queue contains an invalid lease");
    }
  }
  std::sort(surfaces_.begin(), surfaces_.end(),
            [](const auto& left, const auto& right) {
              return compare_exact_media_time(
                         left->info().timing.presentation_time,
                         right->info().timing.presentation_time) ==
                     std::strong_ordering::less;
            });

  device_identity_ = surfaces_.front()->info().device;
  const auto& profile = surfaces_.front()->info().profile;
  for (std::size_t index = 0; index < surfaces_.size(); ++index) {
    const auto& info = surfaces_[index]->info();
    if (!devices_match(device_identity_, info.device) ||
        !profiles_match(profile, info.profile)) {
      throw std::invalid_argument(
          "decoded surface queue crosses a device generation or media profile");
    }
    if (index > 0 &&
        compare_exact_media_time(
            surfaces_[index - 1]->info().timing.presentation_time,
            info.timing.presentation_time) != std::strong_ordering::less) {
      throw std::invalid_argument(
          "decoded surface queue presentation times must be unique");
    }
  }
}

std::size_t DecodedSurfaceQueue::size() const noexcept {
  return surfaces_.size();
}

bool DecodedSurfaceQueue::empty() const noexcept { return surfaces_.empty(); }

const gpu::DeviceIdentity& DecodedSurfaceQueue::device_identity()
    const noexcept {
  return device_identity_;
}

const std::shared_ptr<const NativeVideoSurfaceLease>&
DecodedSurfaceQueue::frame(const std::size_t index) const {
  return surfaces_.at(index);
}

std::shared_ptr<const NativeVideoSurfaceLease> DecodedSurfaceQueue::select_at(
    const ExactMediaTime source_time) const {
  if (!source_time.valid() || surfaces_.empty()) {
    return nullptr;
  }
  const auto iterator = std::upper_bound(
      surfaces_.begin(), surfaces_.end(), source_time,
      [](const ExactMediaTime time, const auto& surface) {
        return compare_exact_media_time(
                   time, surface->info().timing.presentation_time) ==
               std::strong_ordering::less;
      });
  if (iterator == surfaces_.begin()) {
    return nullptr;
  }
  return *std::prev(iterator);
}

bool HardwareDecodeResult::admitted() const noexcept {
  return state == DecodeState::decoded && hardware_decoder && surface &&
         surface->info().valid() && counters.strict_path_clean();
}

bool HardwareDecodeSequenceResult::admitted() const noexcept {
  return state == DecodeState::decoded && hardware_decoder && queue &&
         !queue->empty() && counters.strict_path_clean();
}

bool HardwareVideoPlaybackSource::valid() const noexcept {
  constexpr std::size_t kMaximumIndexedSamples = 4'000'000;
  if (source_path.empty() || source_byte_size == 0 ||
      !expected_profile.valid() || codec_configuration.empty() ||
      samples_decode_order.empty() ||
      samples_decode_order.size() > kMaximumIndexedSamples) {
    return false;
  }
  for (std::size_t index = 0; index < samples_decode_order.size(); ++index) {
    const auto& sample = samples_decode_order[index];
    if (!sample.valid() || sample.source_byte_size == 0 ||
        sample.source_byte_offset > source_byte_size ||
        sample.source_byte_size > source_byte_size - sample.source_byte_offset ||
        (index > 0 && samples_decode_order[index - 1].access_unit_index >=
                          sample.access_unit_index)) {
      return false;
    }
  }
  return true;
}

bool HardwareVideoPlaybackWindowRequest::valid() const noexcept {
  constexpr std::size_t kMaximumResidentSurfaces = 16;
  return target_presentation_time.valid() && maximum_surface_count > 0 &&
         maximum_surface_count <= kMaximumResidentSurfaces &&
         lookahead_surface_count < maximum_surface_count;
}

bool HardwareVideoPlaybackWindowResult::admitted() const noexcept {
  return state == DecodeState::decoded && hardware_decoder && queue &&
         !queue->empty() && counters.strict_path_clean();
}

}  // namespace refusion::runtime::media
