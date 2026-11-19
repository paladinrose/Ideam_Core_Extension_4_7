#include "blueprint_resource.h"
#include "ideam_design_plugin.h"
#include <algorithm>

namespace ideam::godot_ext {

void BlueprintResource::_sync_core_from_serialized() {
    core_blueprint_.clear();

    for (int i = 0; i < serialized_spaces_.size(); ++i) {
        const godot::Dictionary d = serialized_spaces_[i];
        const godot::String name = d.get("room_name", "Room");
        const godot::PackedVector2Array pts = d.get("vertices_2d", godot::PackedVector2Array());
        if (pts.size() < 3) {
            continue;
        }
        std::vector<glm::vec2> verts;
        verts.reserve(static_cast<size_t>(pts.size()));
        for (int j = 0; j < pts.size(); ++j) {
            const godot::Vector2 v = pts[j];
            verts.emplace_back(v.x, v.y);
        }
        const int role_id    = d.get("semantic_role_id", static_cast<int>(core::SEMANTIC_ROLE_ASSEMBLY_ROOM));
        const int seme_mask  = d.get("nuclear_seme_mask", 0);
        const float floor_el = d.get("floor_elevation", 0.0f);
        const float ceil_h   = d.get("ceiling_height", 3.0f);
        godot::CharString cs = name.utf8();
        core_blueprint_.add_space_polygon(
            cs.get_data() ? cs.get_data() : "Room",
            verts,
            static_cast<uint32_t>(role_id),
            static_cast<uint32_t>(seme_mask),
            floor_el,
            ceil_h
        );
    }

    for (int i = 0; i < serialized_features_.size(); ++i) {
        const godot::Dictionary d = serialized_features_[i];
        const int sym_type   = std::clamp(static_cast<int>(d.get("symbol_type", 0)), 0, 5);
        const godot::Vector2 pos = d.get("position_2d", godot::Vector2(0.0f, 0.0f));
        const float width_m  = d.get("width_meters", 0.90f);
        const float height_m = d.get("height_meters", 2.10f);
        const float sill_el  = d.get("sill_elevation", 0.0f);
        const godot::String label = d.get("map_key_label", "");
        godot::CharString cs = label.utf8();
        core_blueprint_.add_feature_symbol(
            static_cast<core::BlueprintFeatureSymbolType>(sym_type),
            glm::vec2(pos.x, pos.y),
            width_m,
            height_m,
            sill_el,
            cs.get_data() ? cs.get_data() : ""
        );
    }

    core_blueprint_.rebuild_reactive_layers();
}

void BlueprintResource::_sync_serialized_from_core() {
    serialized_spaces_.clear();
    for (const auto &sp : core_blueprint_.spaces()) {
        godot::Dictionary d;
        d["room_name"]         = godot::String(sp.room_name.c_str());
        d["semantic_role_id"]  = static_cast<int>(sp.semantic_role_id);
        d["nuclear_seme_mask"] = static_cast<int>(sp.nuclear_seme_mask);
        d["floor_elevation"]   = sp.floor_elevation;
        d["ceiling_height"]    = sp.ceiling_height;
        godot::PackedVector2Array pts;
        pts.resize(static_cast<int64_t>(sp.vertices_2d.size()));
        for (size_t i = 0; i < sp.vertices_2d.size(); ++i) {
            pts.set(static_cast<int64_t>(i), godot::Vector2(sp.vertices_2d[i].x, sp.vertices_2d[i].y));
        }
        d["vertices_2d"] = pts;
        serialized_spaces_.push_back(d);
    }

    serialized_features_.clear();
    for (const auto &feat : core_blueprint_.features()) {
        godot::Dictionary d;
        d["symbol_type"]    = static_cast<int>(feat.symbol_type);
        d["position_2d"]    = godot::Vector2(feat.position_2d.x, feat.position_2d.y);
        d["width_meters"]   = feat.width_meters;
        d["height_meters"]  = feat.height_meters;
        d["sill_elevation"] = feat.sill_elevation;
        d["map_key_label"]  = godot::String("");
        serialized_features_.push_back(d);
    }
}

void BlueprintResource::set_section_cut_height_meters(float p_height) {
    section_cut_height_meters_ = std::max(0.1f, p_height);
    emit_changed();
}

void BlueprintResource::set_spaces(const godot::TypedArray<godot::Dictionary> &p_spaces) {
    serialized_spaces_ = p_spaces;
    _sync_core_from_serialized();
    emit_changed();
}

void BlueprintResource::set_features(const godot::TypedArray<godot::Dictionary> &p_features) {
    serialized_features_ = p_features;
    _sync_core_from_serialized();
    emit_changed();
}

void BlueprintResource::commit_set_section_cut_height_meters(
    float p_new_height,
    const godot::Ref<IdeamUndoRedo> &p_undo_redo
) {
    godot::Ref<IdeamUndoRedo> ur = p_undo_redo.is_valid()
        ? p_undo_redo
        : IdeamDesignPlugin::create_undo_redo_wrapper();
    if (ur.is_valid()) {
        const float old_h = section_cut_height_meters_;
        ur->create_action("Set Blueprint Section Cut Height");
        ur->add_do_property(this, "section_cut_height_meters", p_new_height);
        ur->add_undo_property(this, "section_cut_height_meters", old_h);
        ur->commit_action();
    } else {
        set_section_cut_height_meters(p_new_height);
    }
}

int BlueprintResource::add_space_polygon(
    const godot::String &p_room_name,
    const godot::PackedVector2Array &p_vertices_2d,
    int p_semantic_role_id,
    int p_nuclear_seme_mask,
    float p_floor_elevation,
    float p_ceiling_height
) {
    std::vector<glm::vec2> verts;
    verts.reserve(static_cast<size_t>(p_vertices_2d.size()));
    for (int i = 0; i < p_vertices_2d.size(); ++i) {
        const godot::Vector2 v = p_vertices_2d[i];
        verts.emplace_back(v.x, v.y);
    }
    godot::CharString cs = p_room_name.utf8();
    uint32_t id = core_blueprint_.add_space_polygon(
        cs.get_data() ? cs.get_data() : "Room",
        verts,
        static_cast<uint32_t>(p_semantic_role_id),
        static_cast<uint32_t>(p_nuclear_seme_mask),
        p_floor_elevation,
        p_ceiling_height
    );
    _sync_serialized_from_core();
    emit_changed();
    return static_cast<int>(id);
}

int BlueprintResource::add_feature_symbol(
    int p_symbol_type,
    const godot::Vector2 &p_position_2d,
    float p_width_meters,
    float p_height_meters,
    float p_sill_elevation,
    const godot::String &p_map_key_label
) {
    godot::CharString cs = p_map_key_label.utf8();
    uint32_t id = core_blueprint_.add_feature_symbol(
        static_cast<core::BlueprintFeatureSymbolType>(std::clamp(p_symbol_type, 0, 5)),
        glm::vec2(p_position_2d.x, p_position_2d.y),
        p_width_meters,
        p_height_meters,
        p_sill_elevation,
        cs.get_data() ? cs.get_data() : ""
    );
    _sync_serialized_from_core();
    emit_changed();
    return static_cast<int>(id);
}

void BlueprintResource::populate_sample_room() {
    const int idx = get_space_count();
    const float offset_x = static_cast<float>(idx) * 5.0f;
    godot::PackedVector2Array pts;
    pts.push_back(godot::Vector2(offset_x, 0.0f));
    pts.push_back(godot::Vector2(offset_x + 5.0f, 0.0f));
    pts.push_back(godot::Vector2(offset_x + 5.0f, 4.0f));
    pts.push_back(godot::Vector2(offset_x, 4.0f));
    add_space_polygon("Room " + godot::String::num_int64(idx + 1), pts);
    rebuild_reactive_layers();
}

void BlueprintResource::clear_blueprint() {
    core_blueprint_.clear();
    serialized_spaces_.clear();
    serialized_features_.clear();
    emit_changed();
}

void BlueprintResource::rebuild_reactive_layers() {
    core_blueprint_.rebuild_reactive_layers();
    emit_changed();
}

int BlueprintResource::get_space_count() const noexcept {
    return static_cast<int>(core_blueprint_.spaces().size());
}

int BlueprintResource::get_wall_count() const noexcept {
    return static_cast<int>(core_blueprint_.walls().size());
}

int BlueprintResource::get_feature_count() const noexcept {
    return static_cast<int>(core_blueprint_.features().size());
}

int BlueprintResource::get_annotation_count() const noexcept {
    return static_cast<int>(core_blueprint_.annotations().size());
}

godot::Dictionary BlueprintResource::get_layer_style_info(int p_layer_index, bool p_is_exterior_wall) const {
    const auto style = core::ArchitecturalBlueprintCore::get_layer_visual_style(
        static_cast<core::BlueprintLayerType>(std::clamp(p_layer_index, 0, 3)),
        p_is_exterior_wall
    );
    godot::Dictionary d;
    d["stroke_weight_px"]   = style.stroke_weight_px;
    d["section_cut_height"] = style.section_cut_height;
    d["color_rgba"]         = godot::Color(style.color_rgba.r, style.color_rgba.g, style.color_rgba.b, style.color_rgba.a);
    return d;
}

godot::Array BlueprintResource::get_map_key_summary() const {
    godot::Array arr;
    for (const auto &entry : core_blueprint_.map_key()) {
        godot::Dictionary d;
        d["map_key_code"]   = static_cast<int>(entry.map_key_code);
        d["symbol_type"]    = static_cast<int>(entry.symbol_type);
        d["symbol_label"]   = godot::String(entry.symbol_label.c_str());
        d["instance_count"] = static_cast<int>(entry.instance_count);
        arr.push_back(d);
    }
    return arr;
}

godot::Dictionary BlueprintResource::extrude_to_3d_layout_summary() const {
    core::StructureLayoutSolver solver;
    core_blueprint_.extrude_to_structure_layout(solver);

    godot::Dictionary d;
    d["node_count"]   = static_cast<int>(solver.nodes().size());
    d["socket_count"] = static_cast<int>(solver.sockets().size());
    d["cutout_count"] = static_cast<int>(solver.aperture_cutouts().size());
    return d;
}

void BlueprintResource::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_section_cut_height_meters", "height"), &BlueprintResource::set_section_cut_height_meters);
    ClassDB::bind_method(D_METHOD("get_section_cut_height_meters"), &BlueprintResource::get_section_cut_height_meters);

    ClassDB::bind_method(D_METHOD("set_spaces", "spaces"), &BlueprintResource::set_spaces);
    ClassDB::bind_method(D_METHOD("get_spaces"), &BlueprintResource::get_spaces);
    ClassDB::bind_method(D_METHOD("set_features", "features"), &BlueprintResource::set_features);
    ClassDB::bind_method(D_METHOD("get_features"), &BlueprintResource::get_features);

    ClassDB::bind_method(
        D_METHOD("commit_set_section_cut_height_meters", "new_height", "undo_redo"),
        &BlueprintResource::commit_set_section_cut_height_meters,
        DEFVAL(Ref<IdeamUndoRedo>())
    );

    ClassDB::bind_method(
        D_METHOD("add_space_polygon", "room_name", "vertices_2d", "semantic_role_id", "nuclear_seme_mask", "floor_elevation", "ceiling_height"),
        &BlueprintResource::add_space_polygon,
        DEFVAL(static_cast<int>(core::SEMANTIC_ROLE_ASSEMBLY_ROOM)),
        DEFVAL(0),
        DEFVAL(0.0f),
        DEFVAL(3.0f)
    );
    ClassDB::bind_method(
        D_METHOD("add_feature_symbol", "symbol_type", "position_2d", "width_meters", "height_meters", "sill_elevation", "map_key_label"),
        &BlueprintResource::add_feature_symbol,
        DEFVAL(0.90f),
        DEFVAL(2.10f),
        DEFVAL(0.0f),
        DEFVAL("")
    );
    ClassDB::bind_method(D_METHOD("populate_sample_room"), &BlueprintResource::populate_sample_room);
    ClassDB::bind_method(D_METHOD("clear_blueprint"), &BlueprintResource::clear_blueprint);
    ClassDB::bind_method(D_METHOD("rebuild_reactive_layers"), &BlueprintResource::rebuild_reactive_layers);

    ClassDB::bind_method(D_METHOD("get_space_count"), &BlueprintResource::get_space_count);
    ClassDB::bind_method(D_METHOD("get_wall_count"), &BlueprintResource::get_wall_count);
    ClassDB::bind_method(D_METHOD("get_feature_count"), &BlueprintResource::get_feature_count);
    ClassDB::bind_method(D_METHOD("get_annotation_count"), &BlueprintResource::get_annotation_count);

    ClassDB::bind_method(D_METHOD("get_layer_style_info", "layer_index", "is_exterior_wall"), &BlueprintResource::get_layer_style_info, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("get_map_key_summary"), &BlueprintResource::get_map_key_summary);
    ClassDB::bind_method(D_METHOD("extrude_to_3d_layout_summary"), &BlueprintResource::extrude_to_3d_layout_summary);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "section_cut_height_meters"), "set_section_cut_height_meters", "get_section_cut_height_meters");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "spaces", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_spaces", "get_spaces");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "features", PROPERTY_HINT_ARRAY_TYPE, "Dictionary"), "set_features", "get_features");

    BIND_ENUM_CONSTANT(LAYER_SPACES);
    BIND_ENUM_CONSTANT(LAYER_WALLS);
    BIND_ENUM_CONSTANT(LAYER_FEATURES_AND_APERTURES);
    BIND_ENUM_CONSTANT(LAYER_LABELS_AND_MEASUREMENTS);

    BIND_ENUM_CONSTANT(SYMBOL_DOOR_SINGLE);
    BIND_ENUM_CONSTANT(SYMBOL_DOOR_DOUBLE);
    BIND_ENUM_CONSTANT(SYMBOL_WINDOW_SASH);
    BIND_ENUM_CONSTANT(SYMBOL_ARCHWAY);
    BIND_ENUM_CONSTANT(SYMBOL_STAIR_RUN);
    BIND_ENUM_CONSTANT(SYMBOL_FIXTURE_PROP);
}

// ============================================================================
// BlueprintCanvas2D
// ============================================================================

void BlueprintCanvas2D::_notification(int p_what) {
    using namespace godot;
    if (p_what == NOTIFICATION_DRAW && blueprint_.is_valid()) {
        const auto &core = blueprint_->get_core_blueprint();

        if (is_layer_visible(BlueprintResource::LAYER_SPACES)) {
            const auto style = core::ArchitecturalBlueprintCore::get_layer_visual_style(core::BlueprintLayerType::LAYER_SPACES);
            Color stroke(style.color_rgba.r, style.color_rgba.g, style.color_rgba.b, style.color_rgba.a);
            Color fill(style.color_rgba.r, style.color_rgba.g, style.color_rgba.b, 0.15f);
            for (const auto &sp : core.spaces()) {
                if (sp.vertices_2d.size() < 3) {
                    continue;
                }
                PackedVector2Array pts;
                for (const auto &v : sp.vertices_2d) {
                    pts.push_back(Vector2(v.x * pixels_per_meter_, v.y * pixels_per_meter_));
                }
                draw_colored_polygon(pts, fill);
                pts.push_back(pts[0]);
                draw_polyline(pts, stroke, style.stroke_weight_px, true);
            }
        }

        if (is_layer_visible(BlueprintResource::LAYER_WALLS)) {
            for (const auto &w : core.walls()) {
                const auto style = core::ArchitecturalBlueprintCore::get_layer_visual_style(
                    core::BlueprintLayerType::LAYER_WALLS,
                    w.is_exterior != 0u
                );
                Color stroke(style.color_rgba.r, style.color_rgba.g, style.color_rgba.b, style.color_rgba.a);
                draw_line(
                    Vector2(w.start_2d.x * pixels_per_meter_, w.start_2d.y * pixels_per_meter_),
                    Vector2(w.end_2d.x * pixels_per_meter_, w.end_2d.y * pixels_per_meter_),
                    stroke,
                    w.stroke_weight_px,
                    true
                );
            }
        }

        if (is_layer_visible(BlueprintResource::LAYER_FEATURES_AND_APERTURES)) {
            const auto style = core::ArchitecturalBlueprintCore::get_layer_visual_style(core::BlueprintLayerType::LAYER_FEATURES_AND_APERTURES);
            Color stroke(style.color_rgba.r, style.color_rgba.g, style.color_rgba.b, style.color_rgba.a);
            for (const auto &feat : core.features()) {
                draw_circle(
                    Vector2(feat.position_2d.x * pixels_per_meter_, feat.position_2d.y * pixels_per_meter_),
                    feat.width_meters * 0.5f * pixels_per_meter_,
                    stroke
                );
            }
        }
    }
}

void BlueprintCanvas2D::set_blueprint(const godot::Ref<BlueprintResource> &p_blueprint) {
    blueprint_ = p_blueprint;
    queue_redraw();
}

void BlueprintCanvas2D::set_pixels_per_meter(float p_ppm) {
    pixels_per_meter_ = std::max(1.0f, p_ppm);
    queue_redraw();
}

void BlueprintCanvas2D::set_visible_layer_mask(int p_mask) {
    visible_layer_mask_ = static_cast<uint32_t>(p_mask) & 0x0Fu;
    queue_redraw();
}

void BlueprintCanvas2D::set_layer_visible(int p_layer_index, bool p_visible) {
    if (p_layer_index < 0 || p_layer_index > 3) {
        return;
    }
    if (p_visible) {
        visible_layer_mask_ |= (1u << p_layer_index);
    } else {
        visible_layer_mask_ &= ~(1u << p_layer_index);
    }
    queue_redraw();
}

bool BlueprintCanvas2D::is_layer_visible(int p_layer_index) const noexcept {
    if (p_layer_index < 0 || p_layer_index > 3) {
        return false;
    }
    return (visible_layer_mask_ & (1u << p_layer_index)) != 0u;
}

void BlueprintCanvas2D::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_blueprint", "blueprint"), &BlueprintCanvas2D::set_blueprint);
    ClassDB::bind_method(D_METHOD("get_blueprint"), &BlueprintCanvas2D::get_blueprint);

    ClassDB::bind_method(D_METHOD("set_pixels_per_meter", "ppm"), &BlueprintCanvas2D::set_pixels_per_meter);
    ClassDB::bind_method(D_METHOD("get_pixels_per_meter"), &BlueprintCanvas2D::get_pixels_per_meter);

    ClassDB::bind_method(D_METHOD("set_visible_layer_mask", "mask"), &BlueprintCanvas2D::set_visible_layer_mask);
    ClassDB::bind_method(D_METHOD("get_visible_layer_mask"), &BlueprintCanvas2D::get_visible_layer_mask);

    ClassDB::bind_method(D_METHOD("set_layer_visible", "layer_index", "visible"), &BlueprintCanvas2D::set_layer_visible);
    ClassDB::bind_method(D_METHOD("is_layer_visible", "layer_index"), &BlueprintCanvas2D::is_layer_visible);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "blueprint", PROPERTY_HINT_RESOURCE_TYPE, "BlueprintResource"), "set_blueprint", "get_blueprint");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pixels_per_meter"), "set_pixels_per_meter", "get_pixels_per_meter");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "visible_layer_mask", PROPERTY_HINT_FLAGS, "Spaces,Walls,Features & Apertures,Labels & Measurements"), "set_visible_layer_mask", "get_visible_layer_mask");
}

} // namespace ideam::godot_ext
