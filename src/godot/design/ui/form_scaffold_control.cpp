#include "form_scaffold_control.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>

using namespace godot;

namespace ideam::godot_ext {

void FormScaffoldControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &FormScaffoldControl::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &FormScaffoldControl::get_tool_context);
    ClassDB::bind_method(D_METHOD("set_scaffold", "scaffold"), &FormScaffoldControl::set_scaffold);
    ClassDB::bind_method(D_METHOD("get_scaffold"), &FormScaffoldControl::get_scaffold);
    ClassDB::bind_method(D_METHOD("set_ruler_graph", "ruler"), &FormScaffoldControl::set_ruler_graph);
    ClassDB::bind_method(D_METHOD("get_ruler_graph"), &FormScaffoldControl::get_ruler_graph);

    ClassDB::bind_method(D_METHOD("_on_archetype_selected", "index"), &FormScaffoldControl::_on_archetype_selected);
    ClassDB::bind_method(D_METHOD("_on_unit_changed", "value"), &FormScaffoldControl::_on_unit_changed);
    ClassDB::bind_method(D_METHOD("_on_contrapposto_changed", "value"), &FormScaffoldControl::_on_contrapposto_changed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &FormScaffoldControl::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_ortho_canvas_draw"), &FormScaffoldControl::_on_ortho_canvas_draw);
    ClassDB::bind_method(D_METHOD("_on_asset_changed"), &FormScaffoldControl::_on_asset_changed);
}

FormScaffoldControl::FormScaffoldControl() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);

    domain_tool::stamp_ui_selector(this, "design.scaffold_control");
    domain_tool::attach_help_topic(this, get_domain_tool_id());
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());

    _build_ui();
}

PackedStringArray FormScaffoldControl::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("design");
    tags.append("loomis");
    tags.append("scaffold");
    tags.append("proportions");
    tags.append("skeleton");
    return tags;
}

void FormScaffoldControl::_build_ui() {
    auto *bar = memnew(HBoxContainer);
    bar->add_theme_constant_override("separation", 6);
    add_child(bar);

    auto *arch_lbl = memnew(Label);
    arch_lbl->set_text("Canon:");
    bar->add_child(arch_lbl);

    archetype_opt_ = memnew(OptionButton);
    archetype_opt_->add_item("Biped Normal (7.5H)", SCAFFOLD_ARCHETYPE_BIPED_NORMAL);
    archetype_opt_->add_item("Biped Idealistic (8H)", SCAFFOLD_ARCHETYPE_BIPED_IDEALISTIC);
    archetype_opt_->add_item("Biped Heroic (8.5H)", SCAFFOLD_ARCHETYPE_BIPED_HEROIC);
    archetype_opt_->add_item("Biped Child (5H)", SCAFFOLD_ARCHETYPE_BIPED_CHILD);
    archetype_opt_->add_item("Biped Stylized (4.5H)", SCAFFOLD_ARCHETYPE_BIPED_STYLIZED);
    archetype_opt_->add_item("Quadruped", SCAFFOLD_ARCHETYPE_QUADRUPED);
    archetype_opt_->add_item("Avian", SCAFFOLD_ARCHETYPE_AVIAN);
    archetype_opt_->add_item("Serpentine", SCAFFOLD_ARCHETYPE_SERPENTINE);
    domain_tool::stamp_ui_selector(archetype_opt_, "design.scaffold_control.archetype");
    archetype_opt_->connect("item_selected", Callable(this, "_on_archetype_selected"));
    bar->add_child(archetype_opt_);

    auto *unit_lbl = memnew(Label);
    unit_lbl->set_text("Unit (m):");
    bar->add_child(unit_lbl);

    unit_spin_ = memnew(SpinBox);
    unit_spin_->set_min(0.05);
    unit_spin_->set_max(5.0);
    unit_spin_->set_step(0.01);
    unit_spin_->set_value(0.23);
    domain_tool::stamp_ui_selector(unit_spin_, "design.scaffold_control.unit_meters");
    unit_spin_->connect("value_changed", Callable(this, "_on_unit_changed"));
    bar->add_child(unit_spin_);

    auto *contra_lbl = memnew(Label);
    contra_lbl->set_text("Contrapposto:");
    bar->add_child(contra_lbl);

    contrapposto_sld_ = memnew(HSlider);
    contrapposto_sld_->set_min(-1.0);
    contrapposto_sld_->set_max(1.0);
    contrapposto_sld_->set_step(0.02);
    contrapposto_sld_->set_value(0.0);
    contrapposto_sld_->set_custom_minimum_size(Vector2(110.0f, 18.0f));
    domain_tool::stamp_ui_selector(contrapposto_sld_, "design.scaffold_control.contrapposto");
    contrapposto_sld_->connect("value_changed", Callable(this, "_on_contrapposto_changed"));
    bar->add_child(contrapposto_sld_);

    auto *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for Loomis Form Scaffolds & Proportion Rulers");
    domain_tool::stamp_ui_selector(help_btn, "design.scaffold_control.help_btn");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    bar->add_child(help_btn);

    ortho_canvas_ = memnew(Control);
    ortho_canvas_->set_custom_minimum_size(Vector2(320.0f, 260.0f));
    ortho_canvas_->set_h_size_flags(SIZE_EXPAND_FILL);
    ortho_canvas_->set_v_size_flags(SIZE_EXPAND_FILL);
    domain_tool::stamp_ui_selector(ortho_canvas_, "design.scaffold_control.ortho_canvas");
    ortho_canvas_->connect("draw", Callable(this, "_on_ortho_canvas_draw"));
    add_child(ortho_canvas_);

    telemetry_lbl_ = memnew(Label);
    telemetry_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(telemetry_lbl_);
}

void FormScaffoldControl::bind_domain_asset(const Ref<Resource> &p_asset, const Ref<ToolContext> &p_context) {
    set_tool_context(p_context);
    if (Ref<FormScaffoldResource> sc = p_asset; sc.is_valid()) {
        set_scaffold(sc);
    } else if (Ref<ProportionRulerGraphResource> rg = p_asset; rg.is_valid()) {
        set_ruler_graph(rg);
    }
}

void FormScaffoldControl::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        if (scaffold_.is_valid() && !scaffold_->get_undo_redo().is_valid()) scaffold_->set_undo_redo(tool_context_->get_undo_redo());
        if (ruler_.is_valid() && !ruler_->get_undo_redo().is_valid()) ruler_->set_undo_redo(tool_context_->get_undo_redo());
    }
}

void FormScaffoldControl::set_scaffold(const Ref<FormScaffoldResource> &p_scaffold) {
    const Callable cb(this, "_on_asset_changed");
    if (scaffold_.is_valid() && scaffold_->is_connected("changed", cb)) scaffold_->disconnect("changed", cb);
    if (ruler_.is_valid() && ruler_->is_connected("changed", cb)) ruler_->disconnect("changed", cb);

    ruler_.unref();
    scaffold_ = p_scaffold;
    if (scaffold_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid() && !scaffold_->get_undo_redo().is_valid()) {
            scaffold_->set_undo_redo(tool_context_->get_undo_redo());
        }
        if (!scaffold_->is_connected("changed", cb)) scaffold_->connect("changed", cb);
    }
    _sync_ui();
}

void FormScaffoldControl::set_ruler_graph(const Ref<ProportionRulerGraphResource> &p_ruler) {
    const Callable cb(this, "_on_asset_changed");
    if (scaffold_.is_valid() && scaffold_->is_connected("changed", cb)) scaffold_->disconnect("changed", cb);
    if (ruler_.is_valid() && ruler_->is_connected("changed", cb)) ruler_->disconnect("changed", cb);

    scaffold_.unref();
    ruler_ = p_ruler;
    if (ruler_.is_valid() && !ruler_->is_connected("changed", cb)) {
        ruler_->connect("changed", cb);
    }
    _sync_ui();
}

void FormScaffoldControl::_sync_ui() {
    updating_ui_ = true;
    if (scaffold_.is_valid()) {
        archetype_opt_->select(static_cast<int>(scaffold_->get_archetype()));
        if (scaffold_->get_ruler_graph().is_valid()) {
            unit_spin_->set_value(static_cast<double>(scaffold_->get_ruler_graph()->get_base_unit_meters()));
        }
        contrapposto_sld_->set_value(static_cast<double>(scaffold_->get_contrapposto_amount()));

        const Dictionary bal = scaffold_->get_balance_report();
        telemetry_lbl_->set_text(
            scaffold_->get_scaffold_name() + String("  |  Nodes/Bones: ") + String::num_int64(scaffold_->get_node_count()) +
            String("  |  Balance: ") + (bool(bal.get("is_balanced", true)) ? String("Balanced") : String("Off-Balance")) +
            String(" (CoG offset ") + String::num(double(bal.get("horizontal_offset", 0.0)), 2) + String("m)")
        );
    } else if (ruler_.is_valid()) {
        unit_spin_->set_value(static_cast<double>(ruler_->get_base_unit_meters()));
        telemetry_lbl_->set_text(
            ruler_->get_canon_name() + String("  |  Unit: ") + String::num(ruler_->get_base_unit_meters(), 2) +
            String("m  |  Landmarks: ") + String::num_int64(ruler_->get_landmarks().size())
        );
    } else {
        telemetry_lbl_->set_text("No FormScaffoldResource or ProportionRulerGraphResource bound.");
    }
    updating_ui_ = false;
    if (ortho_canvas_) ortho_canvas_->queue_redraw();
}

void FormScaffoldControl::_on_archetype_selected(int p_index) {
    if (updating_ui_ || !scaffold_.is_valid()) return;
    scaffold_->set_archetype(static_cast<FormScaffoldArchetype>(p_index));
}

void FormScaffoldControl::_on_unit_changed(double p_val) {
    if (updating_ui_) return;
    if (scaffold_.is_valid() && scaffold_->get_ruler_graph().is_valid()) {
        scaffold_->get_ruler_graph()->set_base_unit_meters(static_cast<real_t>(p_val));
    } else if (ruler_.is_valid()) {
        ruler_->set_base_unit_meters(static_cast<real_t>(p_val));
    }
}

void FormScaffoldControl::_on_contrapposto_changed(double p_val) {
    if (updating_ui_ || !scaffold_.is_valid()) return;
    scaffold_->set_contrapposto_amount(static_cast<real_t>(p_val));
}

void FormScaffoldControl::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void FormScaffoldControl::_on_asset_changed() {
    _sync_ui();
}

void FormScaffoldControl::_on_ortho_canvas_draw() {
    if (!ortho_canvas_) return;
    const Vector2 sz = ortho_canvas_->get_size();
    ortho_canvas_->draw_rect(Rect2(Vector2(0.0f, 0.0f), sz), Color(0.09f, 0.11f, 0.14f, 1.0f), true);

    const float ground_y = sz.y - 24.0f;
    const float usable_h = std::max(40.0f, sz.y - 48.0f);
    ortho_canvas_->draw_line(Vector2(16.0f, ground_y), Vector2(sz.x - 16.0f, ground_y), Color(0.45f, 0.50f, 0.58f, 0.8f), 1.5f);

    Ref<ProportionRulerGraphResource> rg = scaffold_.is_valid() ? scaffold_->get_ruler_graph() : ruler_;
    float max_h = 1.8f;
    if (rg.is_valid()) {
        const TypedArray<Dictionary> lms = rg->get_landmarks();
        for (int64_t i = 0; i < lms.size(); ++i) {
            const Dictionary d = lms[i];
            max_h = std::max(max_h, static_cast<float>(double(d.get("computed_value", 0.0))));
        }
    }
    const float px_per_m = usable_h / std::max(0.25f, max_h * 1.08f);

    // Draw Loomis Canonical Ruler ticks on the left
    const float ruler_x = 34.0f;
    ortho_canvas_->draw_line(Vector2(ruler_x, ground_y), Vector2(ruler_x, ground_y - max_h * px_per_m), Color(0.32f, 0.82f, 0.98f, 0.9f), 2.0f);
    if (rg.is_valid()) {
        const TypedArray<Dictionary> lms = rg->get_landmarks();
        for (int64_t i = 0; i < lms.size(); ++i) {
            const Dictionary d = lms[i];
            const float y = ground_y - static_cast<float>(double(d.get("computed_value", 0.0))) * px_per_m;
            ortho_canvas_->draw_line(Vector2(ruler_x - 6.0f, y), Vector2(sz.x - 24.0f, y), Color(0.32f, 0.82f, 0.98f, 0.22f), 1.0f);
            ortho_canvas_->draw_line(Vector2(ruler_x - 6.0f, y), Vector2(ruler_x + 10.0f, y), Color(0.32f, 0.82f, 0.98f, 0.95f), 2.0f);
        }
    }

    // Draw Front Orthographic Scaffold Volumes + Plumb Line
    if (scaffold_.is_valid()) {
        const auto &sc = scaffold_->get_core_scaffold();
        const float cx = sz.x * 0.56f;
        for (const auto &n : sc.nodes()) {
            const float nx = cx + n.world_transform[3][0] * px_per_m;
            const float ny = ground_y - n.world_transform[3][1] * px_per_m;
            const float rx = std::max(4.0f, n.evaluated_half_extents.x * px_per_m);
            const float ry = std::max(4.0f, n.evaluated_half_extents.y * px_per_m);
            ortho_canvas_->draw_rect(Rect2(nx - rx, ny - ry, rx * 2.0f, ry * 2.0f), Color(0.86f, 0.78f, 0.64f, 0.32f), true);
            ortho_canvas_->draw_rect(Rect2(nx - rx, ny - ry, rx * 2.0f, ry * 2.0f), Color(0.95f, 0.84f, 0.58f, 0.85f), false, 1.25f);
        }

        const auto bal = sc.compute_balance_report();
        const float cog_x = cx + bal.center_of_gravity.x * px_per_m;
        const float cog_y = ground_y - bal.center_of_gravity.y * px_per_m;
        const Color pcol = bal.is_balanced ? Color(0.34f, 0.94f, 0.48f, 0.95f) : Color(0.98f, 0.42f, 0.25f, 0.95f);
        ortho_canvas_->draw_line(Vector2(cog_x, cog_y), Vector2(cog_x, ground_y), pcol, 2.0f);
        ortho_canvas_->draw_circle(Vector2(cog_x, cog_y), 5.0f, pcol);
    }
}

} // namespace ideam::godot_ext
