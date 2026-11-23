#include "relationship_map.h"

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
#include <unordered_set>

using namespace godot;

namespace ideam::godot_ext {

namespace {
constexpr float REL_NODE_W = 142.0f;
constexpr float REL_NODE_H = 48.0f;

Color color_for_narreme_class(const String &p_cls) {
    if (p_cls == "Character") return Color(0.22f, 0.34f, 0.54f, 0.96f);
    if (p_cls == "Location")  return Color(0.20f, 0.38f, 0.32f, 0.96f);
    if (p_cls == "Prop")      return Color(0.42f, 0.32f, 0.18f, 0.96f);
    if (p_cls == "Incident")  return Color(0.46f, 0.22f, 0.26f, 0.96f);
    if (p_cls == "Plot")      return Color(0.34f, 0.22f, 0.48f, 0.96f);
    return Color(0.24f, 0.26f, 0.30f, 0.96f);
}
} // namespace

void RelationshipMap::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_narrative", "narrative"), &RelationshipMap::set_narrative);
    ClassDB::bind_method(D_METHOD("get_narrative"), &RelationshipMap::get_narrative);
    ClassDB::bind_method(D_METHOD("set_focal_character", "character"), &RelationshipMap::set_focal_character);
    ClassDB::bind_method(D_METHOD("get_focal_character"), &RelationshipMap::get_focal_character);
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &RelationshipMap::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &RelationshipMap::get_tool_context);
    ClassDB::bind_method(D_METHOD("refresh_map"), &RelationshipMap::refresh_map);
    ClassDB::bind_method(D_METHOD("auto_layout"), &RelationshipMap::auto_layout);

    ClassDB::bind_method(D_METHOD("_on_canvas_draw"), &RelationshipMap::_on_canvas_draw);
    ClassDB::bind_method(D_METHOD("_on_canvas_gui_input", "event"), &RelationshipMap::_on_canvas_gui_input);
    ClassDB::bind_method(D_METHOD("_on_focal_selected", "index"), &RelationshipMap::_on_focal_selected);
    ClassDB::bind_method(D_METHOD("_on_add_relationship_pressed"), &RelationshipMap::_on_add_relationship_pressed);
    ClassDB::bind_method(D_METHOD("_on_end_relationship_pressed"), &RelationshipMap::_on_end_relationship_pressed);
    ClassDB::bind_method(D_METHOD("_on_auto_layout_pressed"), &RelationshipMap::_on_auto_layout_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &RelationshipMap::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_narrative_changed"), &RelationshipMap::_on_narrative_changed);

    ADD_SIGNAL(MethodInfo("narreme_selected", PropertyInfo(Variant::OBJECT, "narreme", PROPERTY_HINT_RESOURCE_TYPE, "Narreme")));
}

RelationshipMap::RelationshipMap() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);
    domain_tool::stamp_ui_selector(this, "narratives.relationship_map");
    domain_tool::attach_help_topic(this, StringName("RelationshipMap"));
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());
    _build_ui();
}

PackedStringArray RelationshipMap::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("narrative");
    tags.append("relationship");
    tags.append("character");
    tags.append("social_graph");
    return tags;
}

void RelationshipMap::_build_ui() {
    // Row 1: Focal Character Filter & Layout
    HBoxContainer *top_row = memnew(HBoxContainer);
    top_row->add_theme_constant_override("separation", 6);
    add_child(top_row);

    Label *view_lbl = memnew(Label);
    view_lbl->set_text("View:");
    top_row->add_child(view_lbl);

    focal_opt_ = memnew(OptionButton);
    focal_opt_->set_custom_minimum_size(Vector2(170, 0));
    domain_tool::stamp_ui_selector(focal_opt_, "narratives.relationship_map.focal_filter");
    focal_opt_->connect("item_selected", Callable(this, "_on_focal_selected"));
    top_row->add_child(focal_opt_);

    Button *layout_btn = memnew(Button);
    layout_btn->set_text("Auto-Layout");
    domain_tool::stamp_ui_selector(layout_btn, "narratives.relationship_map.auto_layout");
    layout_btn->connect("pressed", Callable(this, "_on_auto_layout_pressed"));
    top_row->add_child(layout_btn);

    Button *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Help: Character Relationship Map");
    domain_tool::stamp_ui_selector(help_btn, "narratives.relationship_map.help");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    top_row->add_child(help_btn);

    // Row 2: Inline Relationship Editor
    HBoxContainer *edit_row = memnew(HBoxContainer);
    edit_row->add_theme_constant_override("separation", 6);
    add_child(edit_row);

    source_char_opt_ = memnew(OptionButton);
    source_char_opt_->set_custom_minimum_size(Vector2(130, 0));
    domain_tool::stamp_ui_selector(source_char_opt_, "narratives.relationship_map.source_character");
    edit_row->add_child(source_char_opt_);

    rel_title_edit_ = memnew(LineEdit);
    rel_title_edit_->set_placeholder("Relationship title (e.g. Mentor, Rival, Guards)...");
    rel_title_edit_->set_h_size_flags(SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(rel_title_edit_, "narratives.relationship_map.rel_title");
    edit_row->add_child(rel_title_edit_);

    target_nar_opt_ = memnew(OptionButton);
    target_nar_opt_->set_custom_minimum_size(Vector2(140, 0));
    domain_tool::stamp_ui_selector(target_nar_opt_, "narratives.relationship_map.target_narreme");
    edit_row->add_child(target_nar_opt_);

    Button *add_rel_btn = memnew(Button);
    add_rel_btn->set_text("Set Relationship");
    domain_tool::stamp_ui_selector(add_rel_btn, "narratives.relationship_map.add_rel");
    add_rel_btn->connect("pressed", Callable(this, "_on_add_relationship_pressed"));
    edit_row->add_child(add_rel_btn);

    Button *end_rel_btn = memnew(Button);
    end_rel_btn->set_text("End");
    end_rel_btn->set_tooltip_text("End relationship with matching title on selected source character");
    domain_tool::stamp_ui_selector(end_rel_btn, "narratives.relationship_map.end_rel");
    end_rel_btn->connect("pressed", Callable(this, "_on_end_relationship_pressed"));
    edit_row->add_child(end_rel_btn);

    // Row 3: Graph Canvas
    canvas_ = memnew(Control);
    canvas_->set_h_size_flags(SIZE_EXPAND_FILL);
    canvas_->set_v_size_flags(SIZE_EXPAND_FILL);
    canvas_->set_custom_minimum_size(Vector2(480, 320));
    canvas_->set_clip_contents(true);
    domain_tool::stamp_ui_selector(canvas_, "narratives.relationship_map.canvas");
    canvas_->connect("draw", Callable(this, "_on_canvas_draw"));
    canvas_->connect("gui_input", Callable(this, "_on_canvas_gui_input"));
    add_child(canvas_);

    status_lbl_ = memnew(Label);
    status_lbl_->set_text("Drag nodes to organize relationship clusters; click a node to inspect.");
    status_lbl_->add_theme_color_override("font_color", Color(0.75f, 0.80f, 0.86f));
    add_child(status_lbl_);
}

void RelationshipMap::bind_domain_asset(
    const Ref<Resource> &p_asset,
    const Ref<ToolContext> &p_context
) {
    set_tool_context(p_context);
    if (Ref<Narrative> nar = p_asset; nar.is_valid()) {
        set_narrative(nar);
    } else if (Ref<Character> ch = p_asset; ch.is_valid()) {
        set_focal_character(ch);
    }
}

void RelationshipMap::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
}

void RelationshipMap::set_narrative(const Ref<Narrative> &p_narrative) {
    if (narrative_.is_valid() && narrative_->is_connected("roster_changed", Callable(this, "_on_narrative_changed"))) {
        narrative_->disconnect("roster_changed", Callable(this, "_on_narrative_changed"));
    }
    narrative_ = p_narrative;
    if (narrative_.is_valid()) {
        narrative_->connect("roster_changed", Callable(this, "_on_narrative_changed"));
    }
    refresh_map();
}

void RelationshipMap::set_focal_character(const Ref<Character> &p_character) {
    focal_character_ = p_character;
    refresh_map();
}

void RelationshipMap::_rebuild_graph_data() {
    nodes_.clear();
    edges_.clear();

    std::unordered_map<uint64_t, size_t> id_to_idx;
    auto ensure_node = [&](const Ref<Narreme> &p_n, bool p_focal) -> size_t {
        const uint64_t id = p_n->get_instance_id();
        const auto it     = id_to_idx.find(id);
        if (it != id_to_idx.end()) {
            if (p_focal) nodes_[it->second].is_focal_character = true;
            return it->second;
        }
        GraphNode gn;
        gn.narreme            = p_n;
        gn.is_focal_character = p_focal;
        const auto pos_it     = saved_positions_.find(id);
        gn.center             = (pos_it != saved_positions_.end()) ? pos_it->second : Vector2(-1.0f, -1.0f);
        const size_t idx      = nodes_.size();
        nodes_.push_back(gn);
        id_to_idx[id] = idx;
        return idx;
    };

    std::vector<Ref<Character>> active_chars;
    if (focal_character_.is_valid()) {
        active_chars.push_back(focal_character_);
    } else if (narrative_.is_valid()) {
        const TypedArray<Character> chars = narrative_->get_characters();
        for (int i = 0; i < chars.size(); ++i) {
            Ref<Character> c = chars[i];
            if (c.is_valid()) active_chars.push_back(c);
        }
        const TypedArray<Character> roles = narrative_->get_roles();
        for (int i = 0; i < roles.size(); ++i) {
            Ref<Character> r = roles[i];
            if (r.is_valid()) active_chars.push_back(r);
        }
    }

    for (const Ref<Character> &ch : active_chars) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid() && !ch->get_undo_redo().is_valid()) {
            ch->set_undo_redo(tool_context_->get_undo_redo());
        }
        const size_t from_idx = ensure_node(ch, focal_character_.is_valid() && ch == focal_character_);
        const TypedArray<Relationship> rels = ch->get_relationships();
        for (int r = 0; r < rels.size(); ++r) {
            Ref<Relationship> rel = rels[r];
            if (!rel.is_valid()) continue;
            Ref<Narreme> target = rel->get_relation();
            if (!target.is_valid()) continue;
            const size_t to_idx = ensure_node(target, false);
            edges_.push_back({from_idx, to_idx, r, ch, rel});
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
        _apply_radial_layout();
    }
}

void RelationshipMap::_apply_radial_layout() {
    const size_t count = nodes_.size();
    if (count == 0) return;

    const Vector2 sz  = canvas_ ? canvas_->get_size() : Vector2(640.0f, 380.0f);
    const float w     = (sz.x > 200.0f) ? sz.x : 640.0f;
    const float h     = (sz.y > 200.0f) ? sz.y : 380.0f;
    const Vector2 mid = Vector2(w * 0.5f, h * 0.5f);

    if (focal_character_.is_valid() && count > 1) {
        const float radius = std::min(w, h) * 0.34f;
        size_t ring_pos    = 0;
        const size_t outer = count - 1;
        for (size_t i = 0; i < count; ++i) {
            if (nodes_[i].is_focal_character) {
                nodes_[i].center = mid;
            } else {
                const float angle = (6.2831853f * static_cast<float>(ring_pos++)) / static_cast<float>(outer) - 1.5707963f;
                nodes_[i].center  = mid + Vector2(std::cos(angle) * radius, std::sin(angle) * radius);
            }
            saved_positions_[nodes_[i].narreme->get_instance_id()] = nodes_[i].center;
        }
        return;
    }

    const float radius = (count == 1) ? 0.0f : (std::min(w, h) * 0.35f);
    for (size_t i = 0; i < count; ++i) {
        const float angle = (6.2831853f * static_cast<float>(i)) / static_cast<float>(count) - 1.5707963f;
        nodes_[i].center  = mid + Vector2(std::cos(angle) * radius, std::sin(angle) * radius);
        saved_positions_[nodes_[i].narreme->get_instance_id()] = nodes_[i].center;
    }
}

void RelationshipMap::_refresh_pickers() {
    focal_opt_->clear();
    source_char_opt_->clear();
    target_nar_opt_->clear();
    character_choices_.clear();
    target_choices_.clear();

    focal_opt_->add_item("All Characters (Full Graph)", -1);
    int focal_sel = 0;

    if (narrative_.is_valid()) {
        const TypedArray<Character> chars = narrative_->get_characters();
        for (int i = 0; i < chars.size(); ++i) {
            Ref<Character> c = chars[i];
            if (!c.is_valid()) continue;
            const int cid = static_cast<int>(character_choices_.size());
            character_choices_.push_back(c);
            const String lbl = c->get_official_name().is_empty() ? (String("Character #") + String::num_int64(i)) : c->get_official_name();
            focal_opt_->add_item(String("Focus: ") + lbl, cid);
            source_char_opt_->add_item(lbl, cid);
            if (focal_character_.is_valid() && focal_character_ == c) {
                focal_sel = focal_opt_->get_item_count() - 1;
            }
        }
        const TypedArray<Character> roles = narrative_->get_roles();
        for (int i = 0; i < roles.size(); ++i) {
            Ref<Character> r = roles[i];
            if (!r.is_valid()) continue;
            const int cid = static_cast<int>(character_choices_.size());
            character_choices_.push_back(r);
            const String lbl = String("[Role] ") + (r->get_official_name().is_empty() ? (String("Role #") + String::num_int64(i)) : r->get_official_name());
            focal_opt_->add_item(String("Focus: ") + lbl, cid);
            source_char_opt_->add_item(lbl, cid);
            if (focal_character_.is_valid() && focal_character_ == r) {
                focal_sel = focal_opt_->get_item_count() - 1;
            }
        }

        const TypedArray<Narreme> all = narrative_->get_all_narremes(FILTER_ALL);
        for (int i = 0; i < all.size(); ++i) {
            Ref<Narreme> n = all[i];
            if (!n.is_valid()) continue;
            const int tid = static_cast<int>(target_choices_.size());
            target_choices_.push_back(n);
            const String lbl = (n->get_official_name().is_empty() ? n->get_class_name_str() : n->get_official_name()) + String(" (") + n->get_class_name_str() + String(")");
            target_nar_opt_->add_item(lbl, tid);
        }
    } else if (focal_character_.is_valid()) {
        character_choices_.push_back(focal_character_);
        const String lbl = focal_character_->get_official_name().is_empty() ? String("Character") : focal_character_->get_official_name();
        focal_opt_->add_item(String("Focus: ") + lbl, 0);
        source_char_opt_->add_item(lbl, 0);
        focal_sel = 1;
    }

    focal_opt_->select(focal_sel);
}

void RelationshipMap::refresh_map() {
    _rebuild_graph_data();
    _refresh_pickers();
    status_lbl_->set_text(
        String::num_int64(static_cast<int64_t>(nodes_.size())) + String(" narremes  |  ") +
        String::num_int64(static_cast<int64_t>(edges_.size())) + String(" relationships")
    );
    if (canvas_) canvas_->queue_redraw();
}

void RelationshipMap::auto_layout() {
    _apply_radial_layout();
    if (canvas_) canvas_->queue_redraw();
}

int RelationshipMap::_hit_test_node(const Vector2 &p_pos) const {
    for (int i = static_cast<int>(nodes_.size()) - 1; i >= 0; --i) {
        const Rect2 rect(
            nodes_[static_cast<size_t>(i)].center - Vector2(REL_NODE_W * 0.5f, REL_NODE_H * 0.5f),
            Vector2(REL_NODE_W, REL_NODE_H)
        );
        if (rect.has_point(p_pos)) return i;
    }
    return -1;
}

void RelationshipMap::_on_canvas_draw() {
    if (!canvas_) return;
    const Vector2 sz = canvas_->get_size();
    canvas_->draw_rect(Rect2(Vector2(), sz), Color(0.09f, 0.10f, 0.13f, 1.0f), true);

    const Ref<Font> font = ThemeDB::get_singleton()->get_fallback_font();

    // 1. Draw labeled relationship edges
    for (const GraphEdge &e : edges_) {
        const Vector2 p0  = nodes_[e.from_idx].center;
        const Vector2 p1  = nodes_[e.to_idx].center;
        const Vector2 dir = (p1 - p0).normalized();
        if (dir.length_squared() < 0.0001f) continue;

        const Vector2 perp(-dir.y, dir.x);
        const Vector2 start = p0 + dir * 28.0f + perp * 4.0f;
        const Vector2 end   = p1 - dir * 32.0f + perp * 4.0f;
        const Color col(0.84f, 0.68f, 0.98f, 0.88f);

        canvas_->draw_line(start, end, col, 2.0f, true);
        PackedVector2Array head;
        head.append(end);
        head.append(end - dir * 10.0f + perp * 5.0f);
        head.append(end - dir * 10.0f - perp * 5.0f);
        canvas_->draw_colored_polygon(head, col);

        if (font.is_valid() && e.relationship.is_valid()) {
            const Vector2 mid = (start + end) * 0.5f + perp * 10.0f;
            String lbl        = e.relationship->get_title();
            if (e.relationship->get_semantic_tags().is_valid()) {
                lbl += String(" [") + String::num_int64(e.relationship->get_semantic_tags()->get_nuclear_popcount()) + String("s]");
            }
            canvas_->draw_string(font, mid, lbl, HORIZONTAL_ALIGNMENT_CENTER, 120, 11, Color(0.96f, 0.88f, 1.0f));
        }
    }

    // 2. Draw Narreme nodes
    for (size_t i = 0; i < nodes_.size(); ++i) {
        const GraphNode &gn = nodes_[i];
        const Rect2 card(gn.center - Vector2(REL_NODE_W * 0.5f, REL_NODE_H * 0.5f), Vector2(REL_NODE_W, REL_NODE_H));
        const String cls = gn.narreme->get_class_name_str();
        const Color bg   = color_for_narreme_class(cls);
        Color border     = (static_cast<int>(i) == selected_node_idx_)
            ? Color(0.98f, 0.86f, 0.36f, 1.0f)
            : (gn.is_focal_character ? Color(0.45f, 0.92f, 0.98f, 1.0f) : Color(0.50f, 0.54f, 0.65f, 0.85f));

        canvas_->draw_rect(card, bg, true);
        canvas_->draw_rect(card, border, false, gn.is_focal_character ? 2.5f : 1.5f);

        if (font.is_valid()) {
            const String title = gn.narreme->get_official_name().is_empty() ? cls : gn.narreme->get_official_name();
            canvas_->draw_string(font, card.position + Vector2(8, 20), title, HORIZONTAL_ALIGNMENT_LEFT, REL_NODE_W - 16, 13, Color(0.96f, 0.97f, 1.0f));
            canvas_->draw_string(font, card.position + Vector2(8, 38), cls, HORIZONTAL_ALIGNMENT_LEFT, REL_NODE_W - 16, 10, Color(0.75f, 0.80f, 0.88f));
        }
    }
}

void RelationshipMap::_on_canvas_gui_input(const Ref<InputEvent> &p_event) {
    if (Ref<InputEventMouseButton> mb = p_event; mb.is_valid()) {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT) {
            if (mb->is_pressed()) {
                const int hit = _hit_test_node(mb->get_position());
                if (hit >= 0) {
                    selected_node_idx_ = hit;
                    dragging_idx_      = hit;
                    drag_offset_       = nodes_[static_cast<size_t>(hit)].center - mb->get_position();
                    emit_signal("narreme_selected", nodes_[static_cast<size_t>(hit)].narreme);
                    canvas_->queue_redraw();
                }
            } else {
                dragging_idx_ = -1;
            }
        }
    } else if (Ref<InputEventMouseMotion> mm = p_event; mm.is_valid()) {
        if (dragging_idx_ >= 0 && dragging_idx_ < static_cast<int>(nodes_.size())) {
            GraphNode &gn = nodes_[static_cast<size_t>(dragging_idx_)];
            gn.center     = mm->get_position() + drag_offset_;
            saved_positions_[gn.narreme->get_instance_id()] = gn.center;
            canvas_->queue_redraw();
        }
    }
}

void RelationshipMap::_on_focal_selected(int p_index) {
    if (p_index < 0) return;
    const int cid = focal_opt_->get_item_id(p_index);
    if (cid < 0 || cid >= static_cast<int>(character_choices_.size())) {
        focal_character_.unref();
    } else {
        focal_character_ = character_choices_[static_cast<size_t>(cid)];
    }
    refresh_map();
}

void RelationshipMap::_on_add_relationship_pressed() {
    const int src_sel = source_char_opt_->get_selected();
    const int dst_sel = target_nar_opt_->get_selected();
    if (src_sel < 0 || dst_sel < 0) return;

    const int src_id = source_char_opt_->get_item_id(src_sel);
    const int dst_id = target_nar_opt_->get_item_id(dst_sel);
    if (src_id < 0 || src_id >= static_cast<int>(character_choices_.size())) return;
    if (dst_id < 0 || dst_id >= static_cast<int>(target_choices_.size())) return;

    String title = rel_title_edit_->get_text().strip_edges();
    if (title.is_empty()) title = "Related To";

    Ref<Character> ch = character_choices_[static_cast<size_t>(src_id)];
    ch->begin_relationship(target_choices_[static_cast<size_t>(dst_id)], title);
    rel_title_edit_->clear();
    refresh_map();
}

void RelationshipMap::_on_end_relationship_pressed() {
    const int src_sel = source_char_opt_->get_selected();
    if (src_sel < 0) return;
    const int src_id = source_char_opt_->get_item_id(src_sel);
    if (src_id < 0 || src_id >= static_cast<int>(character_choices_.size())) return;

    const String title = rel_title_edit_->get_text().strip_edges();
    Ref<Character> ch  = character_choices_[static_cast<size_t>(src_id)];
    const int rel_id   = ch->get_relationship_id(title);
    if (rel_id >= 0) {
        ch->end_relationship_at(rel_id);
        refresh_map();
    }
}

void RelationshipMap::_on_auto_layout_pressed() {
    auto_layout();
}

void RelationshipMap::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void RelationshipMap::_on_narrative_changed() {
    refresh_map();
}

} // namespace ideam::godot_ext
