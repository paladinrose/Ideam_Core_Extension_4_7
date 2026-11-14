#pragma once

/**
 * communications_inspector.h
 * `CommunicationsInspector` (`src/godot/communications/editor/`).
 *
 * Provides Inspector previews for `CommunicationMediumResource` (live SNR &
 * noise tier calculator), `UtteranceResource` (SSML & TTS test trigger),
 * `DiscourseResource` (turn state summary), and Transceiver/SpeechBox nodes.
 */

#include "../../editor/ideam_editor_inspector_plugin.h"
#include "../transceiver_nodes.h"

namespace ideam::godot_ext {

class CommunicationsInspector : public IdeamEditorInspectorPlugin {
    GDCLASS(CommunicationsInspector, IdeamEditorInspectorPlugin)

protected:
    static void _bind_methods();

public:
    CommunicationsInspector() = default;
    ~CommunicationsInspector() override = default;

    bool _can_handle(godot::Object *p_object) const override;
    void _parse_begin(godot::Object *p_object) override;
};

} // namespace ideam::godot_ext
