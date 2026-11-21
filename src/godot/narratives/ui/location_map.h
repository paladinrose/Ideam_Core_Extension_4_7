#pragma once

/**
 * location_map.h
 * Tier 1 DomainTool (`src/godot/narratives/ui/`): Interactive directed-graph topology
 * visualizer and editor for a group of `Location` narremes.
 *
 * Features:
 *   - Draggable Location cards with live character/prop counts
 *   - Directed edges: amber single-arrow for one-way streets, cyan dual-arrow for bidirectional
 *   - Live BFS shortest-path visualization (`Location::find_path_to`) with hop-count readout
 *   - All topology edits route through `Location::connect_location` / `disconnect_location`
 *     (object-owned `IdeamUndoRedo`)
 *   - `DomainToolInterface` + `IdeamTutorials` selectors & `?` Help hook
 */

#include "../narremes/location.h"
#include "../narremes/narrative.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_context.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/option_button.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ideam::godot_ext {

class LocationMap : public godot::VBoxContainer, public DomainToolInterface {
    GDCLASS(LocationMap, godot::VBoxContainer)

private:
    struct NodeLayout {
        godot::Ref<Location> location;
        godot::Vector2 center;
        bool is_symbolic = false;
    };

    godot::Ref<Narrative> narrative_;
    godot::TypedArray<Location> explicit_locations_;
    godot::Ref<ToolContext> tool_context_;

    std::vector<NodeLayout> nodes_;
    std::unordered_map<uint64_t, godot::Vector2> saved_positions_;
    std::vector<uint64_t> highlighted_path_ids_;

    int selected_source_idx_ = -1;
    int selected_target_idx_ = -1;
    int dragging_idx_        = -1;
    godot::Vector2 drag_offset_;

    godot::OptionButton *source_opt_ = nullptr;
    godot::OptionButton *target_opt_ = nullptr;
    godot::Control *canvas_          = nullptr;
    godot::Label *status_lbl_        = nullptr;

    void _build_ui();
    void _sync_nodes_from_sources();
    void _apply_circular_layout();
    void _refresh_dropdowns();
    [[nodiscard]] int _hit_test_node(const godot::Vector2 &p_pos) const;
    [[nodiscard]] bool _is_edge_on_highlighted_path(uint64_t p_from_id, uint64_t p_to_id) const;

protected:
    static void _bind_methods();

public:
    LocationMap();
    ~LocationMap() override = default;

    // --- DomainToolInterface ---
    virtual void bind_domain_asset(
        const godot::Ref<godot::Resource> &p_asset,
        const godot::Ref<ToolContext> &p_context
    ) override;
    [[nodiscard]] virtual godot::StringName get_domain_tool_id() const override {
        return godot::StringName("LocationMap");
    }
    [[nodiscard]] virtual godot::PackedStringArray get_domain_semantic_tags() const override;

    void set_narrative(const godot::Ref<Narrative> &p_narrative);
    [[nodiscard]] godot::Ref<Narrative> get_narrative() const { return narrative_; }

    void set_locations(const godot::TypedArray<Location> &p_locations);
    [[nodiscard]] godot::TypedArray<Location> get_locations() const { return explicit_locations_; }

    void set_tool_context(const godot::Ref<ToolContext> &p_context);
    [[nodiscard]] godot::Ref<ToolContext> get_tool_context() const { return tool_context_; }

    void refresh_map();
    void auto_layout();

    // --- Signal Handlers ---
    void _on_canvas_draw();
    void _on_canvas_gui_input(const godot::Ref<godot::InputEvent> &p_event);
    void _on_source_selected(int p_index);
    void _on_target_selected(int p_index);
    void _on_connect_one_way_pressed();
    void _on_connect_two_way_pressed();
    void _on_disconnect_pressed();
    void _on_find_path_pressed();
    void _on_clear_path_pressed();
    void _on_auto_layout_pressed();
    void _on_help_pressed();
    void _on_narrative_changed();
};

} // namespace ideam::godot_ext
