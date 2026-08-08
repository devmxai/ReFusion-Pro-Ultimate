#include "refusion/core/VisualPropertyRegistry.hpp"

#include "refusion/core/CanonicalCoordinates.hpp"
#include "refusion/core/CanonicalText.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <locale>
#include <sstream>
#include <string_view>
#include <utility>

namespace refusion::core {
namespace {

struct ConstTarget final {
  const LayerSnapshot* layer{nullptr};
  const LayerGroupSnapshot* group{nullptr};
  VisualPropertyOwner owner{VisualPropertyOwner::shape};
};

struct MutableTarget final {
  LayerSnapshot* layer{nullptr};
  LayerGroupSnapshot* group{nullptr};
  VisualPropertyOwner owner{VisualPropertyOwner::shape};
};

using Getter = std::function<VisualPropertyValue(const ConstTarget&)>;
using Setter =
    std::function<bool(MutableTarget&, const VisualPropertyValue&)>;

struct RegistryEntry final {
  VisualPropertyDescriptor descriptor;
  Getter getter;
  Setter setter;
};

[[nodiscard]] const Transform2D& transform(const ConstTarget& target) {
  return target.group == nullptr ? target.layer->transform
                                 : target.group->transform;
}

[[nodiscard]] Transform2D& transform(MutableTarget& target) {
  return target.group == nullptr ? target.layer->transform
                                 : target.group->transform;
}

[[nodiscard]] bool supports(const VisualPropertyDescriptor& descriptor,
                            const VisualPropertyOwner owner) {
  return std::find(descriptor.owners.begin(), descriptor.owners.end(), owner) !=
         descriptor.owners.end();
}

[[nodiscard]] std::optional<ConstTarget> find_target(
    const CompositionSnapshot& composition,
    const VisualNodeRef& node) {
  if (const auto* layer_id = std::get_if<LayerId>(&node)) {
    const auto* layer = find_layer(composition, *layer_id);
    if (layer == nullptr) {
      return std::nullopt;
    }
    return ConstTarget{
        .layer = layer,
        .owner = std::holds_alternative<ShapeLayerContent>(layer->content)
                     ? VisualPropertyOwner::shape
                     : VisualPropertyOwner::text,
    };
  }
  const auto* group =
      find_layer_group(composition, std::get<LayerGroupId>(node));
  if (group == nullptr) {
    return std::nullopt;
  }
  return ConstTarget{
      .group = group,
      .owner = VisualPropertyOwner::group,
  };
}

[[nodiscard]] std::optional<MutableTarget> find_target(
    CompositionSnapshot& composition,
    const VisualNodeRef& node) {
  if (const auto* layer_id = std::get_if<LayerId>(&node)) {
    const auto found = std::find_if(
        composition.layers.begin(), composition.layers.end(),
        [layer_id](const LayerSnapshot& layer) {
          return layer.layer_id == *layer_id;
        });
    if (found == composition.layers.end()) {
      return std::nullopt;
    }
    return MutableTarget{
        .layer = &*found,
        .owner = std::holds_alternative<ShapeLayerContent>(found->content)
                     ? VisualPropertyOwner::shape
                     : VisualPropertyOwner::text,
    };
  }
  const auto group_id = std::get<LayerGroupId>(node);
  const auto found = std::find_if(
      composition.groups.begin(), composition.groups.end(),
      [&group_id](const LayerGroupSnapshot& group) {
        return group.group_id == group_id;
      });
  if (found == composition.groups.end()) {
    return std::nullopt;
  }
  return MutableTarget{
      .group = &*found,
      .owner = VisualPropertyOwner::group,
  };
}

[[nodiscard]] const std::vector<VisualPropertyOwner>& all_owners() {
  static const std::vector<VisualPropertyOwner> owners{
      VisualPropertyOwner::group,
      VisualPropertyOwner::shape,
      VisualPropertyOwner::text,
  };
  return owners;
}

[[nodiscard]] RegistryEntry number_transform_entry(
    std::string id,
    std::string display_name,
    std::string unit,
    double Transform2D::*member,
    std::optional<double> minimum = std::nullopt,
    std::optional<double> maximum = std::nullopt,
    const bool animatable = true,
    std::vector<VisualPropertyOwner> owners = all_owners()) {
  return RegistryEntry{
      .descriptor = VisualPropertyDescriptor{
          .id = VisualPropertyId{std::move(id)},
          .display_name = std::move(display_name),
          .value_kind = VisualPropertyValueKind::number,
          .unit = std::move(unit),
          .owners = std::move(owners),
          .minimum = minimum,
          .maximum = maximum,
          .animatable = animatable,
      },
      .getter = [member](const ConstTarget& target) {
        return VisualPropertyValue{transform(target).*member};
      },
      .setter = [member](MutableTarget& target,
                         const VisualPropertyValue& value) {
        const auto* number = std::get_if<double>(&value);
        if (number == nullptr) {
          return false;
        }
        transform(target).*member = *number;
        return true;
      },
  };
}

[[nodiscard]] const std::vector<RegistryEntry>& entries() {
  static const std::vector<RegistryEntry> registry = [] {
    std::vector<RegistryEntry> result;
    result.push_back(RegistryEntry{
        .descriptor = VisualPropertyDescriptor{
            .id = VisualPropertyId{"visual.name"},
            .display_name = "Name",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "text",
            .owners = all_owners(),
        },
        .getter = [](const ConstTarget& target) {
          return VisualPropertyValue{target.group == nullptr
                                         ? target.layer->display_name
                                         : target.group->display_name};
        },
        .setter = [](MutableTarget& target,
                     const VisualPropertyValue& value) {
          const auto* text = std::get_if<std::string>(&value);
          if (text == nullptr) {
            return false;
          }
          if (target.group == nullptr) {
            target.layer->display_name = *text;
          } else {
            target.group->display_name = *text;
          }
          return true;
        },
    });
    result.push_back(number_transform_entry(
        "transform.position.x", "Position X", "parent_px",
        &Transform2D::position_x));
    result.push_back(number_transform_entry(
        "transform.position.y", "Position Y", "parent_px",
        &Transform2D::position_y));
    result.push_back(number_transform_entry(
        "transform.anchor.x", "Anchor X", "local_px",
        &Transform2D::anchor_x, std::nullopt, std::nullopt, false));
    result.push_back(number_transform_entry(
        "transform.anchor.y", "Anchor Y", "local_px",
        &Transform2D::anchor_y, std::nullopt, std::nullopt, false));
    result.push_back(number_transform_entry(
        "transform.scale.x", "Scale X", "ratio", &Transform2D::scale_x,
        0.0));
    result.push_back(number_transform_entry(
        "transform.scale.y", "Scale Y", "ratio", &Transform2D::scale_y,
        0.0));
    result.push_back(number_transform_entry(
        "transform.rotation", "Rotation", "degrees",
        &Transform2D::rotation_degrees));
    result.push_back(number_transform_entry(
        "transform.opacity", "Opacity", "ratio", &Transform2D::opacity,
        0.0, 1.0, true,
        {VisualPropertyOwner::shape, VisualPropertyOwner::text}));
    result.push_back(RegistryEntry{
        .descriptor = VisualPropertyDescriptor{
            .id = VisualPropertyId{"compositing.blend_mode"},
            .display_name = "Blend Mode",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "enum(normal|multiply|screen|overlay)",
            .owners = {
                VisualPropertyOwner::shape,
                VisualPropertyOwner::text,
            },
        },
        .getter = [](const ConstTarget& target) {
          switch (target.layer->blend_mode) {
            case BlendMode::normal:
              return VisualPropertyValue{std::string{"normal"}};
            case BlendMode::multiply:
              return VisualPropertyValue{std::string{"multiply"}};
            case BlendMode::screen:
              return VisualPropertyValue{std::string{"screen"}};
            case BlendMode::overlay:
              return VisualPropertyValue{std::string{"overlay"}};
          }
          return VisualPropertyValue{std::string{"normal"}};
        },
        .setter = [](MutableTarget& target,
                     const VisualPropertyValue& value) {
          const auto* text = std::get_if<std::string>(&value);
          if (text == nullptr) {
            return false;
          }
          if (*text == "normal") {
            target.layer->blend_mode = BlendMode::normal;
          } else if (*text == "multiply") {
            target.layer->blend_mode = BlendMode::multiply;
          } else if (*text == "screen") {
            target.layer->blend_mode = BlendMode::screen;
          } else if (*text == "overlay") {
            target.layer->blend_mode = BlendMode::overlay;
          } else {
            return false;
          }
          return true;
        },
    });

    const std::vector<VisualPropertyOwner> shape_owner{
        VisualPropertyOwner::shape};
    const auto shape_number = [&result, &shape_owner](
                                  std::string id,
                                  std::string name,
                                  double ShapeLayerContent::*member,
                                  const double minimum) {
      result.push_back(RegistryEntry{
          .descriptor = VisualPropertyDescriptor{
              .id = VisualPropertyId{std::move(id)},
              .display_name = std::move(name),
              .value_kind = VisualPropertyValueKind::number,
              .unit = "composition_px",
              .owners = shape_owner,
              .minimum = minimum,
          },
          .getter = [member](const ConstTarget& target) {
            const auto& shape =
                std::get<ShapeLayerContent>(target.layer->content);
            return VisualPropertyValue{shape.*member};
          },
          .setter = [member](MutableTarget& target,
                             const VisualPropertyValue& value) {
            const auto* number = std::get_if<double>(&value);
            if (number == nullptr) {
              return false;
            }
            std::get<ShapeLayerContent>(target.layer->content).*member =
                *number;
            return true;
          },
      });
    };
    shape_number("shape.width", "Width", &ShapeLayerContent::width, 0.0);
    shape_number("shape.height", "Height", &ShapeLayerContent::height, 0.0);
    shape_number("shape.corner_radius", "Corner Radius",
                 &ShapeLayerContent::corner_radius, 0.0);
    shape_number("shape.stroke_width", "Border Width",
                 &ShapeLayerContent::stroke_width, 0.0);
    result.push_back(RegistryEntry{
        .descriptor = VisualPropertyDescriptor{
            .id = VisualPropertyId{"shape.fill"},
            .display_name = "Fill",
            .value_kind = VisualPropertyValueKind::shape_fill,
            .unit = "paint",
            .owners = shape_owner,
        },
        .getter = [](const ConstTarget& target) {
          return VisualPropertyValue{
              std::in_place_type<ShapeFill>,
              std::get<ShapeLayerContent>(target.layer->content).fill};
        },
        .setter = [](MutableTarget& target,
                     const VisualPropertyValue& value) {
          if (const auto* fill = std::get_if<ShapeFill>(&value)) {
            std::get<ShapeLayerContent>(target.layer->content).fill = *fill;
            return true;
          }
          if (const auto* color = std::get_if<ColorRgba8>(&value)) {
            std::get<ShapeLayerContent>(target.layer->content).fill = *color;
            return true;
          }
          return false;
        },
    });
    result.push_back(RegistryEntry{
        .descriptor = VisualPropertyDescriptor{
            .id = VisualPropertyId{"shape.stroke_color"},
            .display_name = "Border Color",
            .value_kind = VisualPropertyValueKind::color_rgba8,
            .unit = "rgba8",
            .owners = shape_owner,
        },
        .getter = [](const ConstTarget& target) {
          return VisualPropertyValue{
              std::get<ShapeLayerContent>(target.layer->content)
                  .stroke_color};
        },
        .setter = [](MutableTarget& target,
                     const VisualPropertyValue& value) {
          const auto* color = std::get_if<ColorRgba8>(&value);
          if (color == nullptr) {
            return false;
          }
          std::get<ShapeLayerContent>(target.layer->content).stroke_color =
              *color;
          return true;
        },
    });

    const std::vector<VisualPropertyOwner> text_owner{
        VisualPropertyOwner::text};
    const auto push_text = [&result, &text_owner](
                               VisualPropertyDescriptor descriptor,
                               Getter getter,
                               Setter setter) {
      descriptor.owners = text_owner;
      result.push_back(RegistryEntry{
          .descriptor = std::move(descriptor),
          .getter = std::move(getter),
          .setter = std::move(setter),
      });
    };
    const auto text_content = [](const ConstTarget& target)
        -> const TextLayerContent& {
      return std::get<TextLayerContent>(target.layer->content);
    };
    const auto mutable_text_content = [](MutableTarget& target)
        -> TextLayerContent& {
      return std::get<TextLayerContent>(target.layer->content);
    };
    const auto string_value = [](const VisualPropertyValue& value)
        -> const std::string* { return std::get_if<std::string>(&value); };
    const auto number_value = [](const VisualPropertyValue& value)
        -> const double* { return std::get_if<double>(&value); };

    push_text(
        VisualPropertyDescriptor{
            .id = VisualPropertyId{"text.value"},
            .display_name = "Text",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "text",
        },
        [text_content](const ConstTarget& target) {
          return VisualPropertyValue{text_content(target).text};
        },
        [mutable_text_content, string_value](MutableTarget& target,
                                             const VisualPropertyValue& value) {
          const auto* text = string_value(value);
          if (text == nullptr) {
            return false;
          }
          mutable_text_content(target).text = *text;
          return true;
        });
    push_text(
        VisualPropertyDescriptor{
            .id = VisualPropertyId{"text.font.source"},
            .display_name = "Font Source",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "enum(system_family|packaged_asset)",
            .writable = false,
        },
        [text_content](const ConstTarget& target) {
          return VisualPropertyValue{std::string{
              text_content(target).font.source == FontSourceKind::packaged_asset
                  ? "packaged_asset"
                  : "system_family"}};
        },
        [](MutableTarget&, const VisualPropertyValue&) { return false; });
    push_text(
        VisualPropertyDescriptor{
            .id = VisualPropertyId{"text.font.family"},
            .display_name = "Font Family",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "text",
        },
        [text_content](const ConstTarget& target) {
          return VisualPropertyValue{text_content(target).font.family_name};
        },
        [mutable_text_content, string_value](MutableTarget& target,
                                             const VisualPropertyValue& value) {
          const auto* family = string_value(value);
          if (family == nullptr) {
            return false;
          }
          mutable_text_content(target).font.family_name = *family;
          return true;
        });
    push_text(
        VisualPropertyDescriptor{
            .id = VisualPropertyId{"text.font.asset_id"},
            .display_name = "Font Asset ID",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "asset_id",
            .writable = false,
        },
        [text_content](const ConstTarget& target) {
          return VisualPropertyValue{text_content(target).font.asset_id};
        },
        [](MutableTarget&, const VisualPropertyValue&) { return false; });
    push_text(
        VisualPropertyDescriptor{
            .id = VisualPropertyId{"text.font.digest"},
            .display_name = "Font Digest",
            .value_kind = VisualPropertyValueKind::text,
            .unit = "sha256",
            .writable = false,
        },
        [text_content](const ConstTarget& target) {
          return VisualPropertyValue{text_content(target).font.content_digest};
        },
        [](MutableTarget&, const VisualPropertyValue&) { return false; });

    const auto push_number = [&push_text, text_content, mutable_text_content,
                              number_value](std::string id,
                                            std::string name,
                                            std::string unit,
                                            std::optional<double> minimum,
                                            std::optional<double> maximum,
                                            auto getter,
                                            auto setter) {
      push_text(
          VisualPropertyDescriptor{
              .id = VisualPropertyId{std::move(id)},
              .display_name = std::move(name),
              .value_kind = VisualPropertyValueKind::number,
              .unit = std::move(unit),
              .minimum = minimum,
              .maximum = maximum,
          },
          [text_content, getter](const ConstTarget& target) {
            return VisualPropertyValue{getter(text_content(target))};
          },
          [mutable_text_content, number_value, setter](
              MutableTarget& target, const VisualPropertyValue& value) {
            const auto* number = number_value(value);
            if (number == nullptr) {
              return false;
            }
            setter(mutable_text_content(target), *number);
            return true;
          });
    };
    push_number("text.font_size", "Font Size", "local_px", 0.0, 4096.0,
                [](const TextLayerContent& text) { return text.font_size; },
                [](TextLayerContent& text, const double value) {
                  text.font_size = value;
                });
    push_number("text.box.width", "TextBox Width", "local_px", 0.0,
                std::nullopt,
                [](const TextLayerContent& text) { return text.box.width; },
                [](TextLayerContent& text, const double value) {
                  text.box.width = value;
                });
    push_number("text.box.height", "TextBox Height", "local_px", 0.0,
                std::nullopt,
                [](const TextLayerContent& text) { return text.box.height; },
                [](TextLayerContent& text, const double value) {
                  text.box.height = value;
                });
    const auto padding = [&push_number](std::string id,
                                       std::string name,
                                       auto getter,
                                       auto setter) {
      push_number(std::move(id), std::move(name), "local_px", 0.0,
                  std::nullopt, std::move(getter), std::move(setter));
    };
    padding("text.box.padding.top", "Padding Top",
            [](const TextLayerContent& text) { return text.box.padding_top; },
            [](TextLayerContent& text, const double value) {
              text.box.padding_top = value;
            });
    padding("text.box.padding.right", "Padding Right",
            [](const TextLayerContent& text) { return text.box.padding_right; },
            [](TextLayerContent& text, const double value) {
              text.box.padding_right = value;
            });
    padding("text.box.padding.bottom", "Padding Bottom",
            [](const TextLayerContent& text) { return text.box.padding_bottom; },
            [](TextLayerContent& text, const double value) {
              text.box.padding_bottom = value;
            });
    padding("text.box.padding.left", "Padding Left",
            [](const TextLayerContent& text) { return text.box.padding_left; },
            [](TextLayerContent& text, const double value) {
              text.box.padding_left = value;
            });

    const auto push_enum = [&push_text, text_content, mutable_text_content,
                            string_value](std::string id,
                                          std::string name,
                                          std::string unit,
                                          auto getter,
                                          auto setter) {
      push_text(
          VisualPropertyDescriptor{
              .id = VisualPropertyId{std::move(id)},
              .display_name = std::move(name),
              .value_kind = VisualPropertyValueKind::text,
              .unit = std::move(unit),
          },
          [text_content, getter](const ConstTarget& target) {
            return VisualPropertyValue{getter(text_content(target))};
          },
          [mutable_text_content, string_value, setter](
              MutableTarget& target, const VisualPropertyValue& value) {
            const auto* text = string_value(value);
            return text != nullptr && setter(mutable_text_content(target), *text);
          });
    };
    push_enum(
        "text.direction", "Direction", "enum(ltr|rtl)",
        [](const TextLayerContent& text) {
          return std::string{text.direction == ParagraphDirection::left_to_right
                                 ? "ltr"
                                 : "rtl"};
        },
        [](TextLayerContent& text, const std::string& value) {
          if (value == "ltr") {
            text.direction = ParagraphDirection::left_to_right;
          } else if (value == "rtl") {
            text.direction = ParagraphDirection::right_to_left;
          } else {
            return false;
          }
          return true;
        });
    push_enum(
        "text.horizontal_alignment", "Horizontal Align",
        "enum(start|center|end|left|right)",
        [](const TextLayerContent& text) {
          switch (text.horizontal_alignment) {
            case TextHorizontalAlignment::start: return std::string{"start"};
            case TextHorizontalAlignment::center: return std::string{"center"};
            case TextHorizontalAlignment::end: return std::string{"end"};
            case TextHorizontalAlignment::left: return std::string{"left"};
            case TextHorizontalAlignment::right: return std::string{"right"};
          }
          return std::string{};
        },
        [](TextLayerContent& text, const std::string& value) {
          if (value == "start") text.horizontal_alignment = TextHorizontalAlignment::start;
          else if (value == "center") text.horizontal_alignment = TextHorizontalAlignment::center;
          else if (value == "end") text.horizontal_alignment = TextHorizontalAlignment::end;
          else if (value == "left") text.horizontal_alignment = TextHorizontalAlignment::left;
          else if (value == "right") text.horizontal_alignment = TextHorizontalAlignment::right;
          else return false;
          return true;
        });
    push_enum(
        "text.vertical_alignment", "Vertical Align", "enum(top|center|bottom)",
        [](const TextLayerContent& text) {
          switch (text.vertical_alignment) {
            case TextVerticalAlignment::top: return std::string{"top"};
            case TextVerticalAlignment::center: return std::string{"center"};
            case TextVerticalAlignment::bottom: return std::string{"bottom"};
          }
          return std::string{};
        },
        [](TextLayerContent& text, const std::string& value) {
          if (value == "top") text.vertical_alignment = TextVerticalAlignment::top;
          else if (value == "center") text.vertical_alignment = TextVerticalAlignment::center;
          else if (value == "bottom") text.vertical_alignment = TextVerticalAlignment::bottom;
          else return false;
          return true;
        });
    push_enum(
        "text.wrap", "Wrap", "enum(no_wrap|word)",
        [](const TextLayerContent& text) {
          return std::string{text.wrap == TextWrapMode::word ? "word" : "no_wrap"};
        },
        [](TextLayerContent& text, const std::string& value) {
          if (value == "no_wrap") text.wrap = TextWrapMode::no_wrap;
          else if (value == "word") text.wrap = TextWrapMode::word;
          else return false;
          return true;
        });
    push_enum(
        "text.overflow", "Overflow", "enum(clip|visible)",
        [](const TextLayerContent& text) {
          return std::string{text.overflow == TextOverflowMode::clip ? "clip" : "visible"};
        },
        [](TextLayerContent& text, const std::string& value) {
          if (value == "clip") text.overflow = TextOverflowMode::clip;
          else if (value == "visible") text.overflow = TextOverflowMode::visible;
          else return false;
          return true;
        });
    push_number("text.line_height", "Line Height", "ratio", 0.5, 10.0,
                [](const TextLayerContent& text) { return text.line_height_ratio; },
                [](TextLayerContent& text, const double value) {
                  text.line_height_ratio = value;
                });
    push_number("text.letter_spacing", "Letter Spacing", "local_px", -1024.0,
                1024.0,
                [](const TextLayerContent& text) { return text.letter_spacing; },
                [](TextLayerContent& text, const double value) {
                  text.letter_spacing = value;
                });
    result.push_back(RegistryEntry{
        .descriptor = VisualPropertyDescriptor{
            .id = VisualPropertyId{"text.fill"},
            .display_name = "Fill",
            .value_kind = VisualPropertyValueKind::color_rgba8,
            .unit = "rgba8",
            .owners = text_owner,
        },
        .getter = [](const ConstTarget& target) {
          return VisualPropertyValue{
              std::get<TextLayerContent>(target.layer->content).fill};
        },
        .setter = [](MutableTarget& target,
                     const VisualPropertyValue& value) {
          const auto* color = std::get_if<ColorRgba8>(&value);
          if (color == nullptr) {
            return false;
          }
          std::get<TextLayerContent>(target.layer->content).fill = *color;
          return true;
        },
    });
    return result;
  }();
  return registry;
}

[[nodiscard]] CompositionValidation rejected(std::string code,
                                             std::string message) {
  return CompositionValidation{
      .valid = false,
      .code = std::move(code),
      .message = std::move(message),
  };
}

[[nodiscard]] bool pixel_unit(const std::string_view unit) noexcept {
  return unit == "parent_px" || unit == "local_px" ||
         unit == "composition_px";
}

}  // namespace

const std::vector<VisualPropertyDescriptor>& visual_property_descriptors() {
  static const std::vector<VisualPropertyDescriptor> descriptors = [] {
    std::vector<VisualPropertyDescriptor> result;
    result.reserve(entries().size());
    for (const auto& entry : entries()) {
      result.push_back(entry.descriptor);
    }
    return result;
  }();
  return descriptors;
}

const std::string& visual_property_registry_digest() {
  static const std::string digest = [] {
    // FNV-1a is used as a deterministic schema fingerprint, not as a security
    // primitive. Font/media identities continue to require sha256.
    std::uint64_t hash = 14695981039346656037ULL;
    const auto append = [&hash](const std::string_view text) {
      for (const unsigned char byte : text) {
        hash ^= byte;
        hash *= 1099511628211ULL;
      }
      hash ^= 0xffU;
      hash *= 1099511628211ULL;
    };
    for (const auto& descriptor : visual_property_descriptors()) {
      append(descriptor.id.value);
      append(descriptor.display_name);
      append(canonical_uint64(static_cast<unsigned>(descriptor.value_kind)));
      append(descriptor.unit);
      for (const auto owner : descriptor.owners) {
        append(canonical_uint64(static_cast<unsigned>(owner)));
      }
      append(descriptor.minimum
                 ? canonical_fixed6_float64(*descriptor.minimum)
                 : "-");
      append(descriptor.maximum
                 ? canonical_fixed6_float64(*descriptor.maximum)
                 : "-");
      append(descriptor.animatable ? "animated" : "static");
      append(descriptor.writable ? "writable" : "readonly");
    }
    return "rfx-vp-fnv1a64:" + canonical_hex64(hash);
  }();
  return digest;
}

std::string visual_property_registry_markdown() {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "# Generated visual property registry\n\n"
         << "Digest: `" << visual_property_registry_digest() << "`\n\n"
         << "This file is generated from the Core descriptors consumed by "
            "Inspector and typed property commands. Do not edit it by hand.\n\n"
         << "| Property ID | Unit/domain | Writable | Animatable |\n"
         << "|---|---|---:|---:|\n";
  for (const auto& descriptor : visual_property_descriptors()) {
    output << "| `" << descriptor.id.value << "` | `" << descriptor.unit
           << "` | " << (descriptor.writable ? "yes" : "no") << " | "
           << (descriptor.animatable ? "yes" : "no") << " |\n";
  }
  return output.str();
}

std::vector<VisualPropertyRecord> inspect_visual_properties(
    const CompositionSnapshot& composition,
    const VisualNodeRef& node) {
  const auto target = find_target(composition, node);
  if (!target) {
    return {};
  }
  std::vector<VisualPropertyRecord> result;
  for (const auto& entry : entries()) {
    if (supports(entry.descriptor, target->owner)) {
      result.push_back(VisualPropertyRecord{
          .descriptor = entry.descriptor,
          .value = entry.getter(*target),
      });
    }
  }
  return result;
}

CompositionValidation set_visual_property(
    CompositionSnapshot& candidate,
    const VisualNodeRef& node,
    const VisualPropertyId& property_id,
    const VisualPropertyValue& value) {
  auto target = find_target(candidate, node);
  if (!target) {
    return rejected("RFX-VISUAL-NODE-404",
                    "visual property target does not exist");
  }
  const auto found = std::find_if(
      entries().begin(), entries().end(), [&property_id](const auto& entry) {
        return entry.descriptor.id == property_id;
      });
  if (found == entries().end()) {
    return rejected("RFX-PROPERTY-404",
                    "visual property descriptor does not exist");
  }
  if (!supports(found->descriptor, target->owner)) {
    return rejected("RFX-PROPERTY-OWNER-400",
                    "visual property is incompatible with the selected node");
  }
  if (!found->descriptor.writable) {
    return rejected("RFX-PROPERTY-READONLY-400",
                    "visual property is a read-only registry projection");
  }
  auto accepted_value = value;
  if (pixel_unit(found->descriptor.unit)) {
    if (const auto* number = std::get_if<double>(&value)) {
      accepted_value = quantize_authored_pixel(*number);
    }
  }
  if (!found->setter(*target, accepted_value)) {
    return rejected("RFX-PROPERTY-TYPE-400",
                    "visual property value has the wrong type");
  }
  return validate_composition(candidate);
}

}  // namespace refusion::core
