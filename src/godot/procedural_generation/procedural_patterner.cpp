#include "procedural_patterner.h"
#include "../linguistics/metadata_linguistics_manager.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_state.hpp>
#include <vector>

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
    ClassDB::bind_method(
        D_METHOD("create_pattern_from_hierarchy_dicts", "hierarchy_nodes"),
        &ProceduralPatterner::create_pattern_from_hierarchy_dicts
    );
}

godot::Ref<ProceduralPattern> ProceduralPatterner::create_pattern_from_packed_scene(
    const godot::Ref<godot::PackedScene> &p_scene
) const {
    godot::Ref<ProceduralPattern> pattern;
    pattern.instantiate();
    if (p_scene.is_null()) return pattern;

    const godot::Ref<godot::SceneState> state = p_scene->get_state();
    if (state.is_null() || state->get_node_count() <= 0) return pattern;

    std::vector<core::AnalysisRulePOD> flat_rules;
    godot::String eff_class;
    godot::String eff_suffix;
    if (analysis_style_.is_valid()) {
        analysis_style_->compile_flattened_rules(flat_rules, &eff_class, &eff_suffix);
    }

    const int32_t node_count = state->get_node_count();
    std::vector<core::HarvestedSceneNodeDescriptor> harvested;
    harvested.reserve(static_cast<size_t>(node_count));
    std::vector<godot::String> node_paths;
    node_paths.reserve(static_cast<size_t>(node_count));

    for (int32_t i = 0; i < node_count; ++i) {
        const godot::String path_str = godot::String(state->get_node_path(i));
        node_paths.push_back(path_str);

        core::HarvestedSceneNodeDescriptor desc{};
        desc.class_name = godot::String(state->get_node_type(i)).utf8().get_data();
        desc.node_name  = godot::String(state->get_node_name(i)).utf8().get_data();
        desc.parent_index = -1;

        // Resolve parent index in SceneState by matching parent path prefix
        if (i > 0) {
            const int64_t last_slash = path_str.rfind("/");
            const godot::String parent_path = (last_slash > 0) ? path_str.substr(0, last_slash) : godot::String(".");
            for (int32_t p = i - 1; p >= 0; --p) {
                if (node_paths[static_cast<size_t>(p)] == parent_path) {
                    desc.parent_index = p;
                    break;
                }
            }
        }

        const godot::PackedStringArray groups = state->get_node_groups(i);
        for (int g = 0; g < groups.size(); ++g) {
            const uint32_t gh = core::hash_reflection_symbol(groups[g].utf8().get_data());
            desc.group_or_tag_mask |= (1u << (gh & 31u));
        }

        const int32_t prop_cnt = state->get_node_property_count(i);
        for (int32_t k = 0; k < prop_cnt; ++k) {
            const godot::String pname(state->get_node_property_name(i, k));
            const uint32_t ph = core::hash_reflection_symbol(pname.utf8().get_data());
            desc.property_trait_mask |= (1u << (ph & 31u));
        }

        harvested.push_back(std::move(desc));
    }

    core::ProceduralEnvironment env;
    env.synthesize_pattern_from_harvested_nodes(
        harvested,
        flat_rules,
        eff_class.utf8().get_data(),
        eff_suffix.utf8().get_data()
    );
    pattern->import_from_environment(env);
    return pattern;
}

godot::Ref<ProceduralPattern> ProceduralPatterner::create_pattern_from_object(
    godot::Object *p_root_object
) const {
    godot::Ref<ProceduralPattern> pattern;
    pattern.instantiate();
    if (!p_root_object) return pattern;

    std::vector<core::AnalysisRulePOD> flat_rules;
    godot::String eff_class;
    godot::String eff_suffix;
    if (analysis_style_.is_valid()) {
        analysis_style_->compile_flattened_rules(flat_rules, &eff_class, &eff_suffix);
    }

    std::vector<core::HarvestedSceneNodeDescriptor> harvested;

    auto harvest_recursive = [&](auto &self, godot::Object *p_obj, int32_t p_parent_idx) -> void {
        if (!p_obj) return;

        core::HarvestedSceneNodeDescriptor desc{};
        desc.class_name   = p_obj->get_class().utf8().get_data();
        desc.parent_index = p_parent_idx;

        if (auto *node = godot::Object::cast_to<godot::Node>(p_obj)) {
            desc.node_name = godot::String(node->get_name()).utf8().get_data();
            const godot::TypedArray<godot::StringName> groups = node->get_groups();
            for (int g = 0; g < groups.size(); ++g) {
                const godot::String gs(groups[g]);
                const uint32_t gh = core::hash_reflection_symbol(gs.utf8().get_data());
                desc.group_or_tag_mask |= (1u << (gh & 31u));
            }
        } else {
            desc.node_name = desc.class_name;
        }

        // Check attached IdeamLinguistics metadata if present
        if (MetadataLinguisticsManager::has_linguistic_metadata(p_obj)) {
            const godot::Ref<SemanticTagSetResource> tags =
                MetadataLinguisticsManager::extract_tag_set_from_metadata(p_obj);
            if (tags.is_valid()) {
                desc.nuclear_seme_mask = static_cast<uint32_t>(
                    tags->get_core_descriptor().nuclear_seme_words[0] & 0xFFFFFFFFULL
                );
                desc.peripheral_seme_mask = static_cast<uint32_t>(
                    tags->get_core_descriptor().peripheral_seme_words[0] & 0xFFFFFFFFULL
                );
            }
        }

        const int32_t my_idx = static_cast<int32_t>(harvested.size());
        harvested.push_back(std::move(desc));

        if (auto *node = godot::Object::cast_to<godot::Node>(p_obj)) {
            const int32_t child_cnt = node->get_child_count();
            for (int32_t c = 0; c < child_cnt; ++c) {
                self(self, node->get_child(c), my_idx);
            }
        }
    };

    harvest_recursive(harvest_recursive, p_root_object, -1);

    core::ProceduralEnvironment env;
    env.synthesize_pattern_from_harvested_nodes(
        harvested,
        flat_rules,
        eff_class.utf8().get_data(),
        eff_suffix.utf8().get_data()
    );
    pattern->import_from_environment(env);
    return pattern;
}

godot::Ref<ProceduralPattern> ProceduralPatterner::create_pattern_from_hierarchy_dicts(
    const godot::Array &p_hierarchy_nodes
) const {
    godot::Ref<ProceduralPattern> pattern;
    pattern.instantiate();
    if (p_hierarchy_nodes.is_empty()) return pattern;

    std::vector<core::AnalysisRulePOD> flat_rules;
    godot::String eff_class;
    godot::String eff_suffix;
    if (analysis_style_.is_valid()) {
        analysis_style_->compile_flattened_rules(flat_rules, &eff_class, &eff_suffix);
    }

    std::vector<core::HarvestedSceneNodeDescriptor> harvested;
    harvested.reserve(static_cast<size_t>(p_hierarchy_nodes.size()));
    for (int i = 0; i < p_hierarchy_nodes.size(); ++i) {
        const godot::Dictionary d = p_hierarchy_nodes[i];
        core::HarvestedSceneNodeDescriptor desc{};
        desc.class_name   = godot::String(d.get("class_name", "Node3D")).utf8().get_data();
        desc.node_name    = godot::String(d.get("node_name", "")).utf8().get_data();
        desc.parent_index = static_cast<int32_t>(d.get("parent_index", -1));
        harvested.push_back(std::move(desc));
    }

    core::ProceduralEnvironment env;
    env.synthesize_pattern_from_harvested_nodes(
        harvested,
        flat_rules,
        eff_class.utf8().get_data(),
        eff_suffix.utf8().get_data()
    );
    pattern->import_from_environment(env);
    return pattern;
}

} // namespace ideam::godot_ext
