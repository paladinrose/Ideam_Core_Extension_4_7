#include "design_dod.h"
#include <algorithm>
#include <cstring>

namespace ideam::core {

namespace {

template <typename T_Elem>
bool copy_span_to_buffer(MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::span<const T_Elem> p_src) {
    if (!p_mem || p_buf_id == INVALID_ID) return false;
    MemoryBufferPOD *buf = p_mem->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr) return false;
    const size_t max_elems = static_cast<size_t>(std::max<int64_t>(0, buf->max_elements));
    const size_t count     = std::min<size_t>(p_src.size(), max_elems);
    if (count > 0) {
        std::memcpy(buf->master_block_ptr + buf->memory_offset, p_src.data(), count * sizeof(T_Elem));
    }
    buf->current_count = static_cast<int64_t>(count);
    return true;
}

template <typename T_Elem>
void read_buffer_to_vector(const MemoryManagerDOD *p_mem, uint32_t p_buf_id, std::vector<T_Elem> &r_out) {
    r_out.clear();
    if (!p_mem || p_buf_id == INVALID_ID) return;
    const MemoryBufferPOD *buf = const_cast<MemoryManagerDOD *>(p_mem)->get_buffer(p_buf_id);
    if (!buf || !buf->master_block_ptr || buf->current_count <= 0) return;
    const size_t count = static_cast<size_t>(std::min<int64_t>(buf->current_count, buf->max_elements));
    r_out.resize(count);
    std::memcpy(r_out.data(), buf->master_block_ptr + buf->memory_offset, count * sizeof(T_Elem));
}

} // namespace

bool DesignDOD::initialize_design_memory(MemoryManagerDOD *p_mem) {
    if (!p_mem) return false;
    memory_manager_ = p_mem;

    auto alloc_buf = [&](const char *, size_t p_elem_count, size_t p_elem_bytes, DataType p_dtype) -> uint32_t {
        const size_t total_bytes = std::max<size_t>(1u, p_elem_count) * p_elem_bytes;
        const uint32_t id = p_mem->create_buffer(BufferLayoutType::FLAT, total_bytes, 64u);
        if (MemoryBufferPOD *buf = p_mem->get_buffer(id)) {
            buf->max_elements  = static_cast<int64_t>(std::max<size_t>(1u, p_elem_count));
            buf->current_count = 0;
        }
        return id;
    };

    manifest_.set(
        DesignBufferSlot::STRUCTURE_NODE_POD,
        alloc_buf("DSG_StructureNodes", capacities_.max_structure_nodes, sizeof(StructureNodePod), DataType::BYTE)
    );
    manifest_.set(
        DesignBufferSlot::SOCKET_POD,
        alloc_buf("DSG_Sockets", capacities_.max_sockets, sizeof(SocketPod), DataType::BYTE)
    );
    manifest_.set(
        DesignBufferSlot::STYLE_3D_RULE_POD,
        alloc_buf("DSG_Style3DRules", capacities_.max_style_rules, sizeof(Style3DRulePod), DataType::BYTE)
    );
    manifest_.set(
        DesignBufferSlot::APERTURE_CUTOUT_POD,
        alloc_buf("DSG_ApertureCutouts", capacities_.max_aperture_cutouts, sizeof(ApertureCutoutPod), DataType::BYTE)
    );
    manifest_.set(
        DesignBufferSlot::NODE_STATUS_MASK,
        alloc_buf("DSG_NodeStatusMasks", capacities_.max_structure_nodes, sizeof(uint32_t), DataType::INT32)
    );

    return manifest_.is_valid();
}

bool DesignDOD::stage_solver_to_dod(const StructureLayoutSolver &p_solver) {
    if (!memory_manager_) return false;
    if (!manifest_.is_valid()) {
        if (!initialize_design_memory(memory_manager_)) return false;
    }

    copy_span_to_buffer(memory_manager_, manifest_.get(DesignBufferSlot::STRUCTURE_NODE_POD), std::span<const StructureNodePod>(p_solver.nodes()));
    copy_span_to_buffer(memory_manager_, manifest_.get(DesignBufferSlot::SOCKET_POD), std::span<const SocketPod>(p_solver.sockets()));
    copy_span_to_buffer(memory_manager_, manifest_.get(DesignBufferSlot::STYLE_3D_RULE_POD), std::span<const Style3DRulePod>(p_solver.style_rules()));
    copy_span_to_buffer(memory_manager_, manifest_.get(DesignBufferSlot::APERTURE_CUTOUT_POD), std::span<const ApertureCutoutPod>(p_solver.aperture_cutouts()));

    if (MemoryBufferPOD *cut_buf = memory_manager_->get_buffer(manifest_.get(DesignBufferSlot::APERTURE_CUTOUT_POD))) {
        cut_buf->current_count = static_cast<int64_t>(capacities_.max_aperture_cutouts);
    }
    if (MemoryBufferPOD *mask_buf = memory_manager_->get_buffer(manifest_.get(DesignBufferSlot::NODE_STATUS_MASK))) {
        mask_buf->current_count = static_cast<int64_t>(p_solver.nodes().size());
    }
    return true;
}

bool DesignDOD::execute_full_pipeline_direct(uint32_t p_stage_mask) {
    if (!memory_manager_ || !manifest_.is_valid()) return false;

    TaskContextPOD ctx{};
    ctx.manager = memory_manager_;
    SingleElementView<uint32_t, FlatStrategy> dummy_view{};

    if ((p_stage_mask & DESIGN_STAGE_PARAMETRIC_ASSEMBLY_MORPH) != 0u) {
        ParametricAssemblyMorphMetadataLogic<> dsg4{};
        dsg4.target_buffer_id      = manifest_.get(DesignBufferSlot::STRUCTURE_NODE_POD);
        dsg4.socket_buffer_id      = manifest_.get(DesignBufferSlot::SOCKET_POD);
        dsg4.status_mask_buffer_id = manifest_.get(DesignBufferSlot::NODE_STATUS_MASK);
        dsg4.dream_morph_factor    = dream_morph_factor_;
        dsg4.seat_height_override  = seat_height_override_;
        dsg4.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    if ((p_stage_mask & DESIGN_STAGE_STYLE_3D_THEME_RESKIN) != 0u) {
        Style3DThemeReskinTransformLogic<> dsg2{};
        dsg2.target_buffer_id     = manifest_.get(DesignBufferSlot::STRUCTURE_NODE_POD);
        dsg2.style_rule_buffer_id = manifest_.get(DesignBufferSlot::STYLE_3D_RULE_POD);
        dsg2.enforce_cascade_sort = 1u;
        dsg2.global_scale_factor  = global_scale_factor_;
        dsg2.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    if ((p_stage_mask & DESIGN_STAGE_STRUCTURE_SOCKET_SOLVE) != 0u) {
        StructureSocketConstraintSolveTransformLogic<> dsg1{};
        dsg1.target_buffer_id      = manifest_.get(DesignBufferSlot::STRUCTURE_NODE_POD);
        dsg1.socket_buffer_id      = manifest_.get(DesignBufferSlot::SOCKET_POD);
        dsg1.status_mask_buffer_id = manifest_.get(DesignBufferSlot::NODE_STATUS_MASK);
        dsg1.solve_sockets_first   = 1u;
        dsg1.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    if ((p_stage_mask & DESIGN_STAGE_APERTURE_HOST_CUTOUT_BRIDGE) != 0u) {
        ApertureHostCutoutBridgeTransformLogic<> dsg3{};
        dsg3.target_buffer_id          = manifest_.get(DesignBufferSlot::STRUCTURE_NODE_POD);
        dsg3.socket_buffer_id          = manifest_.get(DesignBufferSlot::SOCKET_POD);
        dsg3.aperture_cutout_buffer_id = manifest_.get(DesignBufferSlot::APERTURE_CUTOUT_POD);
        dsg3.spawn_door_hinge_sockets  = 1u;
        dsg3.execute<decltype(dummy_view), FlatStrategy>(ctx, dummy_view);
    }

    sync_dod_back_to_solver();
    return true;
}

void DesignDOD::read_structure_nodes(std::vector<StructureNodePod> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(DesignBufferSlot::STRUCTURE_NODE_POD), r_out);
}

void DesignDOD::read_sockets(std::vector<SocketPod> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(DesignBufferSlot::SOCKET_POD), r_out);
}

void DesignDOD::read_style_rules(std::vector<Style3DRulePod> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(DesignBufferSlot::STYLE_3D_RULE_POD), r_out);
}

void DesignDOD::read_aperture_cutouts(std::vector<ApertureCutoutPod> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(DesignBufferSlot::APERTURE_CUTOUT_POD), r_out);
}

void DesignDOD::read_node_status_masks(std::vector<uint32_t> &r_out) const {
    read_buffer_to_vector(memory_manager_, manifest_.get(DesignBufferSlot::NODE_STATUS_MASK), r_out);
}

void DesignDOD::sync_dod_back_to_solver() {
    if (!memory_manager_ || !manifest_.is_valid()) return;

    std::vector<StructureNodePod> updated_nodes;
    read_structure_nodes(updated_nodes);
    if (!updated_nodes.empty()) {
        solver_.nodes_mut() = std::move(updated_nodes);
    }

    std::vector<SocketPod> updated_sockets;
    read_sockets(updated_sockets);
    if (!updated_sockets.empty()) {
        solver_.sockets_mut() = std::move(updated_sockets);
    }

    std::vector<Style3DRulePod> updated_rules;
    read_style_rules(updated_rules);
    if (!updated_rules.empty()) {
        solver_.style_rules_mut() = std::move(updated_rules);
    }

    std::vector<ApertureCutoutPod> raw_cutouts;
    read_aperture_cutouts(raw_cutouts);
    solver_.aperture_cutouts_mut().clear();
    for (const auto &c : raw_cutouts) {
        if (c.is_applied != 0u && c.aperture_node_id != 0xFFFFFFFFu) {
            solver_.aperture_cutouts_mut().push_back(c);
        }
    }
}

} // namespace ideam::core
