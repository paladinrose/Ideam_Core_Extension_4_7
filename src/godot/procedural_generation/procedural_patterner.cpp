#include "procedural_patterner.h"

namespace ideam::godot_ext {

void ProceduralPatterner::_bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("set_analysis_style", "style"), &ProceduralPatterner::set_analysis_style);
    ClassDB::bind_method(D_METHOD("get_analysis_style"), &ProceduralPatterner::get_analysis_style);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "analysis_style", PROPERTY_HINT_RESOURCE_TYPE, "AnalysisStyle"),
        "set_analysis_style",
        "get_analysis_style"
    );

    ClassDB::bind_method(
        D_METHOD("create_pattern_from_packed_scene", "scene"),
        &ProceduralPatterner::create_pattern_from_packed_scene
    );
    ClassDB::bind_method(
        D_METHOD("create_pattern_from_object", "root_object"),
        &ProceduralPatterner::create_pattern_from_object
    );
}

godot::Ref<ProceduralPattern> ProceduralPatterner::create_pattern_from_packed_scene(
    const godot::Ref<godot::PackedScene> &p_scene
) const {
    (void)p_scene;
    godot::Ref<ProceduralPattern> pattern;
    pattern.instantiate();
    // Architectural stub: Full SceneState traversal guided by cascading AnalysisStyle
    // will populate pattern->add_generation_rule(...) here.
    return pattern;
}

godot::Ref<ProceduralPattern> ProceduralPatterner::create_pattern_from_object(
    godot::Object *p_root_object
) const {
    (void)p_root_object;
    godot::Ref<ProceduralPattern> pattern;
    pattern.instantiate();
    // Architectural stub: Live Godot Object/Node/Resource reflection traversal
    // guided by cascading AnalysisStyle will populate pattern here.
    return pattern;
}

} // namespace ideam::godot_ext
