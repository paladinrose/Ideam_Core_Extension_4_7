#pragma once

/**
 * tutorial_resource.h
 * Data Layer resource (`godot::Resource`) holding a complete guided tutorial (`src/godot/tutorials/resources/`).
 */

#include "tutorial_step_resource.h"
#include "../../../core/tutorials/tutorial_common.h"
#include "../../graphs/ideam_graph_resource.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

enum TutorialTargetContext : int32_t {
    CONTEXT_EDITOR  = static_cast<int32_t>(core::tutorials::TutorialTargetContext::CONTEXT_EDITOR),
    CONTEXT_RUNTIME = static_cast<int32_t>(core::tutorials::TutorialTargetContext::CONTEXT_RUNTIME),
    CONTEXT_BOTH    = static_cast<int32_t>(core::tutorials::TutorialTargetContext::CONTEXT_BOTH)
};

class TutorialResource : public godot::Resource {
    GDCLASS(TutorialResource, godot::Resource)

private:
    godot::StringName tutorial_id_;
    godot::String title_;
    godot::String description_;
    godot::TypedArray<TutorialStepResource> steps_;
    godot::PackedStringArray required_active_plugins_;
    TutorialTargetContext target_context_ = CONTEXT_BOTH;
    godot::Ref<IdeamGraphResource> prerequisite_graph_;
    godot::PackedStringArray help_topic_ids_;
    godot::PackedStringArray semantic_tags_;

protected:
    static void _bind_methods();

public:
    TutorialResource() = default;
    ~TutorialResource() override = default;

    void set_tutorial_id(const godot::StringName &p_id) { tutorial_id_ = p_id; }
    [[nodiscard]] godot::StringName get_tutorial_id() const { return tutorial_id_; }

    void set_title(const godot::String &p_title) { title_ = p_title; }
    [[nodiscard]] godot::String get_title() const { return title_; }

    void set_description(const godot::String &p_desc) { description_ = p_desc; }
    [[nodiscard]] godot::String get_description() const { return description_; }

    void set_steps(const godot::TypedArray<TutorialStepResource> &p_steps) { steps_ = p_steps; }
    [[nodiscard]] godot::TypedArray<TutorialStepResource> get_steps() const { return steps_; }
    void add_step(const godot::Ref<TutorialStepResource> &p_step);

    void set_required_active_plugins(const godot::PackedStringArray &p_plugins) { required_active_plugins_ = p_plugins; }
    [[nodiscard]] godot::PackedStringArray get_required_active_plugins() const { return required_active_plugins_; }

    void set_target_context(TutorialTargetContext p_ctx) { target_context_ = p_ctx; }
    [[nodiscard]] TutorialTargetContext get_target_context() const noexcept { return target_context_; }

    void set_prerequisite_graph(const godot::Ref<IdeamGraphResource> &p_graph) { prerequisite_graph_ = p_graph; }
    [[nodiscard]] godot::Ref<IdeamGraphResource> get_prerequisite_graph() const { return prerequisite_graph_; }

    void set_help_topic_ids(const godot::PackedStringArray &p_topics) { help_topic_ids_ = p_topics; }
    [[nodiscard]] godot::PackedStringArray get_help_topic_ids() const { return help_topic_ids_; }

    void set_semantic_tags(const godot::PackedStringArray &p_tags) { semantic_tags_ = p_tags; }
    [[nodiscard]] godot::PackedStringArray get_semantic_tags() const { return semantic_tags_; }
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::TutorialTargetContext);
