#pragma once

/**
 * domain_tool_interface.h
 * Lightweight Tier 1 (`DomainTool`) contract and metadata helpers (`src/godot/tools/domain/`).
 *
 * Avoids multiple-inheritance friction with Godot's `GDCLASS` macro by providing:
 *   - Non-Object `DomainToolInterface` mixin for C++ DomainTool controls that want an explicit
 *     typed binding contract.
 *   - Static helpers in `domain_tool` for stamping `ideam_ui_selector`, `ideam_help_topic_id`,
 *     and `ideam_semantic_tags` metadata onto any `godot::Control` so `IdeamTutorials` and
 *     `ToolWorkspace` can discover and guide any DomainTool uniformly.
 */

#include "../tool_context.h"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace ideam::godot_ext {

class DomainToolInterface {
public:
    virtual ~DomainToolInterface() = default;

    virtual void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) = 0;

    [[nodiscard]] virtual godot::StringName get_domain_tool_id() const { return godot::StringName(); }
    [[nodiscard]] virtual godot::StringName get_domain_help_topic_id() const { return get_domain_tool_id(); }
    [[nodiscard]] virtual godot::PackedStringArray get_domain_semantic_tags() const {
        return godot::PackedStringArray();
    }
};

namespace domain_tool {

inline constexpr const char *META_UI_SELECTOR   = "ideam_ui_selector";
inline constexpr const char *META_HELP_TOPIC_ID = "ideam_help_topic_id";
inline constexpr const char *META_SEMANTIC_TAGS = "ideam_semantic_tags";

inline void stamp_ui_selector(godot::Control *p_control, const godot::String &p_selector) {
    if (p_control && !p_selector.is_empty()) {
        p_control->set_meta(META_UI_SELECTOR, p_selector);
    }
}

inline void attach_help_topic(godot::Control *p_control, const godot::StringName &p_topic_id) {
    if (p_control && p_topic_id != godot::StringName()) {
        p_control->set_meta(META_HELP_TOPIC_ID, godot::String(p_topic_id));
    }
}

inline void attach_semantic_tags(godot::Control *p_control, const godot::PackedStringArray &p_tags) {
    if (p_control && !p_tags.is_empty()) {
        p_control->set_meta(META_SEMANTIC_TAGS, p_tags);
    }
}

inline bool request_control_help(
    godot::Control *p_control,
    const godot::Ref<ToolContext> &p_context,
    const godot::StringName &p_fallback_tool_id = godot::StringName()
) {
    godot::StringName topic_id = p_fallback_tool_id;
    godot::String selector;
    if (p_control) {
        if (p_control->has_meta(META_HELP_TOPIC_ID)) {
            topic_id = godot::StringName(godot::String(p_control->get_meta(META_HELP_TOPIC_ID)));
        }
        if (p_control->has_meta(META_UI_SELECTOR)) {
            selector = godot::String(p_control->get_meta(META_UI_SELECTOR));
        }
    }
    if (p_context.is_valid()) {
        return p_context->request_help(topic_id, selector);
    }
    return false;
}

} // namespace domain_tool

} // namespace ideam::godot_ext
