#pragma once

/**
 * narrative_condition_field.h
 * Tier 1 DomainTool (`src/godot/narratives/ui/`): Single-line malleable sentence editor
 * for `Narrative_Condition`.
 *
 * Renders `[Subject] [Condition Fragment] [Optional Object]` with:
 *   - Subject & Object popup pickers scoped by `NarremeFilterMode` (All / Literal / Symbolic)
 *   - Dynamic predicate population from `Narreme::get_narrative_condition_descriptors()`
 *   - Language-agnostic semantic frame badge (`frame_name`, `canonical_key`, `seme_tags`)
 *   - Object-owned `IdeamUndoRedo` commits via `Narrative_Condition::commit_configuration()`
 *   - `DomainToolInterface` + `IdeamTutorials` selector & `?` Help hooks
 */

#include "../helpers/narrative_condition.h"
#include "../narreme.h"
#include "../narremes/narrative.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <vector>

namespace ideam::godot_ext {

class NarrativeConditionField : public godot::HBoxContainer, public DomainToolInterface {
    GDCLASS(NarrativeConditionField, godot::HBoxContainer)

private:
    static constexpr int POPUP_FILTER_ALL      = 9001;
    static constexpr int POPUP_FILTER_LITERAL  = 9002;
    static constexpr int POPUP_FILTER_SYMBOLIC = 9003;
    static constexpr int POPUP_CLEAR_SELECTION = 9004;

    godot::Ref<Narrative_Condition> condition_;
    godot::Ref<Narrative> narrative_;
    godot::Ref<ToolContext> tool_context_;

    NarremeFilterMode subject_filter_mode_ = FILTER_ALL;
    NarremeFilterMode object_filter_mode_  = FILTER_ALL;
    bool updating_ui_                      = false;

    godot::Button *subject_btn_         = nullptr;
    godot::OptionButton *predicate_opt_ = nullptr;
    godot::Button *object_btn_          = nullptr;
    godot::Button *clear_object_btn_    = nullptr;
    godot::Label *semantic_badge_lbl_   = nullptr;
    godot::Button *help_btn_            = nullptr;

    godot::PopupMenu *subject_popup_ = nullptr;
    godot::PopupMenu *object_popup_  = nullptr;

    std::vector<godot::Ref<Narreme>> subject_candidates_;
    std::vector<godot::Ref<Narreme>> object_candidates_;
    godot::TypedArray<godot::Dictionary> active_descriptors_;

    void _rebuild_ui();
    void _populate_picker_popup(
        godot::PopupMenu *p_popup,
        NarremeFilterMode p_filter,
        bool p_allow_clear,
        std::vector<godot::Ref<Narreme>> &r_candidates
    );
    void _refresh_predicate_options();
    void _refresh_semantic_badge();
    [[nodiscard]] godot::String _format_narreme_label(const godot::Ref<Narreme> &p_narreme) const;

protected:
    static void _bind_methods();

public:
    NarrativeConditionField();
    ~NarrativeConditionField() override = default;

    // --- DomainToolInterface ---
    virtual void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;
    [[nodiscard]] virtual godot::StringName get_domain_tool_id() const override {
        return godot::StringName("NarrativeConditionField");
    }
    [[nodiscard]] virtual godot::PackedStringArray get_domain_semantic_tags() const override;

    // --- Public Binding API ---
    void set_narrative(const godot::Ref<Narrative> &p_narrative);
    [[nodiscard]] godot::Ref<Narrative> get_narrative() const { return narrative_; }

    void set_condition(const godot::Ref<Narrative_Condition> &p_condition);
    [[nodiscard]] godot::Ref<Narrative_Condition> get_condition() const { return condition_; }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void set_subject_filter_mode(NarremeFilterMode p_mode);
    [[nodiscard]] NarremeFilterMode get_subject_filter_mode() const noexcept { return subject_filter_mode_; }

    void set_object_filter_mode(NarremeFilterMode p_mode);
    [[nodiscard]] NarremeFilterMode get_object_filter_mode() const noexcept { return object_filter_mode_; }

    void refresh_display();

    // --- Signal Handlers ---
    void _on_subject_btn_pressed();
    void _on_object_btn_pressed();
    void _on_clear_object_pressed();
    void _on_subject_popup_id_pressed(int p_id);
    void _on_object_popup_id_pressed(int p_id);
    void _on_predicate_selected(int p_index);
    void _on_help_pressed();
    void _on_condition_resource_changed();
};

} // namespace ideam::godot_ext
