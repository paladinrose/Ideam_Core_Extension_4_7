#pragma once

#include "../i_native_task.h"
#include "../../memory/memory_common.h"           // For DataType
#include "../../memory/memory_buffer_pod.h"       // For BufferLayoutType
#include "../../memory/views/view_traits.h"       // For ViewStrategies and ViewCapability

#include "../../../godot/tasks/task_resource.h" 
#include "../../../godot/tasks/task_graph_node.h" 
#include "task_manifest.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/classes/file_access.hpp>

#include <memory>
#include <functional>
#include <cstdint>

namespace ideam::core {

enum class MemoryView : uint32_t {
    SingleElementView = 0,
    MultiElementView,
    SparseSetView,
    PagedView,
    RingView,
    StencilView,
    AtomicView,
    SwapView,
    StaticStencilView,
    BridgeView,
    AOSOA_Tight_AVX2,
    AOSOA_Tight_AVX512,
    AOSOA_STD430_AVX2,
    AOSOA_STD430_AVX512,
    AOSOA_STD140_AVX2,
    AOSOA_STD140_AVX512,
    Count
};

enum class MemoryStrategy : uint32_t {
    FlatStrategy = 0,
    SoAStrategy,
    AoSStrategy,
    Spatial2DStrategy,
    Spatial3DStrategy,
    Spatial4DStrategy,
    TiledSoAStrategy,
    RingStrategy,
    PagedStrategy, 
    Count
};

enum class MemoryTypes : uint32_t {
    BOOL = 0,
    BYTE,
    INT32,
    INT64,
    FLOAT32,
    FLOAT64,
    VECTOR2,
    VECTOR3,
    VECTOR4,
    VECTOR2I,
    VECTOR3I,
    VECTOR4I,
    VECTOR2D,
    VECTOR3D,
    VECTOR4D,
    COLOR,
    CUSTOM,
    Count
};

// --- The Translation Layer ---
template <MemoryTypes T_MemType>
struct NativeMemoryTraits {
    // Base template left deliberately undefined to enforce explicit specialization
};

// --- Specializations Mapping MemoryTypes -> Concrete C++ Type & DataType Bitmask ---
template <> struct NativeMemoryTraits<MemoryTypes::BOOL> { using ConcreteType = bool; static constexpr DataType DataFlag = DataType::BOOL; };
template <> struct NativeMemoryTraits<MemoryTypes::BYTE> { using ConcreteType = uint8_t; static constexpr DataType DataFlag = DataType::BYTE; };
template <> struct NativeMemoryTraits<MemoryTypes::INT32> { using ConcreteType = int32_t; static constexpr DataType DataFlag = DataType::INT32; };
template <> struct NativeMemoryTraits<MemoryTypes::INT64> { using ConcreteType = int64_t; static constexpr DataType DataFlag = DataType::INT64; };
template <> struct NativeMemoryTraits<MemoryTypes::FLOAT32> { using ConcreteType = float; static constexpr DataType DataFlag = DataType::FLOAT32; };
template <> struct NativeMemoryTraits<MemoryTypes::FLOAT64> { using ConcreteType = double; static constexpr DataType DataFlag = DataType::FLOAT64; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2> { using ConcreteType = godot::Vector2; static constexpr DataType DataFlag = DataType::VECTOR2; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3> { using ConcreteType = godot::Vector3; static constexpr DataType DataFlag = DataType::VECTOR3; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4> { using ConcreteType = godot::Vector4; static constexpr DataType DataFlag = DataType::VECTOR4; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2I> { using ConcreteType = godot::Vector2i; static constexpr DataType DataFlag = DataType::VECTOR2I; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3I> { using ConcreteType = godot::Vector3i; static constexpr DataType DataFlag = DataType::VECTOR3I; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4I> { using ConcreteType = godot::Vector4i; static constexpr DataType DataFlag = DataType::VECTOR4I; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR2D> { using ConcreteType = godot::Vector2; static constexpr DataType DataFlag = DataType::VECTOR2D; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR3D> { using ConcreteType = godot::Vector3; static constexpr DataType DataFlag = DataType::VECTOR3D; };
template <> struct NativeMemoryTraits<MemoryTypes::VECTOR4D> { using ConcreteType = godot::Vector4; static constexpr DataType DataFlag = DataType::VECTOR4D; };
template <> struct NativeMemoryTraits<MemoryTypes::COLOR> { using ConcreteType = godot::Color; static constexpr DataType DataFlag = DataType::COLOR; };
template <> struct NativeMemoryTraits<MemoryTypes::CUSTOM> { using ConcreteType = void*; static constexpr DataType DataFlag = DataType::CUSTOM; };

struct TaskUIFactories {
    std::function<godot::Ref<godot_ext::TaskResource>()> resource_factory;
    std::function<godot_ext::TaskGraphNode*()> node_factory;
};

struct LogicVariantGroup {
    bool is_primary;
    uint32_t variant_count;
    const char* const* labels; // Pointer to an array of C-strings
};

using NativeTaskFactory = std::function<std::unique_ptr<INativeTask>()>;

/**
 * @struct TaskDomainPack
 * @brief Descriptor and dispatch table for an isolated task domain (e.g., "Core", "IdeamGames", "IdeamNarratives").
 */
struct TaskDomainPack {
    godot::StringName domain_name;          // e.g., "Core", "IdeamGames", "IdeamNarratives"
    godot::String ui_menu_prefix;           // e.g., "" for Core, "Games" for IdeamGames
    godot::String manifest_path;            // e.g., "res://addons/ideam_games/resources/games_task_manifest.tres"
    int expected_manifest_version = 1;
    bool is_core = false;                   // Core is always active; companion packs depend on plugin state
    bool is_active_in_editor = false;       // Toggled when companion plugin enters/exits tree

    godot::Ref<TaskManifest> manifest;

    // Lifecycle & Bake Hooks
    void (*init_routing)() = nullptr;
    void (*cleanup_routing)() = nullptr;
    void (*generate_query_ui)(godot::Dictionary&) = nullptr;
    void (*generate_transform_ui)(godot::Dictionary&) = nullptr;
    void (*generate_metadata_ui)(godot::Dictionary&) = nullptr;

    // O(1) Runtime Execution Dispatchers
    std::unique_ptr<INativeTask> (*create_transform)(uint32_t l, uint32_t v, uint32_t s, uint32_t t) = nullptr;
    std::unique_ptr<INativeTask> (*create_query)(uint32_t o, uint32_t l, uint32_t v, uint32_t s, uint32_t t) = nullptr;
    std::unique_ptr<INativeTask> (*create_metadata)(uint32_t l, uint32_t v, uint32_t s, uint32_t t) = nullptr;
};

class IdeamTaskRegistry : public godot::Object {
    GDCLASS(IdeamTaskRegistry, godot::Object)

private:
    static godot::Ref<godot::FileAccess> bake_log_file;
    
    // Global Singleton Reference
    static IdeamTaskRegistry* singleton;

    // Runtime C++ Memory Allocation (Not Serialized)
    godot::HashMap<godot::StringName, NativeTaskFactory> manual_factories;
    godot::HashMap<godot::StringName, TaskUIFactories> ui_factories;

    // Temporary storage for utility definitions pending bake
    godot::Dictionary pending_utility_matrix;

    // Registered Domain Packs ("Core", "IdeamGames", etc.)
    godot::HashMap<godot::StringName, TaskDomainPack> domain_packs;

    // Primary Core manifest reference (aliased to domain_packs["Core"].manifest)
    godot::Ref<TaskManifest> active_manifest;

protected:
    static void _bind_methods();

public:
    IdeamTaskRegistry() = default;
    ~IdeamTaskRegistry() = default;

    static void start_bake_log();
    static void log_with_registry(const godot::String& p_message);
    static void end_bake_log();

    // --- Lifecycle Management ---
    static void init();
    static void cleanup();

    // --- Singleton Accessors ---
    static IdeamTaskRegistry* get_singleton() { return singleton; }

    // --- Domain Pack Registration & State Management ---
    void register_domain_pack(const TaskDomainPack& p_pack);
    bool has_domain_pack(const godot::StringName& p_domain) const;
    void set_domain_pack_active(const godot::StringName& p_domain, bool p_active);
    void sync_active_domains_from_config();

    bool are_active_manifests_valid() const;
    godot::PackedStringArray get_outdated_active_domains() const;
    godot::PackedStringArray get_active_domains() const;
    godot::String get_domain_ui_prefix(const godot::StringName& p_domain) const;

    // --- Task Baking ---
    void bake_manifest();

    // --- Active Manifest Access ---
    godot::Ref<TaskManifest> get_active_manifest() const { return active_manifest; }
    godot::Ref<TaskManifest> get_domain_manifest(const godot::StringName& p_domain) const;
    int get_manifest_version() const;
    int get_domain_manifest_version(const godot::StringName& p_domain) const;

    // --- Dynamic Creation for Manual Tasks ---
    [[nodiscard]] std::unique_ptr<INativeTask> create(const godot::StringName& p_name);
    godot::HashMap<godot::StringName, TaskUIFactories>* get_ui_factories() { return &ui_factories; }

    // --- Domain-Scoped O(1) Native Task Factory Dispatchers ---
    [[nodiscard]] std::unique_ptr<INativeTask> create_transform_task(
        const godot::StringName& p_domain,
        uint32_t p_logic_id,
        uint32_t p_view_id,
        uint32_t p_strategy_id,
        uint32_t p_type_id
    ) const;

    [[nodiscard]] std::unique_ptr<INativeTask> create_query_task(
        const godot::StringName& p_domain,
        uint32_t p_op_id,
        uint32_t p_logic_id,
        uint32_t p_view_id,
        uint32_t p_strategy_id,
        uint32_t p_type_id
    ) const;

    [[nodiscard]] std::unique_ptr<INativeTask> create_metadata_task(
        const godot::StringName& p_domain,
        uint32_t p_logic_id,
        uint32_t p_view_id,
        uint32_t p_strategy_id,
        uint32_t p_type_id
    ) const;

    // --- Variadic Registration ---
    template <typename T_Task, typename T_Resource, typename T_Node, typename... Args>
    void register_task(const godot::StringName& p_name, Args... args) {
        // 1. Register Execution Strategy (Memory)
        manual_factories[p_name] = [args...]() -> std::unique_ptr<INativeTask> {
            return std::make_unique<T_Task>(args...);
        };

        // Capture type-safe instantiation closures (Memory)
        ui_factories[p_name] = {
            []() -> godot::Ref<godot_ext::TaskResource> {
                godot::Ref<T_Resource> res;
                res.instantiate();
                return res;
            },
            []() -> godot_ext::TaskGraphNode* {
                return memnew(T_Node); 
            }
        };

        // 2. Register UI Presentation Data (Cached until bake)
        godot::Dictionary task_def;
        task_def["resource_class"] = T_Resource::get_class_static();
        task_def["node_class"] = T_Node::get_class_static();
        pending_utility_matrix[p_name] = task_def;
    }

    // --- Domain Matrix Accessors ---
    godot::Dictionary get_domain_query_matrix(const godot::StringName& p_domain) const;
    godot::Dictionary get_domain_transform_matrix(const godot::StringName& p_domain) const;
    godot::Dictionary get_domain_metadata_matrix(const godot::StringName& p_domain) const;

    // --- Static / Legacy Godot Accessors (Defaults to "Core", supports optional domain) ---
    static godot::Dictionary get_ui_query_matrix(const godot::StringName& p_domain = "Core");
    static godot::Dictionary get_ui_transform_matrix(const godot::StringName& p_domain = "Core");
    static godot::Dictionary get_ui_metadata_matrix(const godot::StringName& p_domain = "Core");
    static godot::Dictionary get_ui_utility_matrix();
};

} // namespace ideam::core