#include "surface_patch_control.h"

#include "../coons_patch_3d_resource.h"
#include "../nurbs_patch_3d_resource.h"

#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>

#include <algorithm>

using namespace godot;

namespace ideam::godot_ext {

void SurfacePatchControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &SurfacePatchControl::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &SurfacePatchControl::get_tool_context);
    ClassDB::bind_method(D_METHOD("set_surface_patch", "patch"), &SurfacePatchControl::set_surface_patch);
    ClassDB::bind_method(D_METHOD("get_surface_patch"), &SurfacePatchControl::get_surface_patch);
    ClassDB::bind_method(D_METHOD("set_surface_network", "network"), &SurfacePatchControl::set_surface_network);
    ClassDB::bind_method(D_METHOD("get_surface_network"), &SurfacePatchControl::get_surface_network);

    ClassDB::bind_method(D_METHOD("_on_lod_selected", "index"), &SurfacePatchControl::_on_lod_selected);
    ClassDB::bind_method(D_METHOD("_on_u_divs_changed", "value"), &SurfacePatchControl::_on_u_divs_changed);
    ClassDB::bind_method(D_METHOD("_on_v_divs_changed", "value"), &SurfacePatchControl::_on_v_divs_changed);
    ClassDB::bind_method(D_METHOD("_on_preset_flat_pressed"), &SurfacePatchControl::_on_preset_flat_pressed);
    ClassDB::bind_method(D_METHOD("_on_preset_cylinder_pressed"), &SurfacePatchControl::_on_preset_cylinder_pressed);
    ClassDB::bind_method(D_METHOD("_on_preset_dome_pressed"), &SurfacePatchControl::_on_preset_dome_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &SurfacePatchControl::_on_help_pressed);
    ClassDB::bind_method(D_METHOD("_on_uv_canvas_draw"), &SurfacePatchControl::_on_uv_canvas_draw);
    ClassDB::bind_method(D_METHOD("_on_uv_canvas_gui_input", "event"), &SurfacePatchControl::_on_uv_canvas_gui_input);
    ClassDB::bind_method(D_METHOD("_on_asset_changed"), &SurfacePatchControl::_on_asset_changed);
}

SurfacePatchControl::SurfacePatchControl() {
    set_h_size_flags(SIZE_EXPAND_FILL);
    set_v_size_flags(SIZE_EXPAND_FILL);
    add_theme_constant_override("separation", 6);

    domain_tool::stamp_ui_selector(this, "surfaces.patch_control");
    domain_tool::attach_help_topic(this, get_domain_tool_id());
    domain_tool::attach_semantic_tags(this, get_domain_semantic_tags());

    _build_ui();
}

PackedStringArray SurfacePatchControl::get_domain_semantic_tags() const {
    PackedStringArray tags;
    tags.append("surfaces");
    tags.append("nurbs");
    tags.append("coons");
    tags.append("gordon");
    tags.append("quad_mesh");
    return tags;
}

void SurfacePatchControl::_build_ui() {
    auto *toolbar = memnew(HBoxContainer);
    toolbar->add_theme_constant_override("separation", 6);
    add_child(toolbar);

    auto *lod_lbl = memnew(Label);
    lod_lbl->set_text("LOD:");
    toolbar->add_child(lod_lbl);

    lod_opt_ = memnew(OptionButton);
    lod_opt_->add_item("LOD 0 (Full)", 0);
    lod_opt_->add_item("LOD 1 (50%)", 1);
    lod_opt_->add_item("LOD 2 (25%)", 2);
    lod_opt_->add_item("LOD 3 (Cage)", 3);
    domain_tool::stamp_ui_selector(lod_opt_, "surfaces.patch_control.lod_selector");
    lod_opt_->connect("item_selected", Callable(this, "_on_lod_selected"));
    toolbar->add_child(lod_opt_);

    auto *u_lbl = memnew(Label);
    u_lbl->set_text("U Divs:");
    toolbar->add_child(u_lbl);

    u_divs_spin_ = memnew(SpinBox);
    u_divs_spin_->set_min(2.0);
    u_divs_spin_->set_max(128.0);
    u_divs_spin_->set_step(2.0);
    u_divs_spin_->set_value(12.0);
    domain_tool::stamp_ui_selector(u_divs_spin_, "surfaces.patch_control.u_divs");
    u_divs_spin_->connect("value_changed", Callable(this, "_on_u_divs_changed"));
    toolbar->add_child(u_divs_spin_);

    auto *v_lbl = memnew(Label);
    v_lbl->set_text("V Divs:");
    toolbar->add_child(v_lbl);

    v_divs_spin_ = memnew(SpinBox);
    v_divs_spin_->set_min(2.0);
    v_divs_spin_->set_max(128.0);
    v_divs_spin_->set_step(2.0);
    v_divs_spin_->set_value(12.0);
    domain_tool::stamp_ui_selector(v_divs_spin_, "surfaces.patch_control.v_divs");
    v_divs_spin_->connect("value_changed", Callable(this, "_on_v_divs_changed"));
    toolbar->add_child(v_divs_spin_);

    preset_flat_btn_ = memnew(Button);
    preset_flat_btn_->set_text("Flat");
    domain_tool::stamp_ui_selector(preset_flat_btn_, "surfaces.patch_control.preset_flat");
    preset_flat_btn_->connect("pressed", Callable(this, "_on_preset_flat_pressed"));
    toolbar->add_child(preset_flat_btn_);

    preset_cyl_btn_ = memnew(Button);
    preset_cyl_btn_->set_text("Cylinder");
    domain_tool::stamp_ui_selector(preset_cyl_btn_, "surfaces.patch_control.preset_cylinder");
    preset_cyl_btn_->connect("pressed", Callable(this, "_on_preset_cylinder_pressed"));
    toolbar->add_child(preset_cyl_btn_);

    preset_dome_btn_ = memnew(Button);
    preset_dome_btn_->set_text("Dome / Pillow");
    domain_tool::stamp_ui_selector(preset_dome_btn_, "surfaces.patch_control.preset_dome");
    preset_dome_btn_->connect("pressed", Callable(this, "_on_preset_dome_pressed"));
    toolbar->add_child(preset_dome_btn_);

    auto *help_btn = memnew(Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for Surface Patch Editor");
    domain_tool::stamp_ui_selector(help_btn, "surfaces.patch_control.help_btn");
    help_btn->connect("pressed", Callable(this, "_on_help_pressed"));
    toolbar->add_child(help_btn);

    uv_canvas_ = memnew(Control);
    uv_canvas_->set_custom_minimum_size(Vector2(320.0f, 240.0f));
    uv_canvas_->set_h_size_flags(SIZE_EXPAND_FILL);
    uv_canvas_->set_v_size_flags(SIZE_EXPAND_FILL);
    uv_canvas_->set_mouse_filter(MOUSE_FILTER_STOP);
    domain_tool::stamp_ui_selector(uv_canvas_, "surfaces.patch_control.uv_canvas");
    uv_canvas_->connect("draw", Callable(this, "_on_uv_canvas_draw"));
    uv_canvas_->connect("gui_input", Callable(this, "_on_uv_canvas_gui_input"));
    add_child(uv_canvas_);

    telemetry_lbl_ = memnew(Label);
    telemetry_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    add_child(telemetry_lbl_);

    probe_lbl_ = memnew(Label);
    probe_lbl_->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    probe_lbl_->add_theme_color_override("font_color", Color(0.68f, 0.90f, 1.0f));
    add_child(probe_lbl_);
}

void SurfacePatchControl::bind_domain_asset(const Ref<Resource> &p_asset, const Ref<ToolContext> &p_context) {
    set_tool_context(p_context);
    if (Ref<SurfaceNetworkResource> net = p_asset; net.is_valid()) {
        set_surface_network(net);
    } else if (Ref<SurfacePatchResource> p = p_asset; p.is_valid()) {
        set_surface_patch(p);
    }
}

void SurfacePatchControl::set_tool_context(const Ref<ToolContext> &p_context) {
    tool_context_ = p_context;
    if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
        if (patch_.is_valid() && !patch_->get_undo_redo().is_valid()) {
            patch_->set_undo_redo(tool_context_->get_undo_redo());
        }
        if (network_.is_valid() && !network_->get_undo_redo().is_valid()) {
            network_->set_undo_redo(tool_context_->get_undo_redo());
        }
    }
}

void SurfacePatchControl::set_surface_patch(const Ref<SurfacePatchResource> &p_patch) {
    const Callable cb(this, "_on_asset_changed");
    if (patch_.is_valid() && patch_->is_connected("changed", cb)) patch_->disconnect("changed", cb);
    if (network_.is_valid() && network_->is_connected("changed", cb)) network_->disconnect("changed", cb);

    network_.unref();
    patch_ = p_patch;
    if (patch_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid() && !patch_->get_undo_redo().is_valid()) {
            patch_->set_undo_redo(tool_context_->get_undo_redo());
        }
        if (!patch_->is_connected("changed", cb)) patch_->connect("changed", cb);
    }
    _sync_ui_from_asset();
}

void SurfacePatchControl::set_surface_network(const Ref<SurfaceNetworkResource> &p_network) {
    const Callable cb(this, "_on_asset_changed");
    if (patch_.is_valid() && patch_->is_connected("changed", cb)) patch_->disconnect("changed", cb);
    if (network_.is_valid() && network_->is_connected("changed", cb)) network_->disconnect("changed", cb);

    patch_.unref();
    network_ = p_network;
    if (network_.is_valid()) {
        if (tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid() && !network_->get_undo_redo().is_valid()) {
            network_->set_undo_redo(tool_context_->get_undo_redo());
        }
        if (!network_->is_connected("changed", cb)) network_->connect("changed", cb);
    }
    _sync_ui_from_asset();
}

Ref<SurfacePatchResource> SurfacePatchControl::_active_patch() const {
    if (patch_.is_valid()) return patch_;
    if (network_.is_valid() && network_->get_patch_count() > 0) return network_->get_patch(0);
    return Ref<SurfacePatchResource>();
}

void SurfacePatchControl::_sync_ui_from_asset() {
    updating_ui_ = true;
    Ref<SurfacePatchResource> ap = _active_patch();
    if (ap.is_valid()) {
        u_divs_spin_->set_value(static_cast<double>(ap->get_base_u_divs()));
        v_divs_spin_->set_value(static_cast<double>(ap->get_base_v_divs()));
    }
    updating_ui_ = false;

    if (network_.is_valid()) {
        const Dictionary stats = network_->get_manifold_stats(preview_lod_);
        telemetry_lbl_->set_text(
            String("Network (") + String::num_int64(network_->get_patch_count()) + String(" patches, ") +
            String::num_int64(network_->get_quad_bridges().size()) + String(" bridges, ") +
            String::num_int64(network_->get_pole_stitches().size()) + String(" poles)  |  LOD ") +
            String::num_int64(preview_lod_) + String(": ") +
            String::num_int64(int64_t(stats.get("vertex_count", 0))) + String(" verts / ") +
            String::num_int64(int64_t(stats.get("quad_count", 0))) + String(" quads  |  Manifold: ") +
            (bool(stats.get("is_manifold", false)) ? String("Yes") : String("No"))
        );
    } else if (ap.is_valid()) {
        const Vector2i res = ap->get_resolved_lod_divs(preview_lod_);
        const int64_t quads = static_cast<int64_t>(res.x) * res.y;
        const int64_t verts = static_cast<int64_t>(res.x + 1) * (res.y + 1);
        telemetry_lbl_->set_text(
            ap->get_patch_type_label() + String("  |  LOD ") + String::num_int64(preview_lod_) + String(": ") +
            String::num_int64(res.x) + String("x") + String::num_int64(res.y) + String(" (") +
            String::num_int64(verts) + String(" verts, ") + String::num_int64(quads) + String(" quads)")
        );
    } else {
        telemetry_lbl_->set_text("No SurfacePatchResource or SurfaceNetworkResource bound.");
    }

    if (ap.is_valid() && ap->is_patch_valid()) {
        const Vector3 pos = ap->sample_position(probe_uv_.x, probe_uv_.y);
        const Vector3 nrm = ap->sample_normal(probe_uv_.x, probe_uv_.y);
        probe_lbl_->set_text(
            String("Frame Probe (u=") + String::num(probe_uv_.x, 2) + String(", v=") + String::num(probe_uv_.y, 2) +
            String(")  Pos: (") + String::num(pos.x, 2) + String(", ") + String::num(pos.y, 2) + String(", ") + String::num(pos.z, 2) +
            String(")  Normal: (") + String::num(nrm.x, 2) + String(", ") + String::num(nrm.y, 2) + String(", ") + String::num(nrm.z, 2) + String(")")
        );
    } else {
        probe_lbl_->set_text("Click or drag on the UV canvas to probe sample_frame(u, v).");
    }

    if (uv_canvas_) uv_canvas_->queue_redraw();
}

void SurfacePatchControl::_update_probe_from_canvas_pos(const Vector2 &p_pos) {
    if (!uv_canvas_) return;
    const Vector2 sz = uv_canvas_->get_size();
    constexpr float pad = 18.0f;
    const float w = std::max(1.0f, sz.x - pad * 2.0f);
    const float h = std::max(1.0f, sz.y - pad * 2.0f);
    probe_uv_.x = std::clamp((p_pos.x - pad) / w, 0.0f, 1.0f);
    probe_uv_.y = std::clamp((p_pos.y - pad) / h, 0.0f, 1.0f);
    _sync_ui_from_asset();
}

void SurfacePatchControl::_on_lod_selected(int p_index) {
    preview_lod_ = std::clamp(p_index, 0, 3);
    _sync_ui_from_asset();
}

void SurfacePatchControl::_on_u_divs_changed(double p_val) {
    if (updating_ui_) return;
    if (Ref<SurfacePatchResource> ap = _active_patch(); ap.is_valid()) {
        ap->set_base_u_divs(static_cast<int>(p_val));
    }
}

void SurfacePatchControl::_on_v_divs_changed(double p_val) {
    if (updating_ui_) return;
    if (Ref<SurfacePatchResource> ap = _active_patch(); ap.is_valid()) {
        ap->set_base_v_divs(static_cast<int>(p_val));
    }
}

void SurfacePatchControl::_on_preset_flat_pressed() {
    if (auto *nurbs = Object::cast_to<NurbsPatch3DResource>(_active_patch().ptr())) {
        nurbs->setup_flat_grid();
    } else if (auto *coons = Object::cast_to<CoonsPatch3DResource>(_active_patch().ptr())) {
        coons->setup_default_pillow_patch(2.0f, 2.0f, 0.0f);
    }
}

void SurfacePatchControl::_on_preset_cylinder_pressed() {
    if (auto *nurbs = Object::cast_to<NurbsPatch3DResource>(_active_patch().ptr())) {
        nurbs->setup_cylinder_patch();
    }
}

void SurfacePatchControl::_on_preset_dome_pressed() {
    if (auto *nurbs = Object::cast_to<NurbsPatch3DResource>(_active_patch().ptr())) {
        nurbs->setup_dome_patch();
    } else if (auto *coons = Object::cast_to<CoonsPatch3DResource>(_active_patch().ptr())) {
        coons->setup_default_pillow_patch(2.0f, 2.0f, 0.55f);
    }
}

void SurfacePatchControl::_on_help_pressed() {
    domain_tool::request_control_help(this, tool_context_, get_domain_tool_id());
}

void SurfacePatchControl::_on_asset_changed() {
    _sync_ui_from_asset();
}

void SurfacePatchControl::_on_uv_canvas_draw() {
    if (!uv_canvas_) return;
    const Vector2 sz = uv_canvas_->get_size();
    uv_canvas_->draw_rect(Rect2(Vector2(0.0f, 0.0f), sz), Color(0.10f, 0.12f, 0.16f, 1.0f), true);

    constexpr float pad = 18.0f;
    const Rect2 grid_rect(pad, pad, std::max(10.0f, sz.x - pad * 2.0f), std::max(10.0f, sz.y - pad * 2.0f));
    uv_canvas_->draw_rect(grid_rect, Color(0.15f, 0.18f, 0.24f, 1.0f), true);

    Ref<SurfacePatchResource> ap = _active_patch();
    const Vector2i divs = ap.is_valid() ? ap->get_resolved_lod_divs(preview_lod_) : Vector2i(8, 8);

    const Color u_col(0.30f, 0.78f, 0.96f, 0.45f);
    const Color v_col(0.36f, 0.88f, 0.54f, 0.45f);
    for (int i = 0; i <= divs.x; ++i) {
        const float x = grid_rect.position.x + grid_rect.size.x * (static_cast<float>(i) / static_cast<float>(divs.x));
        uv_canvas_->draw_line(Vector2(x, grid_rect.position.y), Vector2(x, grid_rect.position.y + grid_rect.size.y), u_col, 1.0f);
    }
    for (int j = 0; j <= divs.y; ++j) {
        const float y = grid_rect.position.y + grid_rect.size.y * (static_cast<float>(j) / static_cast<float>(divs.y));
        uv_canvas_->draw_line(Vector2(grid_rect.position.x, y), Vector2(grid_rect.position.x + grid_rect.size.x, y), v_col, 1.0f);
    }

    uv_canvas_->draw_rect(grid_rect, Color(0.55f, 0.68f, 0.84f, 0.85f), false, 1.5f);

    // Draw probe reticle
    const Vector2 p(
        grid_rect.position.x + grid_rect.size.x * probe_uv_.x,
        grid_rect.position.y + grid_rect.size.y * probe_uv_.y
    );
    uv_canvas_->draw_circle(p, 5.5f, Color(1.0f, 0.85f, 0.24f, 1.0f));
    uv_canvas_->draw_line(p - Vector2(10.0f, 0.0f), p + Vector2(10.0f, 0.0f), Color(1.0f, 0.95f, 0.55f, 0.9f), 1.5f);
    uv_canvas_->draw_line(p - Vector2(0.0f, 10.0f), p + Vector2(0.0f, 10.0f), Color(1.0f, 0.95f, 0.55f, 0.9f), 1.5f);
}

void SurfacePatchControl::_on_uv_canvas_gui_input(const Ref<InputEvent> &p_event) {
    if (const auto *mb = Object::cast_to<InputEventMouseButton>(p_event.ptr())) {
        if (mb->get_button_index() == MOUSE_BUTTON_LEFT) {
            dragging_probe_ = mb->is_pressed();
            if (dragging_probe_) {
                _update_probe_from_canvas_pos(mb->get_position());
            }
        }
    } else if (const auto *mm = Object::cast_to<InputEventMouseMotion>(p_event.ptr())) {
        if (dragging_probe_) {
            _update_probe_from_canvas_pos(mm->get_position());
        }
    }
}

} // namespace ideam::godot_ext
