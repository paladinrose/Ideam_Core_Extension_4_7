#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

class XREyeFXNode3D;
class XREyeEffectResource;

// DOD NOTE: While Scene Transitions inherently deal with heavy asset loading, 
// maintaining a node-based, polling architecture for background tasks creates 
// unnecessary cache pressure. In a fully Data-Oriented system, scene loading 
// status should be monitored via a dense integer-indexed state array, updated 
// by a background thread and processed linearly without pointer chasing.
class SceneTransition : public godot::Node {
    GDCLASS(SceneTransition, godot::Node)

protected:
    static void _bind_methods();

private:
    bool _transition_in_progress = false;
    
    godot::Node* _from = nullptr;
    godot::String _to_path;
    godot::Node* _to = nullptr;

    // --- Section 2.4 Stereoscopic XR Eye FX Integration (`Game_In`/`Game_Out`, `Board_In`/`Board_Out`, `Blink`) ---
    XREyeFXNode3D* xr_eye_fx = nullptr;
    godot::Ref<XREyeEffectResource> transition_eye_effect;
    float transition_out_duration = 0.25f;
    float transition_in_duration = 0.25f;

public:
    SceneTransition();
    ~SceneTransition();

    virtual void _process(double delta) override;

    // Setters / Getters
    void set_transition_in_progress(bool p_in_progress);
    bool get_transition_in_progress() const;

    void set_from(godot::Node* p_from);
    godot::Node* get_from() const;

    void set_to_path(const godot::String& p_to_path);
    godot::String get_to_path() const;

    void set_to(godot::Node* p_to);
    godot::Node* get_to() const;

    void set_xr_eye_fx(XREyeFXNode3D* p_eye_fx);
    XREyeFXNode3D* get_xr_eye_fx() const;

    void set_transition_eye_effect(const godot::Ref<XREyeEffectResource>& p_effect);
    godot::Ref<XREyeEffectResource> get_transition_eye_effect() const;

    void set_transition_out_duration(float p_duration);
    float get_transition_out_duration() const;

    void set_transition_in_duration(float p_duration);
    float get_transition_in_duration() const;

    // Class Functions
    void start_transition(const godot::String& to_path, godot::Node* from = nullptr);
    void transition();
    
    void to_scene_load_complete();
    void complete_transition();
};

} // namespace ideam::godot_ext