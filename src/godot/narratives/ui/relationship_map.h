#pragma once

/**
 * relationship_map.h
 * Tier 1 DomainTool (`src/godot/narratives/ui/`): Interactive visual graph for
 * exploring and editing Character relationships across a single Character or an
 * entire `Narrative` (supporting multi-character branching graphs).
 *
 * Features:
 *   - Focal filter: view all Characters in a `Narrative` simultaneously OR focus on 1 Character
 *   - Color-coded nodes by Narreme type (Character, Prop, Location, Incident, Plot, Narrative)
 *   - Directed labeled edges showing `Relationship::title` (and semantic tag counts)
 *   - Inline relationship editor routed through `Character::begin_relationship` /
 *     `Character::end_relationship_at` (object-owned `IdeamUndoRedo`)
 *   - `DomainToolInterface` + `IdeamTutorials` selectors & `?` Help hook
 */

#include "../helpers/relationship.h"
#include "../narreme.h"
#include "../narremes/character.h"
#include "../narremes/narrative.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ideam::godot_ext {

class RelationshipMap : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(RelationshipMap, godot::VBoxContainer)

private:
    struct GraphNode {
        godot::Ref<Narreme> narreme;
        godot::Vector2 center;
        bool is_focal_character = false;
    };

    struct GraphEdge {
        size_t from_idx = 0;
        size_t to_idx   = 0;
        int rel_index   = -1;
        godot::Ref<Character> owner;
        godot::Ref<Relationship> relationship;
    };

    godot::Ref<Narrative> narrative_;
    godot::Ref<Character> focal_character_;
    godot::Ref<ToolContext> tool_context_;

    std::vector<GraphNode> nodes_;
    std::vector<GraphEdge> edges_;
    std::unordered_map<uint64_t, godot::Vector2> saved_positions_;
    std::vector<godot::Ref<Character>> character_choices_;
    std::vector<godot::Ref<Narreme>> target_choices_;

    int selected_node_idx_ = -1;
    int dragging_idx_      = -1;
    godot::Vector2 drag_offset_;

    godot::OptionButton *focal_opt_      = nullptr;
    godot::OptionButton *source_char_opt_= nullptr;
    godot::LineEdit *rel_title_edit_     = nullptr;
    godot::OptionButton *target_nar_opt_ = nullptr;
    godot::Control *canvas_              = nullptr;
    godot::Label *status_lbl_            = nullptr;

    void _build_ui();
    void _rebuild_graph_data();
    void _apply_radial_layout();
    void _refresh_pickers();
    [[nodiscard]] int _hit_test_node(const godot::Vector2 &p_pos) const;

protected:
    static void _bind_methods();

public:
    RelationshipMap();
    ~RelationshipMap() override = default;

    // --- DomainToolInterface ---
    virtual void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;
    [[nodiscard]] virtual godot::StringName get_domain_tool_id() const override {
        return godot::StringName("RelationshipMap");
    }
    [[nodiscard]] virtual godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_narrative(const godot::Ref<Narrative> &p_narrative);
    [[nodiscard]] godot::Ref<Narrative> get_narrative() const { return narrative_; }

    void set_focal_character(const godot::Ref<Character> &p_character);
    [[nodiscard]] godot::Ref<Character> get_focal_character() const { return focal_character_; }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void refresh_map();
    void auto_layout();

    // --- Signal Handlers ---
    void _on_canvas_draw();
    void _on_canvas_gui_input(const godot::Ref<godot::InputEvent> &p_event);
    void _on_focal_selected(int p_index);
    void _on_add_relationship_pressed();
    void _on_end_relationship_pressed();
    void _on_auto_layout_pressed();
    void _on_help_pressed();
    void _on_narrative_changed();
};

} // namespace ideam::godot_ext
