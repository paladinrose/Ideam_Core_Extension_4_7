#pragma once

/**
 * discourse_resource.h
 * `UtteranceResource` and `DiscourseResource` (`src/godot/communications/`).
 *
 * ============================================================================
 * ARCHITECTURAL PLAN & DESIGN NOTES
 * ============================================================================
 * Succeeds `Discourse.cs`, `Turn`, `TurnConstructionUnit`, and `Utterance`:
 *   - `UtteranceResource`: Pairs a surface text / `Sememe` sequence with a
 *     `Spline2DResource` prosody curve (X = Volume, Y = Pitch) and a `TurnConstructionUnitType`.
 *   - `DiscourseResource`: Manages turn allocation across participating `MindAgentNode`s
 *     and routes transmitted `UtteranceResource` packets through a `CommunicationMediumResource`.
 */

#include "communication_medium_resource.h"
#include "../splines/spline_2d/spline_2d_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class UtteranceResource : public godot::Resource {
    GDCLASS(UtteranceResource, godot::Resource)

private:
    godot::String raw_text_;
    godot::Ref<Spline2DResource> prosody_spline_;
    int tcu_type_        = static_cast<int>(core::TurnConstructionUnitType::SENTENTIAL);
    int resolution_type_ = static_cast<int>(core::TurnResolutionType::UNRESOLVED);

protected:
    static void _bind_methods();

public:
    UtteranceResource() = default;
    ~UtteranceResource() override = default;

    void set_raw_text(const godot::String &p_text) { raw_text_ = p_text; emit_changed(); }
    [[nodiscard]] godot::String get_raw_text() const { return raw_text_; }

    void set_prosody_spline(const godot::Ref<Spline2DResource> &p_spline) { prosody_spline_ = p_spline; emit_changed(); }
    [[nodiscard]] godot::Ref<Spline2DResource> get_prosody_spline() const { return prosody_spline_; }
};

class DiscourseResource : public godot::Resource {
    GDCLASS(DiscourseResource, godot::Resource)

private:
    godot::Ref<CommunicationMediumResource> medium_;
    int current_turn_ = -1;

protected:
    static void _bind_methods();

public:
    DiscourseResource() = default;
    ~DiscourseResource() override = default;

    void set_medium(const godot::Ref<CommunicationMediumResource> &p_medium) { medium_ = p_medium; emit_changed(); }
    [[nodiscard]] godot::Ref<CommunicationMediumResource> get_medium() const { return medium_; }

    void set_current_turn(int p_turn) { current_turn_ = p_turn; emit_changed(); }
    [[nodiscard]] int get_current_turn() const noexcept { return current_turn_; }
};

} // namespace ideam::godot_ext
