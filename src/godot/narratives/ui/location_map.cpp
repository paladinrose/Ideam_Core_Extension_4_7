#include "location_map.h"

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/theme_db.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <cmath>

using namespace godot;

namespace ideam::godot_ext {

namespace {
constexpr float LOC_NODE_W = 148.0f;
constexpr float LOC_NODE_H = 54.0f;
} // namespace

void LocationMap::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_narrative", "narrative"), &LocationMap::set_narrative);
    ClassDB::bind_method(D_METHOD("get_narrative"), &LocationMap::get_narrative);
    ClassDB::bind_method(D_METHOD("set_locations", "locations"), &LocationMap::set_locations);
    ClassDB::bind_method(D_METHOD("get_locations"), &LocationMap::get_locations);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &LocationMap::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &LocationMap::get_tool_context);
    ClassDB::bind_method(D_METHOD("refresh_map"), &LocationMap::refresh_map);
    ClassDB::bind_method(D_METHOD("auto_layout"), &LocationMap::auto_layout);

    ClassDB::bind_method(D_METHOD("_on_canvas_draw"), &LocationMap::_on_canvas_draw);
    ClassDB::bind_method(D_METHOD("_on_canvas_gui_input", "event"), &LocationMap::_on_canvas_gui_input);
    ClassDB::bind_method(D_METHOD("_on_source_selected", "index"), &LocationMap::_on_source_selected);
    ClassDB::bind_method(D_METHOD("_on_target_selected", "index"), &LocationMap::_on_target_selected);
    ClassDB::bind_method(D_METHOD("_on_connect_one_way_pressed"), &LocationMap::_on_connect_one_way_pressed);
    ClassDB::bind_method(D_METHOD("_on_connect_two_way_pressed"), &LocationMap::_on_connect_two_way_pressed);
    ClassDB::bind_method(D_METHOD("_on_disconnect_pressed"), &LocationMap::_on_disconnect_pressed);
    ClassDB::bind_method(D_METHOD("_on_find_path_pressed"), &LocationMap::_on_find_path_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_path_pressed"), &LocationMap::_on_clear_path_pressed);
    ClassDB::bind_method(D_METHOD("_on_auto_layout_pressed"), &LocationMap::_on_auto_layout_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &LocationMap::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_narrative_changed"), &LocationMap::_on_narrative_changed);

    ADD_SIGNAL(MethodInfo("location_selected", PropertyInfo(Variant::OBJECT, "location", PROPERTY_HINT_RESOURCE_TYPE, "Location")));
}

LocationMap::LocationMap() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);
    domain_tool::stamp_ui_selector(this, "narratives.location_map");
    domain_tool::attach_help_topic(this, StringName("LocationMap"));
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());
    _build_ui();
}

PackedStringArray LocationMap::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("narrative");
    tags.append("location");
    tags.append("topology");
    tags.append("graph");
    tags.append("pathfinding");
    return tags;
}

void LocationMap::_build_ui() {
    HBoxContainer *toolbar = memnew(HBoxContainer);
    toolbar->add_theme_constant_override("separation", 6);
    add_child(toolbar);

    Label *from_lbl = memnew(Label);
    from_lbl->set_text("From:");
    toolbar->add_child(from_lbl);

    source_opt_ = memnew(OptionButton);
    source_opt_->set_custom_minimum_size(Vector2(130, 0));
    domain_tool::stamp_ui_selector(source_opt_, "narratives.location_map.source");
    source_opt_->connect("item_selected", Callable(this, "_on_source_selected"));
    toolbar->add_child(source_opt_);

    Label *to_lbl = memnew(Label);
    to_lbl->set_text("To:");
    toolbar->add_child(to_lbl);

    target_opt_ = memnew(OptionButton);
    target_opt_->set_custom_minimum_size(Vector2(130, 0));
    domain_tool::stamp_ui_selector(target_opt_, "narratives.location_map.target");
    target_opt_->connect("item_selected", Callable(this, "_on_target_selected"));
    toolbar->add_child(target_opt_);

    auto add_btn = [&](const char *p_text, const char *p_tip, const char *p_sel, const char *p_slot) {
        Button *b = memnew(Button);
        b->set_text(p_text);
        b->set_tooltip_text(p_tip);
        domain_tool::stamp_ui_selector(b, p_sel);
        b->connect("pressed", Callable(this, p_slot));
        toolbar->add_child(b);
    };

    add_btn("1-Way ->", "Connect From -> To (one-way narrative street)", "narratives.location_map.connect_one_way", "_on_connect_one_way_pressed");
    add_btn("2-Way <->", "Connect From <-> To (bidirectional)", "narratives.location_map.connect_two_way", "_on_connect_two_way_pressed");
    add_btn("Disconnect", "Remove connection between From and To", "narratives.location_map.disconnect", "_on_disconnect_pressed");
    add_btn("Find Path", "Highlight shortest directed BFS path from From to To", "narratives.location_map.find_path", "_on_find_path_pressed");
    add_btn("Clear Path", "Clear highlighted path", "narratives.location_map.clear_path", "_on_clear_path_pressed");
    add_btn("Auto-Layout", "Arrange locations in a ring", "narratives.location_map.auto_layout", "_on_auto_layout_pressed");
    add_btn("?", "Help: Location Topology Map", "narratives.location_map.help", "_on_help_pressed");

    canvas_ = memnew(Control);
    canvas_->set_h_size_flags(SIZE_EXPAND_FILL);
    canvas_->set_v_size_flags(SIZE_EXPAND_FILL);
    canvas_->set_custom_minimum_size(Vector2(480, 320));
    canvas_->set_clip_contents(true);
    domain_tool::stamp_ui_selector(canvas_, "narratives.location_map.canvas");
    canvas_->connect("draw", Callable(this, "_on_canvas_draw"));
    canvas_->connect("gui_input", Callable(this, "_on_canvas_gui_input"));
    add_child(canvas_);

    status_lbl_ = memnew(Label);
    status_lbl_->set_text("Left-click a location to select From, Shift+click (or Right-click) to select To, drag to arrange.");
    status_lbl_->add_theme_color_override("font_color", Color(0.75f, 0.80f, 0.86f));
    add_child(status_lbl_);
}

void LocationMap::bind_domain_asset(
    const Ref<Resource> &p_asset,
    const Ref<ToolContext> &p_context
) {
    set_tool_context(p_context);
    if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        set_narrative(nar);
    } else if (Ref<Location> loc = p_asset; loc.is_valid()) {
        TypedArray<Location> group;
        group.append(loc);
        TypedArray<Location> reachable = loc->get_reachable_locations(32);
        for (int i = 0; i < reachable.size(); ++i) {
            group.append(reachable[i]);
        }
        set_locations(group);
    }
}

void LocationMap::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
}

void LocationMap::set_narrative(const Ref<Narrative> &p_narrative) {
    if (narrative_.is_valid() && narrative_->is_connected("roster_changed", Callable(this, "_on_narrative_changed"))) {
        narrative_->disconnect("roster_changed", Callable(this, "_on_narrative_changed"));
    }
    narrative_ = p_narrative;
    if (narrative_.is_valid()) {
        narrative_->connect("roster_changed", Callable(this, "_on_narrative_changed"));
    }
    refresh_map();
}

void LocationMap::set_locations(const TypedArray<Location> &p_locations) {
    explicit_locations_ = p_locations;
    refresh_map();
}

void LocationMap::_sync_nodes_from_sources() {
    nodes_.clear();
    std::unordered_set<uint64_t> seen;

    auto add_loc = [&](const Ref<Location> &p_loc, bool p_symbolic) {
        if (!p_loc.is_valid()) return;
        const uint64_t id = p_loc->get_instance_id();
        if (seen.find(id) != seen.end()) return;
        seen.insert(id);

        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid() && !p_loc->get_undo_redo().is_valid()) {
            p_loc->set_undo_redo(tool_context_->get_undo_redo());
        }

        NodeLayout nl;
        nl.location    = p_loc;
        nl.is_symbolic = p_symbolic;
        const auto it  = saved_positions_.find(id);
        nl.center      = (it != saved_positions_.end()) ? it->second : Vector2(-1.0f, -1.0f);
        nodes_.push_back(nl);
    };

    if (narrative_.is_valid()) {
        const TypedArray<Location> lits = narrative_->get_locations();
        for (int i = 0; i < lits.size(); ++i) add_loc(lits[i], false);
        const TypedArray<Location> syms = narrative_->get_places();
        for (int i = 0; i < syms.size(); ++i) add_loc(syms[i], true);
    } else {
        for (int i = 0; i < explicit_locations_.size(); ++i) {
            add_loc(explicit_locations_[i], false);
        }
    }

    bool any_unplaced = false;
    for (const auto &n : nodes_) {
        if (n.center.x < 0.0f) {
            any_unplaced = true;
            break;
        }
    }
    if (any_unplaced) {
        _apply_circular_layout();
    }
}

void LocationMap::_apply_circular_layout() {
    const size_t count = nodes_.size();
    if (count == 0) return;

    const Vector2 sz   = canvas_ ? canvas_->get_size() : Vector2(640.0f, 380.0f);
    const float w      = (sz.x > 200.0f) ? sz.x : 640.0f;
    const float h      = (sz.y > 200.0f) ? sz.y : 380.0f;
    const Vector2 mid  = Vector2(w * 0.5f, h * 0.5f);
    const float radius = (count == 1) ? 0.0f : (std::min(w, h) * 0.34f);

    for (size_t i = 0; i < count; ++i) {
        const float angle = (6.2831853f * static_cast<float>(i)) / static_cast<float>(count) - 1.5707963f;
        nodes_[i].center  = mid + Vector2(std::cos(angle) * radius, std::sin(angle) * radius);
        saved_positions_[nodes_[i].location->get_instance_id()] = nodes_[i].center;
    }
}

void LocationMap::_refresh_dropdowns() {
    source_opt_->clear();
    target_opt_->clear();

    for (size_t i = 0; i < nodes_.size(); ++i) {
        const Ref<Location> &loc = nodes_[i].location;
        String label = loc->get_official_name().is_empty()
            ? (String("Location #") + String::num_int64(static_cast<int64_t>(i)))
            : loc->get_official_name();
        if (nodes_[i].is_symbolic) label = String("[Place] ") + label;
        source_opt_->add_item(label, static_cast<int>(i));
        target_opt_->add_item(label, static_cast<int>(i));
    }

    if (selected_source_idx_ >= 0 && selected_source_idx_ < static_cast<int>(nodes_.size())) {
        source_opt_->select(selected_source_idx_);
    } else if (!nodes_.empty()) {
        selected_source_idx_ = 0;
        source_opt_->select(0);
    } else {
        selected_source_idx_ = -1;
    }

    if (selected_target_idx_ >= 0 && selected_target_idx_ < static_cast<int>(nodes_.size())) {
        target_opt_->select(selected_target_idx_);
    } else if (nodes_.size() > 1) {
        selected_target_idx_ = 1;
        target_opt_->select(1);
    } else {
        selected_target_idx_ = -1;
    }
}

void LocationMap::refresh_map() {
    _sync_nodes_from_sources();
    _refresh_dropdowns();
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::auto_layout() {
    _apply_circular_layout();
    if (canvas_) canvas_->queue_redraw();
}

int LocationMap::_hit_test_node(const Vector2 &p_pos) const {
    for (int i = static_cast<int>(nodes_.size()) - 1; i >= 0; --i) {
        const Rect2 rect(
            nodes_[static_cast<size_t>(i)].center - Vector2(LOC_NODE_W * 0.5f, LOC_NODE_H * 0.5f),
            Vector2(LOC_NODE_W, LOC_NODE_H)
        );
        if (rect.has_point(p_pos)) {
            return i;
        }
    }
    return -1;
}

bool LocationMap::_is_edge_on_highlighted_path(uint64_t p_from_id, uint64_t p_to_id) const {
    if (highlighted_path_ids_.size() < 2) return false;
    for (size_t i = 0; i + 1 < highlighted_path_ids_.size(); ++i) {
        if (highlighted_path_ids_[i] == p_from_id && highlighted_path_ids_[i + 1] == p_to_id) {
            return true;
        }
    }
    return false;
}

void LocationMap::_on_canvas_draw() {
    if (!canvas_) return;
    const Vector2 sz = canvas_->get_size();
    canvas_->draw_rect(Rect2(Vector2(), sz), Color(0.09f, 0.10f, 0.13f, 1.0f), true);

    std::unordered_map<uint64_t, size_t> id_to_idx;
    for (size_t i = 0; i < nodes_.size(); ++i) {
        id_to_idx[nodes_[i].location->get_instance_id()] = i;
    }

    auto draw_arrow = [&](const Vector2 &p_from, const Vector2 &p_to, const Color &p_col, float p_width) {
        const Vector2 dir = (p_to - p_from).normalized();
        if (dir.length_squared() < 0.0001f) return;
        const Vector2 start = p_from + dir * 32.0f;
        const Vector2 end   = p_to - dir * 36.0f;
        canvas_->draw_line(start, end, p_col, p_width, true);

        const Vector2 perp(-dir.y, dir.x);
        PackedVector2Array head;
        head.append(end);
        head.append(end - dir * 11.0f + perp * 5.5f);
        head.append(end - dir * 11.0f - perp * 5.5f);
        canvas_->draw_colored_polygon(head, p_col);
    };

    // 1. Draw directed topology edges
    for (size_t i = 0; i < nodes_.size(); ++i) {
        const Ref<Location> &src_loc = nodes_[i].location;
        const uint64_t src_id        = src_loc->get_instance_id();
        const TypedArray<Location> conns = src_loc->get_connected_locations();

        for (int c = 0; c < conns.size(); ++c) {
            Ref<Location> dst_loc = conns[c];
            if (!dst_loc.is_valid()) continue;
            const auto it = id_to_idx.find(dst_loc->get_instance_id());
            if (it == id_to_idx.end()) continue;

            const bool mutual    = dst_loc->is_connected_to(src_loc);
            const bool on_path   = _is_edge_on_highlighted_path(src_id, dst_loc->get_instance_id());
            const Color edge_col = on_path
                ? Color(0.32f, 0.98f, 0.56f, 1.0f)
                : (mutual ? Color(0.36f, 0.78f, 0.94f, 0.85f) : Color(0.96f, 0.68f, 0.28f, 0.92f));
            const float width = on_path ? 3.5f : 2.0f;

            Vector2 offset;
            if (mutual) {
                const Vector2 d = (nodes_[it->second].center - nodes_[i].center).normalized();
                offset = Vector2(-d.y, d.x) * 4.5f;
            }
            draw_arrow(nodes_[i].center + offset, nodes_[it->second].center + offset, edge_col, width);
        }
    }

    // 2. Draw Location cards
    const Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();
    for (size_t i = 0; i < nodes_.size(); ++i) {
        const NodeLayout &nl = nodes_[i];
        const Rect2 card(nl.center - Vector2(LOC_NODE_W * 0.5f, LOC_NODE_H * 0.5f), Vector2(LOC_NODE_W, LOC_NODE_H));

        Color bg = nl.is_symbolic ? Color(0.20f, 0.16f, 0.28f, 0.96f) : Color(0.16f, 0.19f, 0.25f, 0.96f);
        Color border(0.42f, 0.50f, 0.64f, 0.90f);
        if (static_cast<int>(i) == selected_source_idx_) {
            border = Color(0.38f, 0.92f, 0.58f, 1.0f);
        } else if (static_cast<int>(i) == selected_target_idx_) {
            border = Color(0.98f, 0.72f, 0.30f, 1.0f);
        }

        canvas_->draw_rect(card, bg, true);
        canvas_->draw_rect(card, border, false, 2.0f);

        if (font.is_valid()) {
            String title = nl.location->get_official_name().is_empty()
                ? (String("Location #") + String::num_int64(static_cast<int64_t>(i)))
                : nl.location->get_official_name();
            if (nl.is_symbolic) title = String("[Place] ") + title;
            canvas_->draw_string(font, card.position + Vector2(8, 22), title, HORIZONTAL_ALIGNMENT_LEFT, LOC_NODE_W - 16, 13, Color(0.95f, 0.96f, 1.0f));

            const String sub = String("Out: ") + String::num_int64(nl.location->get_connection_count()) +
                               String("  |  C:") + String::num_int64(nl.location->get_characters().size()) +
                               String(" P:") + String::num_int64(nl.location->get_props().size());
            canvas_->draw_string(font, card.position + Vector2(8, 42), sub, HORIZONTAL_ALIGNMENT_LEFT, LOC_NODE_W - 16, 11, Color(0.70f, 0.76f, 0.85f));
        }
    }
}

void LocationMap::_on_canvas_gui_input(const Ref<InputEvent> &p_event) {
    if (Ref<InputEventMouseButton> mb = p_event; mb.is_valid()) {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT) {
            if (mb->is_pressed()) {
                const int hit = _hit_test_node(mb->get_position());
                if (hit >= 0) {
                    if (mb->is_shift_pressed()) {
                        selected_target_idx_ = hit;
                        target_opt_->select(hit);
                    } else {
                        selected_source_idx_ = hit;
                        source_opt_->select(hit);
                        dragging_idx_ = hit;
                        drag_offset_  = nodes_[static_cast<size_t>(hit)].center - mb->get_position();
                        emit_signal("location_selected", nodes_[static_cast<size_t>(hit)].location);
                    }
                    canvas_->queue_redraw();
                }
            } else {
                dragging_idx_ = -1;
            }
        } else if (mb->get_button_index() == MOUSE_BUTTON_RIGHT && mb->is_pressed()) {
            const int hit = _hit_test_node(mb->get_position());
            if (hit >= 0) {
                selected_target_idx_ = hit;
                target_opt_->select(hit);
                canvas_->queue_redraw();
            }
        }
    } else if (Ref<InputEventMouseMotion> mm = p_event; mm.is_valid()) {
        if (dragging_idx_ >= 0 && dragging_idx_ < static_cast<int>(nodes_.size())) {
            NodeLayout &nl = nodes_[static_cast<size_t>(dragging_idx_)];
            nl.center      = mm->get_position() + drag_offset_;
            saved_positions_[nl.location->get_instance_id()] = nl.center;
            canvas_->queue_redraw();
        }
    }
}

void LocationMap::_on_source_selected(int p_index) {
    selected_source_idx_ = p_index;
    if (p_index >= 0 && p_index < static_cast<int>(nodes_.size())) {
        emit_signal("location_selected", nodes_[static_cast<size_t>(p_index)].location);
    }
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_target_selected(int p_index) {
    selected_target_idx_ = p_index;
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_connect_one_way_pressed() {
    if (selected_source_idx_ < 0 || selected_target_idx_ < 0 || selected_source_idx_ == selected_target_idx_) return;
    if (selected_source_idx_ >= static_cast<int>(nodes_.size()) || selected_target_idx_ >= static_cast<int>(nodes_.size())) return;

    nodes_[static_cast<size_t>(selected_source_idx_)].location->connect_location(
        nodes_[static_cast<size_t>(selected_target_idx_)].location, false
    );
    status_lbl_->set_text("Connected one-way street (amber arrow).");
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_connect_two_way_pressed() {
    if (selected_source_idx_ < 0 || selected_target_idx_ < 0 || selected_source_idx_ == selected_target_idx_) return;
    if (selected_source_idx_ >= static_cast<int>(nodes_.size()) || selected_target_idx_ >= static_cast<int>(nodes_.size())) return;

    nodes_[static_cast<size_t>(selected_source_idx_)].location->connect_location(
        nodes_[static_cast<size_t>(selected_target_idx_)].location, true
    );
    status_lbl_->set_text("Connected bidirectional street (cyan arrows).");
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_disconnect_pressed() {
    if (selected_source_idx_ < 0 || selected_target_idx_ < 0 || selected_source_idx_ == selected_target_idx_) return;
    if (selected_source_idx_ >= static_cast<int>(nodes_.size()) || selected_target_idx_ >= static_cast<int>(nodes_.size())) return;

    nodes_[static_cast<size_t>(selected_source_idx_)].location->disconnect_location(
        nodes_[static_cast<size_t>(selected_target_idx_)].location, false
    );
    status_lbl_->set_text("Disconnected From -> To edge.");
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_find_path_pressed() {
    highlighted_path_ids_.clear();
    if (selected_source_idx_ < 0 || selected_target_idx_ < 0) return;
    if (selected_source_idx_ >= static_cast<int>(nodes_.size()) || selected_target_idx_ >= static_cast<int>(nodes_.size())) return;

    const Ref<Location> src = nodes_[static_cast<size_t>(selected_source_idx_)].location;
    const Ref<Location> dst = nodes_[static_cast<size_t>(selected_target_idx_)].location;
    const TypedArray<Location> path = src->find_path_to(dst);

    if (path.is_empty()) {
        status_lbl_->set_text(String("No directed path found from ") + src->get_official_name() + String(" to ") + dst->get_official_name() + String("."));
    } else {
        String chain;
        for (int i = 0; i < path.size(); ++i) {
            Ref<Location> step = path[i];
            if (!step.is_valid()) continue;
            highlighted_path_ids_.push_back(step->get_instance_id());
            if (i > 0) chain += " -> ";
            chain += step->get_official_name().is_empty() ? String("Location") : step->get_official_name();
        }
        status_lbl_->set_text(String("Shortest Path (") + String::num_int64(path.size() - 1) + String(" hops): ") + chain);
    }
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_clear_path_pressed() {
    highlighted_path_ids_.clear();
    status_lbl_->set_text("Path highlight cleared.");
    if (canvas_) canvas_->queue_redraw();
}

void LocationMap::_on_auto_layout_pressed() {
    auto_layout();
}

void LocationMap::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void LocationMap::_on_narrative_changed() {
    refresh_map();
}

} // namespace ideam::godot_ext
