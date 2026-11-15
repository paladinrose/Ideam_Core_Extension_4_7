#include "data_oriented_characters.h"
#include <algorithm>
#include <span>

namespace ideam::godot_ext {

DataOrientedCharacters::DataOrientedCharacters()
    : dod_engine_(std::make_unique<core::CharactersDOD>()) {}

void DataOrientedCharacters::set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem) {
    memory_manager_resource_ = p_mem;
    emit_changed();
}

void DataOrientedCharacters::set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph) {
    task_graph_resource_ = p_graph;
    emit_changed();
}

void DataOrientedCharacters::set_active_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint) {
    active_blueprint_ = p_blueprint;
    emit_changed();
}

void DataOrientedCharacters::set_active_apparel(const godot::Ref<CharacterApparelResource> &p_apparel) {
    active_apparel_ = p_apparel;
    emit_changed();
}

void DataOrientedCharacters::set_max_vertices(int p_count) {
    max_vertices_ = std::clamp(p_count, 16, 65536);
    if (dod_engine_) {
        core::CharactersBufferCapacities caps = dod_engine_->get_capacities();
        caps.max_vertices     = static_cast<size_t>(max_vertices_);
        caps.max_quads        = static_cast<size_t>(max_vertices_);
        caps.max_morph_deltas = static_cast<size_t>(max_vertices_) * 4u;
        dod_engine_->set_capacities(caps);
    }
    emit_changed();
}

void DataOrientedCharacters::set_max_garment_vertices(int p_count) {
    max_garment_vertices_ = std::clamp(p_count, 16, 65536);
    if (dod_engine_) {
        core::CharactersBufferCapacities caps = dod_engine_->get_capacities();
        caps.max_garment_vertices = static_cast<size_t>(max_garment_vertices_);
        dod_engine_->set_capacities(caps);
    }
    emit_changed();
}

bool DataOrientedCharacters::initialize_dod_buffers() {
    if (!dod_engine_ || memory_manager_resource_.is_null()) {
        return false;
    }
    core::MemoryManagerDOD *mem = memory_manager_resource_->get_dod_manager();
    if (!mem) {
        return false;
    }
    core::TaskGraphDOD *graph = task_graph_resource_.is_valid()
        ? task_graph_resource_->get_dod_graph()
        : nullptr;
    dod_engine_->bind_execution_context(mem, graph);
    return dod_engine_->initialize_characters_memory(mem);
}

bool DataOrientedCharacters::stage_active_sources_to_dod() {
    if (!dod_engine_) return false;

    if (active_blueprint_.is_valid()) {
        dod_engine_->character_mesh() = active_blueprint_->get_core_mesh();
        dod_engine_->set_target_lod_level(static_cast<uint32_t>(active_blueprint_->get_subdivision_lod_level()));
        dod_engine_->set_active_partition_mask(static_cast<uint32_t>(active_blueprint_->get_active_partition_mask()));

        const godot::PackedFloat32Array gd_w = active_blueprint_->get_morph_weights();
        std::vector<float> w(static_cast<size_t>(gd_w.size()), 0.0f);
        for (int i = 0; i < gd_w.size(); ++i) {
            w[static_cast<size_t>(i)] = gd_w[i];
        }
        dod_engine_->set_morph_weights(std::span<const float>(w));
    }

    if (active_apparel_.is_valid()) {
        if (active_apparel_->get_garment_vertex_count() == 0 && active_blueprint_.is_valid()) {
            active_apparel_->generate_default_bindings_for_blueprint(active_blueprint_);
        }
        dod_engine_->set_garment_bindings(
            std::span<const core::SurfaceGarmentBindPod>(active_apparel_->get_garment_bindings())
        );
        dod_engine_->set_xpbd_relax_iterations(static_cast<uint32_t>(active_apparel_->get_xpbd_relax_iterations()));
        dod_engine_->set_xpbd_compliance(active_apparel_->get_xpbd_compliance());
    }

    return dod_engine_->stage_character_to_dod();
}

bool DataOrientedCharacters::execute_pipeline_direct(int p_stage_mask) {
    if (!dod_engine_) return false;
    return dod_engine_->execute_full_pipeline_direct(static_cast<uint32_t>(p_stage_mask));
}

godot::PackedVector3Array DataOrientedCharacters::read_deformed_positions() const {
    godot::PackedVector3Array out;
    if (!dod_engine_) return out;
    std::vector<glm::vec3> pos;
    dod_engine_->read_deformed_positions(pos);
    out.resize(static_cast<int>(pos.size()));
    for (size_t i = 0; i < pos.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(pos[i].x, pos[i].y, pos[i].z));
    }
    return out;
}

godot::PackedVector3Array DataOrientedCharacters::read_deformed_normals() const {
    godot::PackedVector3Array out;
    if (!dod_engine_) return out;
    std::vector<glm::vec3> nrm;
    dod_engine_->read_deformed_normals(nrm);
    out.resize(static_cast<int>(nrm.size()));
    for (size_t i = 0; i < nrm.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(nrm[i].x, nrm[i].y, nrm[i].z));
    }
    return out;
}

godot::PackedVector3Array DataOrientedCharacters::read_garment_positions() const {
    godot::PackedVector3Array out;
    if (!dod_engine_) return out;
    std::vector<glm::vec3> gpos;
    dod_engine_->read_garment_positions(gpos);
    out.resize(static_cast<int>(gpos.size()));
    for (size_t i = 0; i < gpos.size(); ++i) {
        out.set(static_cast<int>(i), godot::Vector3(gpos[i].x, gpos[i].y, gpos[i].z));
    }
    return out;
}

godot::PackedInt32Array DataOrientedCharacters::read_active_lod_masks() const {
    godot::PackedInt32Array out;
    if (!dod_engine_) return out;
    std::vector<uint32_t> masks;
    dod_engine_->read_active_lod_masks(masks);
    out.resize(static_cast<int>(masks.size()));
    for (size_t i = 0; i < masks.size(); ++i) {
        out.set(static_cast<int>(i), static_cast<int32_t>(masks[i]));
    }
    return out;
}

void DataOrientedCharacters::_bind_methods() {
    using namespace godot;

    ClassDB::bind_method(D_METHOD("set_memory_manager_resource", "mem"), &DataOrientedCharacters::set_memory_manager_resource);
    ClassDB::bind_method(D_METHOD("get_memory_manager_resource"), &DataOrientedCharacters::get_memory_manager_resource);

    ClassDB::bind_method(D_METHOD("set_task_graph_resource", "graph"), &DataOrientedCharacters::set_task_graph_resource);
    ClassDB::bind_method(D_METHOD("get_task_graph_resource"), &DataOrientedCharacters::get_task_graph_resource);

    ClassDB::bind_method(D_METHOD("set_active_blueprint", "blueprint"), &DataOrientedCharacters::set_active_blueprint);
    ClassDB::bind_method(D_METHOD("get_active_blueprint"), &DataOrientedCharacters::get_active_blueprint);

    ClassDB::bind_method(D_METHOD("set_active_apparel", "apparel"), &DataOrientedCharacters::set_active_apparel);
    ClassDB::bind_method(D_METHOD("get_active_apparel"), &DataOrientedCharacters::get_active_apparel);

    ClassDB::bind_method(D_METHOD("set_max_vertices", "count"), &DataOrientedCharacters::set_max_vertices);
    ClassDB::bind_method(D_METHOD("get_max_vertices"), &DataOrientedCharacters::get_max_vertices);

    ClassDB::bind_method(D_METHOD("set_max_garment_vertices", "count"), &DataOrientedCharacters::set_max_garment_vertices);
    ClassDB::bind_method(D_METHOD("get_max_garment_vertices"), &DataOrientedCharacters::get_max_garment_vertices);

    ClassDB::bind_method(D_METHOD("initialize_dod_buffers"), &DataOrientedCharacters::initialize_dod_buffers);
    ClassDB::bind_method(D_METHOD("stage_active_sources_to_dod"), &DataOrientedCharacters::stage_active_sources_to_dod);
    ClassDB::bind_method(
        D_METHOD("execute_pipeline_direct", "stage_mask"),
        &DataOrientedCharacters::execute_pipeline_direct,
        DEFVAL(static_cast<int>(core::CHARACTERS_STAGE_DEFAULT_PIPELINE_MASK))
    );

    ClassDB::bind_method(D_METHOD("read_deformed_positions"), &DataOrientedCharacters::read_deformed_positions);
    ClassDB::bind_method(D_METHOD("read_deformed_normals"), &DataOrientedCharacters::read_deformed_normals);
    ClassDB::bind_method(D_METHOD("read_garment_positions"), &DataOrientedCharacters::read_garment_positions);
    ClassDB::bind_method(D_METHOD("read_active_lod_masks"), &DataOrientedCharacters::read_active_lod_masks);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "memory_manager_resource", PROPERTY_HINT_RESOURCE_TYPE, "MemoryManagerResource"), "set_memory_manager_resource", "get_memory_manager_resource");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "task_graph_resource", PROPERTY_HINT_RESOURCE_TYPE, "TaskGraphResource"), "set_task_graph_resource", "get_task_graph_resource");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_blueprint", PROPERTY_HINT_RESOURCE_TYPE, "CharacterBlueprintResource"), "set_active_blueprint", "get_active_blueprint");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "active_apparel", PROPERTY_HINT_RESOURCE_TYPE, "CharacterApparelResource"), "set_active_apparel", "get_active_apparel");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_vertices", PROPERTY_HINT_RANGE, "16,65536,1"), "set_max_vertices", "get_max_vertices");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "max_garment_vertices", PROPERTY_HINT_RANGE, "16,65536,1"), "set_max_garment_vertices", "get_max_garment_vertices");
}

} // namespace ideam::godot_ext
