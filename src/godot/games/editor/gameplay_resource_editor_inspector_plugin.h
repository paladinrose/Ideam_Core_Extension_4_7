#pragma once

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../gameplay/gameplay_resource.h"
#include "../gameplay/gameplay_rule.h"

#include <godot_cpp/classes/v_box_container.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace ideam::godot_ext {

/**
 * @brief GameplayResourceEditorInspectorPlugin — Phase 4 Editor Inspector plugin
 * for GameplayResource subclasses (GameplayControlMap, GameplayDifficulty,
 * GameplayPerspective, GameplayAccessibilityOption, GameplayAudioOption,
 * GameplayGraphicsOption, GameplayStyle, GameplayMode).
 *
 * Responsibilities:
 *   1. Embedded Live Preview Header:
 *      - GameplayControlMap -> embeds ControlStickDeadzoneVisualizer
 *      - GameplayDifficulty -> embeds DifficultyProfileRadar
 *      - GameplayPerspective -> embeds PerspectiveFrustumVisualizer
 *      - GameplayAccessibilityOption -> embeds AccessibilityLivePreviewCard
 *   2. Collapsible "Synthesized Cascade Rules (collect_rules())" Preview Box:
 *      Shows all auto-synthesized and custom GameplayRule declarations emitted
 *      by collect_rules() with a [Refresh] button and per-rule [Inspect] buttons.
 */
class GameplayResourceEditorInspectorPlugin : public IdeamEditorInspectorPlugin {
    GDCLASS(GameplayResourceEditorInspectorPlugin, IdeamEditorInspectorPlugin)

private:
    godot::ObjectID _current_resource_id;
    godot::ObjectID _rules_list_vbox_id;

    GameplayResource *_get_current_resource() const;
    void _rebuild_synthesized_rules_list();

    void _on_refresh_rules_pressed();
    void _on_inspect_rule_pressed(const godot::Ref<GameplayRule> &p_rule);

protected:
    static void _bind_methods();

public:
    GameplayResourceEditorInspectorPlugin();
    ~GameplayResourceEditorInspectorPlugin() override;

    virtual bool _can_handle(godot::Object *p_object) override;
    virtual void _parse_begin(godot::Object *p_object) override;
    virtual bool _parse_property(
        godot::Object *p_object,
        godot::Variant::Type p_type,
        const godot::String &p_name,
        godot::PropertyHint p_hint_type,
        const godot::String &p_hint_string,
        godot::BitField<godot::PropertyUsageFlags> p_usage_flags,
        bool p_wide
    ) override;
};

} // namespace ideam::godot_ext