#include "surfaces_inspector.h"

#include "../ideam_surfaces_plugin.h"
#include "../ui/surfaces_composer.h"
#include "../../tools/domain/domain_tool_interface.h"
#include "../../tools/tool_registry.h"

#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/panel_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace ideam::godot_ext {

void SurfacesInspector::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("_on_open_in_composer_pressed"), &SurfacesInspector::_on_open_in_composer_pressed);
    ClassDB::bind_method(D_METHOD("_on_help_pressed"), &SurfacesInspector::_on_help_pressed);
}

#ifdef TOOLS_ENABLED
godot::EditorUndoRedoManager *SurfacesInspector::get_undo_redo() const {
    return IdeamSurfacesPlugin::get_static_undo_redo();
}
#endif

bool SurfacesInspector::_can_handle(godot::Object *p_object) {
    if (!p_object) return false;
    return godot::Object::cast_to<SurfacePatchResource>(p_object) != nullptr ||
           godot::Object::cast_to<SurfaceNetworkResource>(p_object) != nullptr ||
           godot::Object::cast_to<SurfaceMeshNode3D>(p_object) != nullptr;
}

void SurfacesInspector::_parse_begin(godot::Object *p_object) {
    if (!p_object) return;

    IdeamEditorInspectorPlugin::_parse_begin(p_object);

    godot::Ref<IdeamUndoRedo> ur = IdeamSurfacesPlugin::create_undo_redo_wrapper();
    if (auto *patch = godot::Object::cast_to<SurfacePatchResource>(p_object)) {
        patch->set_undo_redo(ur);
    } else if (auto *net = godot::Object::cast_to<SurfaceNetworkResource>(p_object)) {
        net->set_undo_redo(ur);
    }

    current_object_id_ = godot::ObjectID(p_object->get_instance_id());

    godot::PanelContainer *panel = memnew(godot::PanelContainer);
    godot::Ref<godot::StyleBoxFlat> style;
    style.instantiate();
    style->set_bg_color(godot::Color(0.10f, 0.14f, 0.18f, 0.95f));
    style->set_border_color(godot::Color(0.32f, 0.70f, 0.88f, 0.85f));
    style->set_border_width_all(1);
    style->set_corner_radius_all(5);
    style->set_content_margin_all(8);
    panel->add_theme_stylebox_override("panel", style);
    domain_tool::stamp_ui_selector(panel, "surfaces.inspector.header");

    godot::VBoxContainer *vbox = memnew(godot::VBoxContainer);
    vbox->add_theme_constant_override("separation", 4);
    panel->add_child(vbox);

    godot::HBoxContainer *top_row = memnew(godot::HBoxContainer);
    vbox->add_child(top_row);

    godot::Label *title_lbl = memnew(godot::Label);
    title_lbl->set_text("IdeamSurfaces — Parametric Quad-Patch Inspector");
    title_lbl->set_h_size_flags(godot::Control::SIZE_EXPAND_FILL);
    title_lbl->add_theme_color_override("font_color", godot::Color(0.74f, 0.92f, 1.0f));
    top_row->add_child(title_lbl);

    godot::Button *open_btn = memnew(godot::Button);
    open_btn->set_text("Open in Composer");
    open_btn->set_tooltip_text("Open this surface resource in the Surfaces Composer window");
    domain_tool::stamp_ui_selector(open_btn, "surfaces.inspector.open_composer_btn");
    open_btn->connect("pressed", godot::Callable(this, "_on_open_in_composer_pressed"));
    top_row->add_child(open_btn);

    godot::Button *help_btn = memnew(godot::Button);
    help_btn->set_text("?");
    help_btn->set_tooltip_text("Open IdeamTutorials help for IdeamSurfaces");
    domain_tool::stamp_ui_selector(help_btn, "surfaces.inspector.help_btn");
    help_btn->connect("pressed", godot::Callable(this, "_on_help_pressed"));
    top_row->add_child(help_btn);

    godot::Label *summary_lbl = memnew(godot::Label);
    summary_lbl->set_autowrap_mode(godot::TextServer::AUTOWRAP_WORD_SMART);
    summary_lbl->add_theme_color_override("font_color", godot::Color(0.80f, 0.85f, 0.90f));
    vbox->add_child(summary_lbl);
    summary_label_id_ = godot::ObjectID(summary_lbl->get_instance_id());

    add_custom_control(panel);
    _refresh_summary_panel();
}

bool SurfacesInspector::_parse_property(
    godot::Object *p_object,
    godot::Variant::Type p_type,
    const godot::String &p_name,
    godot::PropertyHint p_hint_type,
    const godot::String &p_hint_string,
    godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
    bool p_wide
) {
    (void)p_object;
    (void)p_type;
    (void)p_name;
    (void)p_hint_type;
    (void)p_hint_string;
    (void)p_usage_flags;
    (void)p_wide;
    return false;
}

void SurfacesInspector::_refresh_summary_panel() {
    if (current_object_id_.is_null() || summary_label_id_.is_null()) return;
    godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_);
    godot::Label *lbl  = godot::Object::cast_to<godot::Label>(godot::ObjectDB::get_instance(summary_label_id_));
    if (!obj || !lbl) return;

    if (const auto *nurbs = godot::Object::cast_to<NurbsPatch3DResource>(obj)) {
        const godot::Vector2i lod0 = nurbs->get_resolved_lod_divs(0);
        const godot::Vector2i lod3 = nurbs->get_resolved_lod_divs(3);
        lbl->set_text(
            "NURBS Patch (" + godot::String::num_int64(nurbs->get_count_u()) + "x" +
            godot::String::num_int64(nurbs->get_count_v()) + " ctrl pts, deg " +
            godot::String::num_int64(nurbs->get_degree_u()) + "x" +
            godot::String::num_int64(nurbs->get_degree_v()) + ")  |  LOD0: " +
            godot::String::num_int64(static_cast<int64_t>(lod0.x) * lod0.y) + " quads  |  LOD3: " +
            godot::String::num_int64(static_cast<int64_t>(lod3.x) * lod3.y) + " quads"
        );
    } else if (const auto *coons = godot::Object::cast_to<CoonsPatch3DResource>(obj)) {
        const godot::Vector2i lod0 = coons->get_resolved_lod_divs(0);
        lbl->set_text(
            coons->get_patch_type_label() + "  |  U Guides: " +
            godot::String::num_int64(coons->get_interior_u_guides().size()) + "  |  V Guides: " +
            godot::String::num_int64(coons->get_interior_v_guides().size()) + "  |  LOD0: " +
            godot::String::num_int64(static_cast<int64_t>(lod0.x) * lod0.y) + " quads"
        );
    } else if (const auto *net = godot::Object::cast_to<SurfaceNetworkResource>(obj)) {
        const godot::Dictionary stats = net->get_manifold_stats(0);
        lbl->set_text(
            "Surface Network  |  Patches: " + godot::String::num_int64(net->get_patch_count()) +
            "  |  Bridges: " + godot::String::num_int64(net->get_quad_bridges().size()) +
            "  |  Poles: " + godot::String::num_int64(net->get_pole_stitches().size()) +
            "  |  LOD0 Quads: " + godot::String::num_int64(int64_t(stats.get("quad_count", 0))) +
            "  |  Manifold: " + (bool(stats.get("is_manifold", false)) ? godot::String("Yes") : godot::String("No"))
        );
    } else if (const auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(obj)) {
        const godot::String src = node->get_surface_network().is_valid()
            ? godot::String("SurfaceNetworkResource")
            : (node->get_surface_patch().is_valid() ? node->get_surface_patch()->get_patch_type_label() : godot::String("None"));
        lbl->set_text(
            "SurfaceMeshNode3D  |  Source: " + src +
            "  |  Active LOD: " + godot::String::num_int64(node->get_lod_level())
        );
    }
}

void SurfacesInspector::_on_open_in_composer_pressed() {
    if (current_object_id_.is_null()) return;
    if (godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_)) {
        if (auto *res = godot::Object::cast_to<godot::Resource>(obj)) {
            SurfacesComposer::open_surface_resource(godot::Ref<godot::Resource>(res));
        } else if (auto *node = godot::Object::cast_to<SurfaceMeshNode3D>(obj)) {
            if (node->get_surface_network().is_valid()) {
                SurfacesComposer::open_surface_resource(node->get_surface_network());
            } else if (node->get_surface_patch().is_valid()) {
                SurfacesComposer::open_surface_resource(node->get_surface_patch());
            }
        }
    }
}

void SurfacesInspector::_on_help_pressed() {
    godot::StringName topic_id("IdeamSurfaces");
    godot::Dictionary ctx;
    if (!current_object_id_.is_null()) {
        if (godot::Object *obj = godot::ObjectDB::get_instance(current_object_id_)) {
            ctx["object_class"] = obj->get_class();
            if (const auto *patch = godot::Object::cast_to<SurfacePatchResource>(obj)) {
                topic_id = patch->get_help_topic_id();
            } else if (const auto *net = godot::Object::cast_to<SurfaceNetworkResource>(obj)) {
                topic_id = net->get_help_topic_id();
            } else {
                topic_id = godot::StringName(obj->get_class());
            }
        }
    }
    ToolRegistry::get_singleton().dispatch_help_request(topic_id, "surfaces.inspector.header", ctx);
}

} // namespace ideam::godot_ext
