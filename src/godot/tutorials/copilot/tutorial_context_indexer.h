#pragma once

/**
 * tutorial_context_indexer.h
 * Co-Pilot Layer indexer (`src/godot/tutorials/copilot/`):
 *   - Indexes active Ideam plugins from `res://.ideam_registry.cfg`
 *   - Indexes all registered 5-tier tools from `ToolRegistry::export_tool_catalog_schema()`
 *   - Maintains registered `TutorialResource`s and resolves them by `help_topic_id` or keyword/tag
 */

#include "../resources/tutorial_resource.h"
#include "../../tools/tool_registry.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

#include <vector>

namespace ideam::godot_ext {

class TutorialContextIndexer : public godot::RefCounted {
    GDCLASS(TutorialContextIndexer, godot::RefCounted)

private:
    std::vector<godot::Ref<TutorialResource>> tutorials_;

protected:
    static void _bind_methods();

public:
    TutorialContextIndexer() = default;
    ~TutorialContextIndexer() override = default;

    void register_tutorial(const godot::Ref<TutorialResource> &p_tutorial);
    void unregister_tutorial(const godot::StringName &p_tutorial_id);
    void clear_tutorials();

    [[nodiscard]] godot::Ref<TutorialResource> find_tutorial_by_id(const godot::StringName &p_tutorial_id) const;
    [[nodiscard]] godot::Ref<TutorialResource> find_tutorial_for_help_topic(
        const godot::StringName &p_help_topic_id
    ) const;

    [[nodiscard]] godot::PackedStringArray get_active_ecosystem_plugins() const;
    [[nodiscard]] godot::Dictionary build_context_snapshot() const;
};

} // namespace ideam::godot_ext
