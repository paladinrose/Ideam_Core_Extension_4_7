#pragma once

/**
 * pointer_state_horizon_visualizer.h
 * Tier 1 `DomainTool` control (`src/godot/quantum_darwinism/ui/`) for `IdeamQuantumDarwinism`.
 *
 * Implements `DomainToolInterface` and stamps `ideam_ui_selector`, `ideam_help_topic_id`,
 * and `ideam_semantic_tags` metadata for `IdeamTutorials` and `ToolRegistry`.
 *
 * Supports both `QuantumDarwinismGeneratorResource` (outward multi-channel wavefronts,
 * cross-channel couplings, spontaneous nucleation) and `QuantumDarwinismStabilizerResource`
 * (conflict horizon boxes & coherence bridges) inside `ProceduralComposer`.
 */

#include "../quantum_darwinism_generator_resource.h"
#include "../quantum_darwinism_stabilizer_resource.h"
#include "../../tools/domain/domain_tool_interface.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/item_list.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/tree.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace ideam::godot_ext {

class PointerStateHorizonVisualizer : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(PointerStateHorizonVisualizer, godot::VBoxContainer)

private:
    godot::Ref<QuantumDarwinismGeneratorResource> generator_res_;
    godot::Ref<QuantumDarwinismStabilizerResource> stabilizer_res_;
    godot::Ref<ToolContext> tool_context_;

    // Live preview solver substrate so designers can test wavefronts & nucleation directly in ProceduralComposer
    core::ProceduralEnvironment preview_env_;
    core::QuantumDarwinismSolver preview_solver_;
    bool preview_initialized_ = false;

    // UI Widgets
    godot::Label *header_label_          = nullptr;
    godot::Label *status_label_          = nullptr;
    godot::OptionButton *mode_option_    = nullptr;
    godot::Button *btn_run_              = nullptr;
    godot::Button *btn_step_             = nullptr;
    godot::Button *btn_reset_            = nullptr;
    godot::Button *btn_add_channel_      = nullptr;
    godot::Button *btn_add_coupling_     = nullptr;
    godot::Button *btn_help_             = nullptr;
    godot::ItemList *channel_list_       = nullptr;
    godot::ItemList *coupling_list_      = nullptr;
    godot::Tree *slot_wavefront_tree_    = nullptr;

    void _build_ui();
    void _ensure_preview_environment();
    void _refresh_ui();
    void _refresh_channel_and_coupling_lists();
    void _refresh_wavefront_tree();

protected:
    static void _bind_methods();

public:
    PointerStateHorizonVisualizer();
    ~PointerStateHorizonVisualizer() override = default;

    // --- DomainToolInterface ---
    void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;

    [[nodiscard]] godot::StringName get_domain_tool_id() const override {
        return godot::StringName("PointerStateHorizonVisualizer");
    }
    [[nodiscard]] godot::StringName get_domain_help_topic_id() const override {
        return godot::StringName("PointerStateHorizonVisualizer");
    }
    [[nodiscard]] godot::PackedStringArray get_domain_semantic_tags() const override;

    // --- UI Callbacks ---
    void _on_mode_selected(int p_index);
    void _on_run_pressed();
    void _on_step_pressed();
    void _on_reset_pressed();
    void _on_add_channel_pressed();
    void _on_add_coupling_pressed();
    void _on_help_pressed();
    void _on_resource_changed();
};

} // namespace ideam::godot_ext
