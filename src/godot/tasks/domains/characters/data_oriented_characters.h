#pragma once

/**
 * data_oriented_characters.h
 * Godot `Resource` bridge for `CharactersDOD` (`src/godot/tasks/domains/characters/`).
 */

#include "../../../../core/tasks/domains/characters/characters_dod.h"
#include "../../../characters/character_blueprint_resource.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../tasks/task_graph_resource.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <memory>

namespace ideam::godot_ext {

class DataOrientedCharacters : public godot::Resource {
    GDCLASS(DataOrientedCharacters, godot::Resource)

private:
    std::unique_ptr<core::CharactersDOD> dod_engine_;

    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    godot::Ref<CharacterBlueprintResource> active_blueprint_;
    godot::Ref<CharacterApparelResource> active_apparel_;

    int max_vertices_         = 1024;
    int max_garment_vertices_ = 2048;

protected:
    static void _bind_methods();

public:
    DataOrientedCharacters();
    ~DataOrientedCharacters() override = default;

    [[nodiscard]] core::CharactersDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem);
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_active_blueprint(const godot::Ref<CharacterBlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<CharacterBlueprintResource> get_active_blueprint() const { return active_blueprint_; }

    void set_active_apparel(const godot::Ref<CharacterApparelResource> &p_apparel);
    [[nodiscard]] godot::Ref<CharacterApparelResource> get_active_apparel() const { return active_apparel_; }

    void set_max_vertices(int p_count);
    [[nodiscard]] int get_max_vertices() const noexcept { return max_vertices_; }

    void set_max_garment_vertices(int p_count);
    [[nodiscard]] int get_max_garment_vertices() const noexcept { return max_garment_vertices_; }

    // --- DOD Arena Setup, Staging & Execution (`CHR1 -> CHR2 -> CHR3`) ---
    bool initialize_dod_buffers();
    bool stage_active_sources_to_dod();
    bool execute_pipeline_direct(int p_stage_mask = static_cast<int>(core::CHARACTERS_STAGE_DEFAULT_PIPELINE_MASK));

    [[nodiscard]] godot::PackedVector3Array read_deformed_positions() const;
    [[nodiscard]] godot::PackedVector3Array read_deformed_normals() const;
    [[nodiscard]] godot::PackedVector3Array read_garment_positions() const;
    [[nodiscard]] godot::PackedInt32Array read_active_lod_masks() const;
};

} // namespace ideam::godot_ext
