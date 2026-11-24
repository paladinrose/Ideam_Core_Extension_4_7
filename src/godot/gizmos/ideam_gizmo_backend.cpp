#include "ideam_gizmo_backend.h"

#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace ideam::godot_ext {

namespace {

godot::BitField<godot::KeyModifierMask> extract_modifiers(const godot::Ref<godot::InputEvent> &p_event) {
    godot::BitField<godot::KeyModifierMask> mods = 0;
    if (auto *iwm = godot::Object::cast_to<godot::InputEventWithModifiers>(p_event.ptr())) {
        if (iwm->is_shift_pressed()) mods.set_flag(godot::KEY_MASK_SHIFT);
        if (iwm->is_ctrl_pressed())  mods.set_flag(godot::KEY_MASK_CTRL);
        if (iwm->is_alt_pressed())   mods.set_flag(godot::KEY_MASK_ALT);
        if (iwm->is_meta_pressed())  mods.set_flag(godot::KEY_MASK_META);
    }
    return mods;
}

GizmoRay make_ray_from_camera(godot::Camera3D *p_camera, const godot::Vector2 &p_screen_pos) {
    GizmoRay ray{};
    ray.camera     = p_camera;
    ray.screen_pos = p_screen_pos;
    if (p_camera) {
        ray.origin = p_camera->project_ray_origin(p_screen_pos);
        ray.normal = p_camera->project_ray_normal(p_screen_pos);
    }
    return ray;
}

} // namespace

// ============================================================================
// IdeamEditorGizmoAdapter (Context 1 — Godot Editor 3D Viewport)
// ============================================================================

#ifdef TOOLS_ENABLED

void IdeamEditorGizmoAdapter::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_ideam_gizmo", "gizmo"), &IdeamEditorGizmoAdapter::set_ideam_gizmo);
    ClassDB::bind_method(D_METHOD("get_ideam_gizmo"), &IdeamEditorGizmoAdapter::get_ideam_gizmo);
    ClassDB::bind_method(D_METHOD("_on_gizmo_redraw_requested"), &IdeamEditorGizmoAdapter::_on_gizmo_redraw_requested);
}

IdeamEditorGizmoAdapter::IdeamEditorGizmoAdapter() {
    style_resolver_.instantiate();
    create_handle_material("ideam_handles");
}

IdeamEditorGizmoAdapter::~IdeamEditorGizmoAdapter() {
    if (gizmo_.is_valid()) {
        const godot::Callable cb(this, "_on_gizmo_redraw_requested");
        if (gizmo_->is_connected("redraw_requested", cb)) {
            gizmo_->disconnect("redraw_requested", cb);
        }
    }
}

void IdeamEditorGizmoAdapter::set_ideam_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo) {
    const godot::Callable cb(this, "_on_gizmo_redraw_requested");
    if (gizmo_.is_valid() && gizmo_->is_connected("redraw_requested", cb)) {
        gizmo_->disconnect("redraw_requested", cb);
    }
    gizmo_ = p_gizmo;
    if (gizmo_.is_valid()) {
        if (style_resolver_.is_valid()) {
            style_resolver_->set_tool_context(gizmo_->get_tool_context());
        }
        if (!gizmo_->is_connected("redraw_requested", cb)) {
            gizmo_->connect("redraw_requested", cb);
        }
    }
}

void IdeamEditorGizmoAdapter::set_style_resolver(const godot::Ref<IdeamGizmoStyleResolver> &p_resolver) {
    style_resolver_ = p_resolver;
    if (style_resolver_.is_null()) {
        style_resolver_.instantiate();
    }
}

void IdeamEditorGizmoAdapter::request_redraw() {
    if (last_target_id_ != 0u) {
        if (godot::Object *obj = godot::ObjectDB::get_instance(last_target_id_)) {
            if (auto *node = godot::Object::cast_to<godot::Node3D>(obj)) {
                node->update_gizmos();
            }
        }
    }
}

void IdeamEditorGizmoAdapter::_on_gizmo_redraw_requested() {
    request_redraw();
}

int32_t IdeamEditorGizmoAdapter::forward_3d_gui_input(
    godot::Camera3D *p_camera,
    const godot::Ref<godot::InputEvent> &p_event,
    godot::Node3D *p_selected_node
) {
    (void)p_camera;
    if (!gizmo_.is_valid() || !gizmo_->is_enabled() || !p_selected_node || !gizmo_->can_handle(p_selected_node)) {
        return 0; // EditorPlugin::AFTER_GUI_INPUT_PASS
    }
    last_target_id_ = p_selected_node->get_instance_id();

    const auto mods = extract_modifiers(p_event);
    if (mods != last_modifiers_) {
        last_modifiers_ = mods;
        if (gizmo_->on_modifier_state_changed(p_selected_node, mods)) {
            p_selected_node->update_gizmos();
        }
    }
    return 0;
}

bool IdeamEditorGizmoAdapter::_has_gizmo(godot::Node3D *p_for_node_3d) const {
    return gizmo_.is_valid() && gizmo_->is_enabled() && gizmo_->can_handle(p_for_node_3d);
}

godot::String IdeamEditorGizmoAdapter::_get_gizmo_name() const {
    return gizmo_.is_valid() ? gizmo_->get_gizmo_name() : godot::String("IdeamEditorGizmoAdapter");
}

int32_t IdeamEditorGizmoAdapter::_get_priority() const {
    return gizmo_.is_valid() ? gizmo_->get_priority() : 0;
}

void IdeamEditorGizmoAdapter::_redraw(const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo) {
    p_gizmo->clear();
    if (!gizmo_.is_valid() || !gizmo_->is_enabled()) {
        return;
    }

    auto *node = p_gizmo->get_node_3d();
    if (!node) {
        return;
    }
    last_target_id_ = node->get_instance_id();

    if (style_resolver_.is_valid()) {
        style_resolver_->set_tool_context(gizmo_->get_tool_context());
    }

    GizmoViewState view{};
    view.is_editor_context = true;
    view.active_handle_id  = drag_active_ ? active_handle_id_ : -1;
    view.modifiers         = last_modifiers_;
    view.active_tool_mode  = gizmo_->get_active_tool_mode();

    IdeamGizmoDrawList draw_list;
    gizmo_->build_draw_list(node, draw_list, view);

    // 1. Submit line batches
    for (const auto &batch : draw_list.line_batches()) {
        if (batch.points.size() < 2) continue;
        const ResolvedGizmoStyle style = style_resolver_->resolve_style(
            batch.role, batch.state_flags, HANDLE_SHAPE_SPHERE, batch.depth_test
        );
        p_gizmo->add_lines(batch.points, style.material);
    }

    // 2. Submit meshes
    for (const auto &mcmd : draw_list.meshes()) {
        if (mcmd.mesh.is_null()) continue;
        const ResolvedGizmoStyle style = style_resolver_->resolve_style(
            mcmd.role, mcmd.state_flags, HANDLE_SHAPE_SPHERE, mcmd.depth_test
        );
        p_gizmo->add_mesh(mcmd.mesh, style.material, mcmd.transform);
    }

    // 3. Submit interactive handles (split primary vs. secondary)
    godot::PackedVector3Array primary_pts;
    godot::PackedInt32Array primary_ids;
    godot::PackedVector3Array secondary_pts;
    godot::PackedInt32Array secondary_ids;

    for (const auto &h : draw_list.handles()) {
        if (h.secondary) {
            secondary_pts.append(h.position);
            secondary_ids.append(h.id);
        } else {
            primary_pts.append(h.position);
            primary_ids.append(h.id);
        }
    }

    godot::Ref<godot::StandardMaterial3D> handle_mat = get_material("ideam_handles", p_gizmo);
    if (!primary_pts.is_empty()) {
        p_gizmo->add_handles(primary_pts, handle_mat, primary_ids, false, false);
    }
    if (!secondary_pts.is_empty()) {
        p_gizmo->add_handles(secondary_pts, handle_mat, secondary_ids, false, true);
    }
}

godot::String IdeamEditorGizmoAdapter::_get_handle_name(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    if (!gizmo_.is_valid()) return "Handle";
    return gizmo_->get_handle_name(p_gizmo->get_node_3d(), p_handle_id, p_secondary);
}

godot::Variant IdeamEditorGizmoAdapter::_get_handle_value(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary
) const {
    if (!gizmo_.is_valid()) return godot::Variant();
    return gizmo_->get_handle_value(p_gizmo->get_node_3d(), p_handle_id, p_secondary);
}

void IdeamEditorGizmoAdapter::_set_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    godot::Camera3D *p_camera,
    const godot::Vector2 &p_screen_pos
) {
    (void)p_secondary;
    if (!gizmo_.is_valid() || !p_camera) return;
    auto *node = p_gizmo->get_node_3d();
    if (!node) return;

    const GizmoRay ray = make_ray_from_camera(p_camera, p_screen_pos);
    if (!drag_active_ || active_handle_id_ != p_handle_id) {
        drag_active_      = true;
        active_handle_id_ = p_handle_id;
        gizmo_->on_handle_drag_begin(node, p_handle_id, ray);
    }
    gizmo_->on_handle_drag_update(node, p_handle_id, ray, last_modifiers_);
}

void IdeamEditorGizmoAdapter::_commit_handle(
    const godot::Ref<godot::EditorNode3DGizmo> &p_gizmo,
    int32_t p_handle_id,
    bool p_secondary,
    const godot::Variant &p_restore,
    bool p_cancel
) {
    (void)p_secondary;
    (void)p_restore;
    drag_active_      = false;
    active_handle_id_ = -1;
    if (!gizmo_.is_valid()) return;
    auto *node = p_gizmo->get_node_3d();
    if (!node) return;
    gizmo_->on_handle_drag_commit(node, p_handle_id, p_cancel);
    node->update_gizmos();
}

#endif // TOOLS_ENABLED

// ============================================================================
// IdeamViewportGizmoBackend (Contexts 2, 3, and 4 — Runtime-Safe Renderer)
// ============================================================================

void IdeamViewportGizmoBackend::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_style_resolver", "resolver"), &IdeamViewportGizmoBackend::set_style_resolver);
    ClassDB::bind_method(D_METHOD("get_style_resolver"), &IdeamViewportGizmoBackend::get_style_resolver);
}

IdeamViewportGizmoBackend::IdeamViewportGizmoBackend() {
    style_resolver_.instantiate();
}

IdeamViewportGizmoBackend::~IdeamViewportGizmoBackend() {
    _clear_rendered_instances();
}

void IdeamViewportGizmoBackend::_notification(int p_what) {
    if (p_what == NOTIFICATION_EXIT_WORLD) {
        _clear_rendered_instances();
    }
}

void IdeamViewportGizmoBackend::set_style_resolver(const godot::Ref<IdeamGizmoStyleResolver> &p_resolver) {
    style_resolver_ = p_resolver;
    if (style_resolver_.is_null()) {
        style_resolver_.instantiate();
    }
}

void IdeamViewportGizmoBackend::set_depth_test_override(bool p_enabled, GizmoDepthTestMode p_mode) {
    use_depth_test_override_ = p_enabled;
    depth_test_override_     = p_mode;
}

void IdeamViewportGizmoBackend::_clear_rendered_instances() {
    auto *rs = godot::RenderingServer::get_singleton();
    for (auto &inst : rendered_instances_) {
        if (inst.instance_rid.is_valid()) {
            rs->free_rid(inst.instance_rid);
        }
    }
    rendered_instances_.clear();
    pickable_handles_.clear();
}

void IdeamViewportGizmoBackend::begin_frame() {
    _clear_rendered_instances();
}

void IdeamViewportGizmoBackend::append_draw_list(
    int32_t p_gizmo_index,
    const godot::Transform3D &p_target_xform,
    const IdeamGizmoDrawList &p_list
) {
    if (!is_inside_tree() || !get_world_3d().is_valid()) {
        return;
    }
    _submit_draw_list(p_gizmo_index, p_target_xform, p_list);
}

void IdeamViewportGizmoBackend::_submit_draw_list(
    int32_t p_gizmo_index,
    const godot::Transform3D &p_target_xform,
    const IdeamGizmoDrawList &p_list
) {
    auto *rs = godot::RenderingServer::get_singleton();
    const godot::RID scenario = get_world_3d()->get_scenario();
    if (!scenario.is_valid()) {
        return;
    }

    auto add_rs_mesh = [&](const godot::Ref<godot::Mesh> &p_mesh,
                           const godot::Ref<godot::Material> &p_mat,
                           const godot::Transform3D &p_xform) {
        if (p_mesh.is_null()) return;
        const godot::RID rid = rs->instance_create();
        rs->instance_set_base(rid, p_mesh->get_rid());
        rs->instance_set_scenario(rid, scenario);
        rs->instance_set_transform(rid, p_xform);
        rs->instance_set_visible(rid, is_visible_in_tree());
        if (p_mat.is_valid()) {
            rs->instance_geometry_set_material_override(rid, p_mat->get_rid());
        }
        rendered_instances_.push_back({rid, p_mesh, p_mat});
    };

    // 1. Line batches via ImmediateMesh
    for (const auto &batch : p_list.line_batches()) {
        if (batch.points.size() < 2) continue;
        const GizmoDepthTestMode depth_mode = use_depth_test_override_ ? depth_test_override_ : batch.depth_test;
        const ResolvedGizmoStyle style = style_resolver_->resolve_style(
            batch.role, batch.state_flags, HANDLE_SHAPE_SPHERE, depth_mode
        );

        godot::Ref<godot::ImmediateMesh> imm;
        imm.instantiate();
        imm->surface_begin(godot::Mesh::PRIMITIVE_LINES, style.material);
        for (int64_t i = 0; i < batch.points.size(); ++i) {
            imm->surface_add_vertex(batch.points[i]);
        }
        imm->surface_end();
        add_rs_mesh(imm, style.material, p_target_xform);
    }

    // 2. Explicit mesh commands
    for (const auto &mcmd : p_list.meshes()) {
        if (mcmd.mesh.is_null()) continue;
        const GizmoDepthTestMode depth_mode = use_depth_test_override_ ? depth_test_override_ : mcmd.depth_test;
        const ResolvedGizmoStyle style = style_resolver_->resolve_style(
            mcmd.role, mcmd.state_flags, HANDLE_SHAPE_SPHERE, depth_mode
        );
        add_rs_mesh(mcmd.mesh, style.material, p_target_xform * mcmd.transform);
    }

    // 3. Interactive handles (rendered with shape mesh or Theme3DResource diegetic mesh + recorded for picking)
    for (const auto &h : p_list.handles()) {
        const GizmoDepthTestMode depth_mode = use_depth_test_override_ ? depth_test_override_ : h.depth_test;
        const ResolvedGizmoStyle style = style_resolver_->resolve_style(
            h.role, h.state_flags, h.shape, depth_mode
        );
        const godot::Vector3 world_pos = p_target_xform.xform(h.position);
        const real_t effective_r = h.radius * style.handle_scale;

        godot::Transform3D hxform(godot::Basis::from_scale(godot::Vector3(effective_r, effective_r, effective_r)), world_pos);
        add_rs_mesh(style.handle_mesh, style.material, hxform);

        PickableHandle ph{};
        ph.gizmo_index  = p_gizmo_index;
        ph.handle_id    = h.id;
        ph.world_pos    = world_pos;
        ph.world_radius = effective_r;
        ph.secondary    = h.secondary;
        pickable_handles_.push_back(ph);
    }
}

bool IdeamViewportGizmoBackend::pick_handle(
    const GizmoRay &p_ray,
    int32_t &r_gizmo_index,
    int32_t &r_handle_id,
    real_t p_pick_radius_scale
) const {
    if (!p_ray.is_valid() || pickable_handles_.empty()) {
        return false;
    }

    const godot::Vector3 rd = p_ray.normal.normalized();
    real_t best_t = std::numeric_limits<real_t>::max();
    bool found    = false;

    for (const auto &ph : pickable_handles_) {
        const godot::Vector3 oc = ph.world_pos - p_ray.origin;
        const real_t proj = oc.dot(rd);
        if (proj < static_cast<real_t>(0.0)) {
            continue;
        }
        const godot::Vector3 closest = p_ray.origin + rd * proj;
        const real_t dist_sq = (ph.world_pos - closest).length_squared();
        const real_t r = ph.world_radius * std::max(static_cast<real_t>(1.0), p_pick_radius_scale);
        if (dist_sq <= r * r && proj < best_t) {
            best_t        = proj;
            r_gizmo_index = ph.gizmo_index;
            r_handle_id   = ph.handle_id;
            found         = true;
        }
    }
    return found;
}

// ============================================================================
// IdeamGizmoSession (Contexts 2, 3, and 4 Orchestrator)
// ============================================================================

void IdeamGizmoSession::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo("handle_drag_started", PropertyInfo(Variant::INT, "handle_id")));
    ADD_SIGNAL(MethodInfo("handle_drag_committed", PropertyInfo(Variant::INT, "handle_id"), PropertyInfo(Variant::BOOL, "cancelled")));

    ClassDB::bind_method(D_METHOD("set_tool_context", "context"), &IdeamGizmoSession::set_tool_context);
    ClassDB::bind_method(D_METHOD("get_tool_context"), &IdeamGizmoSession::get_tool_context);
    ClassDB::bind_method(D_METHOD("set_target_node", "target"), &IdeamGizmoSession::set_target_node);
    ClassDB::bind_method(D_METHOD("get_target_node"), &IdeamGizmoSession::get_target_node);
    ClassDB::bind_method(D_METHOD("set_camera", "camera"), &IdeamGizmoSession::set_camera);
    ClassDB::bind_method(D_METHOD("get_camera"), &IdeamGizmoSession::get_camera);
    ClassDB::bind_method(D_METHOD("set_active_tool_mode", "mode"), &IdeamGizmoSession::set_active_tool_mode);
    ClassDB::bind_method(D_METHOD("get_active_tool_mode"), &IdeamGizmoSession::get_active_tool_mode);
    ClassDB::bind_method(D_METHOD("set_diegetic_depth_test", "enabled"), &IdeamGizmoSession::set_diegetic_depth_test);
    ClassDB::bind_method(D_METHOD("add_gizmo", "gizmo"), &IdeamGizmoSession::add_gizmo);
    ClassDB::bind_method(D_METHOD("remove_gizmo", "gizmo"), &IdeamGizmoSession::remove_gizmo);
    ClassDB::bind_method(D_METHOD("clear_gizmos"), &IdeamGizmoSession::clear_gizmos);
    ClassDB::bind_method(D_METHOD("rebuild_gizmos_now"), &IdeamGizmoSession::rebuild_gizmos_now);
    ClassDB::bind_method(D_METHOD("handle_input_event", "event"), &IdeamGizmoSession::handle_input_event);

    ClassDB::bind_method(D_METHOD("_on_gizmo_redraw_requested"), &IdeamGizmoSession::_on_gizmo_redraw_requested);
    ClassDB::bind_method(D_METHOD("_on_theme_3d_changed", "theme_3d"), &IdeamGizmoSession::_on_theme_3d_changed);
    ADD_SIGNAL(MethodInfo(
        "gizmo_handle_committed",
        PropertyInfo(Variant::STRING, "gizmo_name"),
        PropertyInfo(Variant::INT, "handle_id"),
        PropertyInfo(Variant::BOOL, "cancelled")
    ));
}

IdeamGizmoSession::IdeamGizmoSession() {
    style_resolver_.instantiate();
    backend_ = memnew(IdeamViewportGizmoBackend);
    backend_->set_name("IdeamViewportGizmoBackend");
    backend_->set_style_resolver(style_resolver_);
    add_child(backend_);
    set_process(true);
}

void IdeamGizmoSession::_notification(int p_what) {
    if (p_what == NOTIFICATION_PROCESS) {
        if (redraw_dirty_) {
            rebuild_gizmos_now();
        }
    }
}

godot::Node3D *IdeamGizmoSession::_resolve_target() const {
    if (target_node_id_ == 0u) return nullptr;
    return godot::Object::cast_to<godot::Node3D>(godot::ObjectDB::get_instance(target_node_id_));
}

godot::Camera3D *IdeamGizmoSession::_resolve_camera() const {
    if (camera_node_id_ != 0u) {
        if (auto *cam = godot::Object::cast_to<godot::Camera3D>(godot::ObjectDB::get_instance(camera_node_id_))) {
            return cam;
        }
    }
    if (is_inside_tree() && get_viewport()) {
        return get_viewport()->get_camera_3d();
    }
    return nullptr;
}

void IdeamGizmoSession::_on_gizmo_redraw_requested() {
    redraw_dirty_ = true;
}

void IdeamGizmoSession::_on_theme_3d_changed(const godot::Ref<godot::Resource> &p_theme_3d) {
    (void)p_theme_3d;
    if (style_resolver_.is_valid()) {
        style_resolver_->clear_cache();
    }
    redraw_dirty_ = true;
}

void IdeamGizmoSession::set_tool_context(const godot::Ref<ToolContext> &p_context) {
    const godot::Callable cb(this, "_on_theme_3d_changed");
    if (tool_context_.is_valid() && tool_context_->is_connected("theme_3d_changed", cb)) {
        tool_context_->disconnect("theme_3d_changed", cb);
    }
    tool_context_ = p_context;
    if (tool_context_.is_valid() && !tool_context_->is_connected("theme_3d_changed", cb)) {
        tool_context_->connect("theme_3d_changed", cb);
    }
    if (style_resolver_.is_valid()) {
        style_resolver_->set_tool_context(tool_context_);
    }
    for (auto &g : gizmos_) {
        if (g.is_valid()) g->set_tool_context(tool_context_);
    }
    redraw_dirty_ = true;
}

void IdeamGizmoSession::set_target_node(godot::Node3D *p_target) {
    target_node_id_ = p_target ? p_target->get_instance_id() : 0u;
    is_dragging_    = false;
    drag_gizmo_     = -1;
    drag_handle_    = -1;
    hovered_gizmo_  = -1;
    hovered_handle_ = -1;
    redraw_dirty_   = true;
}

void IdeamGizmoSession::set_camera(godot::Camera3D *p_camera) {
    camera_node_id_ = p_camera ? p_camera->get_instance_id() : 0u;
}

void IdeamGizmoSession::set_active_tool_mode(const godot::StringName &p_mode) {
    active_tool_mode_ = p_mode;
    for (auto &g : gizmos_) {
        if (g.is_valid()) g->set_active_tool_mode(p_mode);
    }
    redraw_dirty_ = true;
}

void IdeamGizmoSession::set_diegetic_depth_test(bool p_enabled) {
    if (backend_) {
        backend_->set_depth_test_override(p_enabled, p_enabled ? DEPTH_TEST_DIEGETIC_WORLD : DEPTH_TEST_OVERLAY);
    }
    redraw_dirty_ = true;
}

void IdeamGizmoSession::add_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo) {
    if (p_gizmo.is_null()) return;
    for (const auto &existing : gizmos_) {
        if (existing == p_gizmo) return;
    }
    if (tool_context_.is_valid()) {
        p_gizmo->set_tool_context(tool_context_);
    }
    p_gizmo->set_active_tool_mode(active_tool_mode_);
    const godot::Callable cb(this, "_on_gizmo_redraw_requested");
    if (!p_gizmo->is_connected("redraw_requested", cb)) {
        p_gizmo->connect("redraw_requested", cb);
    }
    gizmos_.push_back(p_gizmo);
    redraw_dirty_ = true;
}

void IdeamGizmoSession::remove_gizmo(const godot::Ref<IdeamGizmo> &p_gizmo) {
    if (p_gizmo.is_null()) return;
    const godot::Callable cb(this, "_on_gizmo_redraw_requested");
    if (p_gizmo->is_connected("redraw_requested", cb)) {
        p_gizmo->disconnect("redraw_requested", cb);
    }
    std::erase_if(gizmos_, [&](const godot::Ref<IdeamGizmo> &g) { return g == p_gizmo; });
    redraw_dirty_ = true;
}

void IdeamGizmoSession::clear_gizmos() {
    const godot::Callable cb(this, "_on_gizmo_redraw_requested");
    for (auto &g : gizmos_) {
        if (g.is_valid() && g->is_connected("redraw_requested", cb)) {
            g->disconnect("redraw_requested", cb);
        }
    }
    gizmos_.clear();
    if (backend_) backend_->begin_frame();
    redraw_dirty_ = false;
}

void IdeamGizmoSession::rebuild_gizmos_now() {
    redraw_dirty_ = false;
    if (!backend_) return;
    backend_->begin_frame();

    godot::Node3D *target = _resolve_target();
    if (!target || !target->is_inside_tree()) {
        return;
    }

    godot::Camera3D *cam = _resolve_camera();
    GizmoViewState view{};
    view.camera            = cam;
    view.is_editor_context = false;
    view.modifiers         = modifiers_;
    view.active_tool_mode  = active_tool_mode_;
    if (cam) {
        view.camera_position = cam->get_global_position();
        view.camera_forward  = -cam->get_global_transform().basis.get_column(2);
    }
    if (get_viewport()) {
        view.viewport_size = get_viewport()->get_visible_rect().size;
    }

    const godot::Transform3D target_xform = target->get_global_transform();
    for (int32_t i = 0; i < static_cast<int32_t>(gizmos_.size()); ++i) {
        const auto &g = gizmos_[static_cast<size_t>(i)];
        if (g.is_null() || !g->is_enabled() || !g->can_handle(target)) {
            continue;
        }
        view.hovered_handle_id = (hovered_gizmo_ == i) ? hovered_handle_ : -1;
        view.active_handle_id  = (is_dragging_ && drag_gizmo_ == i) ? drag_handle_ : -1;

        IdeamGizmoDrawList draw_list;
        g->build_draw_list(target, draw_list, view);
        backend_->append_draw_list(i, target_xform, draw_list);
    }
}

bool IdeamGizmoSession::handle_input_event(const godot::Ref<godot::InputEvent> &p_event) {
    if (p_event.is_null()) return false;
    godot::Node3D *target = _resolve_target();
    godot::Camera3D *cam  = _resolve_camera();

    // 1. Track modifier key transitions (e.g., Shift hold for Features Focus Mode) & Undo/Redo shortcuts
    const auto new_mods = extract_modifiers(p_event);
    if (new_mods != modifiers_) {
        modifiers_ = new_mods;
        if (target) {
            for (auto &g : gizmos_) {
                if (g.is_valid() && g->is_enabled() && g->can_handle(target)) {
                    if (g->on_modifier_state_changed(target, modifiers_)) {
                        redraw_dirty_ = true;
                    }
                }
            }
        }
    }

    if (auto *key_ev = godot::Object::cast_to<godot::InputEventKey>(p_event.ptr())) {
        if (key_ev->is_pressed() && !key_ev->is_echo()) {
            if (is_dragging_ && key_ev->get_keycode() == godot::KEY_ESCAPE && target) {
                if (drag_gizmo_ >= 0 && drag_gizmo_ < static_cast<int32_t>(gizmos_.size())) {
                    const godot::String gname = gizmos_[static_cast<size_t>(drag_gizmo_)]->get_gizmo_name();
                    gizmos_[static_cast<size_t>(drag_gizmo_)]->on_handle_drag_commit(target, drag_handle_, true);
                    emit_signal("gizmo_handle_committed", gname, drag_handle_, true);
                }
                const int32_t hid = drag_handle_;
                is_dragging_  = false;
                drag_gizmo_   = -1;
                drag_handle_  = -1;
                redraw_dirty_ = true;
                emit_signal("handle_drag_committed", hid, true);
                return true;
            }
            // Runtime Ctrl+Z / Ctrl+Y undo-redo dispatch
            if (key_ev->is_command_or_control_pressed() && tool_context_.is_valid() && tool_context_->get_undo_redo().is_valid()) {
                auto ur = tool_context_->get_undo_redo();
                if (!ur->is_using_editor_undo_redo()) {
                    if (key_ev->get_keycode() == godot::KEY_Z) {
                        const bool handled = key_ev->is_shift_pressed() ? ur->redo() : ur->undo();
                        if (handled) redraw_dirty_ = true;
                        return handled;
                    }
                    if (key_ev->get_keycode() == godot::KEY_Y) {
                        const bool handled = ur->redo();
                        if (handled) redraw_dirty_ = true;
                        return handled;
                    }
                }
            }
        }
        return false;
    }

    if (!target || !cam || !backend_) {
        return false;
    }

    // 2. Mouse button press / release
    if (auto *mb = godot::Object::cast_to<godot::InputEventMouseButton>(p_event.ptr())) {
        if (mb->get_button_index() == godot::MOUSE_BUTTON_LEFT) {
            const GizmoRay ray = make_ray_from_camera(cam, mb->get_position());
            if (mb->is_pressed()) {
                int32_t hit_gizmo  = -1;
                int32_t hit_handle = -1;
                if (backend_->pick_handle(ray, hit_gizmo, hit_handle)) {
                    if (hit_gizmo >= 0 && hit_gizmo < static_cast<int32_t>(gizmos_.size())) {
                        is_dragging_      = true;
                        drag_gizmo_       = hit_gizmo;
                        drag_handle_      = hit_handle;
                        press_screen_pos_ = mb->get_position();
                        drag_moved_       = false;
                        gizmos_[static_cast<size_t>(hit_gizmo)]->on_handle_drag_begin(target, hit_handle, ray);
                        redraw_dirty_ = true;
                        emit_signal("handle_drag_started", hit_handle);
                        return true;
                    }
                }
            } else if (is_dragging_) {
                const int32_t g_idx = drag_gizmo_;
                const int32_t h_id  = drag_handle_;
                const bool was_click = !drag_moved_;
                is_dragging_ = false;
                drag_gizmo_  = -1;
                drag_handle_ = -1;
                if (g_idx >= 0 && g_idx < static_cast<int32_t>(gizmos_.size())) {
                    if (was_click && gizmos_[static_cast<size_t>(g_idx)]->on_handle_clicked(target, h_id, modifiers_)) {
                        const godot::String gname = gizmos_[static_cast<size_t>(g_idx)]->get_gizmo_name();
                        gizmos_[static_cast<size_t>(g_idx)]->on_handle_drag_commit(target, h_id, true);
                        emit_signal("gizmo_handle_committed", gname, h_id, true);
                    } else {
                        const godot::String gname = gizmos_[static_cast<size_t>(g_idx)]->get_gizmo_name();
                        gizmos_[static_cast<size_t>(g_idx)]->on_handle_drag_commit(target, h_id, false);
                        emit_signal("gizmo_handle_committed", gname, h_id, false);
                    }
                }
                redraw_dirty_ = true;
                emit_signal("handle_drag_committed", h_id, false);
                return true;
            }
        }
        return false;
    }

    // 3. Mouse motion (drag update or hover highlight)
    if (auto *mm = godot::Object::cast_to<godot::InputEventMouseMotion>(p_event.ptr())) {
        const GizmoRay ray = make_ray_from_camera(cam, mm->get_position());
        if (is_dragging_) {
            if ((mm->get_position() - press_screen_pos_).length_squared() > static_cast<real_t>(9.0)) {
                drag_moved_ = true;
            }
            if (drag_gizmo_ >= 0 && drag_gizmo_ < static_cast<int32_t>(gizmos_.size())) {
                gizmos_[static_cast<size_t>(drag_gizmo_)]->on_handle_drag_update(target, drag_handle_, ray, modifiers_);
                redraw_dirty_ = true;
            }
            return true;
        }

        int32_t hit_gizmo  = -1;
        int32_t hit_handle = -1;
        const bool hit = backend_->pick_handle(ray, hit_gizmo, hit_handle);
        const int32_t next_g = hit ? hit_gizmo : -1;
        const int32_t next_h = hit ? hit_handle : -1;
        if (next_g != hovered_gizmo_ || next_h != hovered_handle_) {
            hovered_gizmo_  = next_g;
            hovered_handle_ = next_h;
            redraw_dirty_   = true;
        }
    }

    return false;
}

} // namespace ideam::godot_ext
