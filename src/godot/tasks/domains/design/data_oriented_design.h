#pragma once

/**
 * data_oriented_design.h
 * Godot `Resource` bridge for `DesignDOD` (`src/godot/tasks/domains/design/`).
 */

#include "../../../../core/tasks/domains/design/design_dod.h"
#include "../../../design/blueprint_resource.h"
#include "../../../design/container_3d.h"
#include "../../../design/parametric_assembly_resource.h"
#include "../../../design/socket_3d.h"
#include "../../../design/style_3d_resource.h"
#include "../../../memory/memory_manager_resource.h"
#include "../../../tasks/task_graph_resource.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <memory>

namespace ideam::godot_ext {

class DataOrientedDesign : public godot::Resource {
    GDCLASS(DataOrientedDesign, godot::Resource)

private:
    std::unique_ptr<core::DesignDOD> dod_engine_;

    godot::Ref<MemoryManagerResource> memory_manager_resource_;
    godot::Ref<TaskGraphResource> task_graph_resource_;
    godot::Ref<Theme3DResource> active_theme_3d_;
    godot::Ref<ParametricAssemblyRecipeResource> active_recipe_;
    godot::Ref<BlueprintResource> active_blueprint_;

    int max_structure_nodes_ = 256;
    float dream_morph_factor_ = 0.0f;

protected:
    static void _bind_methods();

public:
    DataOrientedDesign();
    ~DataOrientedDesign() override = default;

    [[nodiscard]] core::DesignDOD *get_dod_engine() const noexcept { return dod_engine_.get(); }

    void set_memory_manager_resource(const godot::Ref<MemoryManagerResource> &p_mem);
    [[nodiscard]] godot::Ref<MemoryManagerResource> get_memory_manager_resource() const { return memory_manager_resource_; }

    void set_task_graph_resource(const godot::Ref<TaskGraphResource> &p_graph);
    [[nodiscard]] godot::Ref<TaskGraphResource> get_task_graph_resource() const { return task_graph_resource_; }

    void set_active_theme_3d(const godot::Ref<Theme3DResource> &p_theme);
    [[nodiscard]] godot::Ref<Theme3DResource> get_active_theme_3d() const { return active_theme_3d_; }

    void set_active_recipe(const godot::Ref<ParametricAssemblyRecipeResource> &p_recipe);
    [[nodiscard]] godot::Ref<ParametricAssemblyRecipeResource> get_active_recipe() const { return active_recipe_; }

    void set_active_blueprint(const godot::Ref<BlueprintResource> &p_blueprint);
    [[nodiscard]] godot::Ref<BlueprintResource> get_active_blueprint() const { return active_blueprint_; }

    void set_max_structure_nodes(int p_count);
    [[nodiscard]] int get_max_structure_nodes() const noexcept { return max_structure_nodes_; }

    void set_dream_morph_factor(float p_factor);
    [[nodiscard]] float get_dream_morph_factor() const noexcept { return dream_morph_factor_; }

    // --- DOD Arena Setup, Staging & Execution ---
    bool initialize_dod_buffers();
    bool stage_active_sources_to_dod();
    bool execute_pipeline_direct(int p_stage_mask = static_cast<int>(core::DESIGN_STAGE_DEFAULT_PIPELINE_MASK));

    [[nodiscard]] godot::Array read_structure_nodes_as_dicts() const;
    [[nodiscard]] godot::Array read_sockets_as_dicts() const;
    [[nodiscard]] godot::Array read_aperture_cutouts_as_dicts() const;
    [[nodiscard]] godot::PackedInt32Array read_node_status_masks() const;
};

} // namespace ideam::godot_ext
