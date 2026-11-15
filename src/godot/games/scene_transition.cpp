#include "scene_transition.h"
#include "gameplay/xr_eye_effect_resource.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace ideam::godot_ext {

void SceneTransition::_bind_methods() {
    // Signals
    ADD_SIGNAL(godot::MethodInfo("transition_started"));
    ADD_SIGNAL(godot::MethodInfo("transition_completed"));
    ADD_SIGNAL(godot::MethodInfo("to_scene_not_found"));
    ADD_SIGNAL(godot::MethodInfo("load_to_scene_started"));
    ADD_SIGNAL(godot::MethodInfo("load_to_scene_failed"));
    ADD_SIGNAL(godot::MethodInfo("load_to_scene_completed", godot::PropertyInfo(godot::Variant::OBJECT, "to_scene", godot::PROPERTY_HINT_NODE_TYPE, "Node")));

    // Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_from", "from"), &SceneTransition::set_from);
    godot::ClassDB::bind_method(godot::D_METHOD("get_from"), &SceneTransition::get_from);

    godot::ClassDB::bind_method(godot::D_METHOD("set_to_path", "to_path"), &SceneTransition::set_to_path);
    godot::ClassDB::bind_method(godot::D_METHOD("get_to_path"), &SceneTransition::get_to_path);

    godot::ClassDB::bind_method(godot::D_METHOD("set_to", "to"), &SceneTransition::set_to);
    godot::ClassDB::bind_method(godot::D_METHOD("get_to"), &SceneTransition::get_to);

    ADD_GROUP("XR Stereoscopic Eye Transition", "");
    godot::ClassDB::bind_method(godot::D_METHOD("set_xr_eye_fx", "eye_fx"), &SceneTransition::set_xr_eye_fx);
    godot::ClassDB::bind_method(godot::D_METHOD("get_xr_eye_fx"), &SceneTransition::get_xr_eye_fx);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "xr_eye_fx", godot::PROPERTY_HINT_NODE_TYPE, "XREyeFXNode3D"), "set_xr_eye_fx", "get_xr_eye_fx");

    godot::ClassDB::bind_method(godot::D_METHOD("set_transition_eye_effect", "effect"), &SceneTransition::set_transition_eye_effect);
    godot::ClassDB::bind_method(godot::D_METHOD("get_transition_eye_effect"), &SceneTransition::get_transition_eye_effect);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::OBJECT, "transition_eye_effect", godot::PROPERTY_HINT_RESOURCE_TYPE, "XREyeEffectResource"), "set_transition_eye_effect", "get_transition_eye_effect");

    godot::ClassDB::bind_method(godot::D_METHOD("set_transition_out_duration", "duration"), &SceneTransition::set_transition_out_duration);
    godot::ClassDB::bind_method(godot::D_METHOD("get_transition_out_duration"), &SceneTransition::get_transition_out_duration);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "transition_out_duration", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.01,suffix:s"), "set_transition_out_duration", "get_transition_out_duration");

    godot::ClassDB::bind_method(godot::D_METHOD("set_transition_in_duration", "duration"), &SceneTransition::set_transition_in_duration);
    godot::ClassDB::bind_method(godot::D_METHOD("get_transition_in_duration"), &SceneTransition::get_transition_in_duration);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::FLOAT, "transition_in_duration", godot::PROPERTY_HINT_RANGE, "0.0,10.0,0.01,suffix:s"), "set_transition_in_duration", "get_transition_in_duration");

    // Methods
    godot::ClassDB::bind_method(godot::D_METHOD("start_transition", "to_path", "from"), &SceneTransition::start_transition, DEFVAL(nullptr));
    godot::ClassDB::bind_method(godot::D_METHOD("transition"), &SceneTransition::transition);
    godot::ClassDB::bind_method(godot::D_METHOD("to_scene_load_complete"), &SceneTransition::to_scene_load_complete);
    godot::ClassDB::bind_method(godot::D_METHOD("complete_transition"), &SceneTransition::complete_transition);
}

SceneTransition::SceneTransition() {}

SceneTransition::~SceneTransition() {}

void SceneTransition::_process(double delta) {
    if (_transition_in_progress) {
        godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
        if (loader->load_threaded_get_status(_to_path) == godot::ResourceLoader::THREAD_LOAD_LOADED) {
            to_scene_load_complete();
        }
    }
}

// Getters / Setters
void SceneTransition::set_transition_in_progress(bool p_in_progress) {
    if (p_in_progress == _transition_in_progress) return;
    _transition_in_progress = p_in_progress; 
}
bool SceneTransition::get_transition_in_progress() const { return _transition_in_progress; }

void SceneTransition::set_from(godot::Node* p_from) { 
    if (p_from == _from) return;
    _from = p_from; 
}
godot::Node* SceneTransition::get_from() const { return _from; }

void SceneTransition::set_to_path(const godot::String& p_to_path) { 
    if (p_to_path == _to_path) return;
    _to_path = p_to_path; 
}
godot::String SceneTransition::get_to_path() const { return _to_path; }

void SceneTransition::set_to(godot::Node* p_to) { 
    if (p_to == _to) return;
    _to = p_to; 
}
godot::Node* SceneTransition::get_to() const { return _to; }

void SceneTransition::set_xr_eye_fx(XREyeFXNode3D* p_eye_fx) { xr_eye_fx = p_eye_fx; }
XREyeFXNode3D* SceneTransition::get_xr_eye_fx() const { return xr_eye_fx; }

void SceneTransition::set_transition_eye_effect(const godot::Ref<XREyeEffectResource>& p_effect) { transition_eye_effect = p_effect; }
godot::Ref<XREyeEffectResource> SceneTransition::get_transition_eye_effect() const { return transition_eye_effect; }

void SceneTransition::set_transition_out_duration(float p_duration) { transition_out_duration = godot::Math::max(0.0f, p_duration); }
float SceneTransition::get_transition_out_duration() const { return transition_out_duration; }

void SceneTransition::set_transition_in_duration(float p_duration) { transition_in_duration = godot::Math::max(0.0f, p_duration); }
float SceneTransition::get_transition_in_duration() const { return transition_in_duration; }

// Class Functions
void SceneTransition::start_transition(const godot::String& to_path, godot::Node* from) {
    if (_transition_in_progress) {
        return;
    }
    
    _from = nullptr;
    _to_path = "";
    _to = nullptr;
    
    if (to_path.is_empty()) {
        return;
    }
        
    _from = from;
    _to_path = to_path;
    
    _transition_in_progress = true;

    // Trigger stereoscopic XR eye transition out (`Game_Out`, `Board_Out`, or `Blink`) if configured
    if (xr_eye_fx) {
        if (transition_eye_effect.is_valid()) {
            int idx = xr_eye_fx->add_effect(transition_eye_effect);
            xr_eye_fx->tween_effect(idx, 1.0f, transition_out_duration);
        } else {
            xr_eye_fx->fade_out(transition_out_duration);
        }
    }
    
    if (get_signal_connection_list("transition_started").size() > 0) {
        emit_signal("transition_started");
    } else {
        transition();
    }
}

void SceneTransition::transition() {
    if (!_transition_in_progress) {
        return;
    }
    
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    
    if (!loader->exists(_to_path)) {
        emit_signal("to_scene_not_found");
        return;
    }
        
    godot::ResourceLoader::ThreadLoadStatus status = loader->load_threaded_get_status(_to_path);
    if (status == godot::ResourceLoader::THREAD_LOAD_IN_PROGRESS) {
        godot::UtilityFunctions::print("Already loading");
        return;
    }
        
    loader->load_threaded_request(_to_path);
    emit_signal("load_to_scene_started");
}

void SceneTransition::to_scene_load_complete() {
    godot::ResourceLoader* loader = godot::ResourceLoader::get_singleton();
    godot::Ref<godot::PackedScene> loadedResource = loader->load_threaded_get(_to_path);
    
    if (!loadedResource.is_valid()) {
        return;
    }
    
    _to = loadedResource->instantiate();
    
    if (!_to) {
        emit_signal("load_to_scene_failed");
        return;
    }
    
    if (get_signal_connection_list("load_to_scene_completed").size() > 0) {
        emit_signal("load_to_scene_completed", _to);
    } else {
        complete_transition();
    }
}

void SceneTransition::complete_transition() {
    _transition_in_progress = false;

    // Trigger stereoscopic XR eye transition in (`Game_In`, `Board_In`) if configured
    if (xr_eye_fx) {
        if (transition_eye_effect.is_valid()) {
            int idx = xr_eye_fx->add_effect(transition_eye_effect);
            xr_eye_fx->tween_effect(idx, 0.0f, transition_in_duration);
        } else {
            xr_eye_fx->fade_in(transition_in_duration);
        }
    }
    
    emit_signal("transition_completed");
    
    _from = nullptr;
    _to = nullptr;
    _to_path = "";
}

} // namespace ideam::godot_ext