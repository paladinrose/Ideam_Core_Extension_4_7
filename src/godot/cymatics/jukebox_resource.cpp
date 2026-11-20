#include "jukebox_resource.h"

#include <godot_cpp/variant/callable.hpp>

#include <algorithm>

namespace ideam::godot_ext {

// ============================================================================
// JukeboxResource
// ============================================================================

void JukeboxResource::_bind_methods() {
    using namespace godot;

    BIND_ENUM_CONSTANT(JUKEBOX_LOOP_ALL);
    BIND_ENUM_CONSTANT(JUKEBOX_LOOP_SINGLE);
    BIND_ENUM_CONSTANT(JUKEBOX_LOOP_NONE);

    ClassDB::bind_method(D_METHOD("set_playlist_id", "id"), &JukeboxResource::set_playlist_id);
    ClassDB::bind_method(D_METHOD("get_playlist_id"), &JukeboxResource::get_playlist_id);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "playlist_id"), "set_playlist_id", "get_playlist_id");

    ClassDB::bind_method(D_METHOD("set_tracks", "tracks"), &JukeboxResource::set_tracks);
    ClassDB::bind_method(D_METHOD("get_tracks"), &JukeboxResource::get_tracks);
    ADD_PROPERTY(
        PropertyInfo(Variant::ARRAY, "tracks", PROPERTY_HINT_ARRAY_TYPE, "AudioStream"),
        "set_tracks",
        "get_tracks"
    );

    ClassDB::bind_method(D_METHOD("set_loop_mode", "mode"), &JukeboxResource::set_loop_mode);
    ClassDB::bind_method(D_METHOD("get_loop_mode"), &JukeboxResource::get_loop_mode);
    ADD_PROPERTY(
        PropertyInfo(Variant::INT, "loop_mode", PROPERTY_HINT_ENUM, "All:0,Single:1,None:2"),
        "set_loop_mode",
        "get_loop_mode"
    );

    ClassDB::bind_method(D_METHOD("set_randomize_tracks", "randomize"), &JukeboxResource::set_randomize_tracks);
    ClassDB::bind_method(D_METHOD("get_randomize_tracks"), &JukeboxResource::get_randomize_tracks);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "randomize_tracks"), "set_randomize_tracks", "get_randomize_tracks");

    ClassDB::bind_method(D_METHOD("set_crossfade_seconds", "seconds"), &JukeboxResource::set_crossfade_seconds);
    ClassDB::bind_method(D_METHOD("get_crossfade_seconds"), &JukeboxResource::get_crossfade_seconds);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "crossfade_seconds"), "set_crossfade_seconds", "get_crossfade_seconds");

    ClassDB::bind_method(D_METHOD("get_current_index"), &JukeboxResource::get_current_index);
    ClassDB::bind_method(D_METHOD("get_current_track"), &JukeboxResource::get_current_track);
    ClassDB::bind_method(D_METHOD("advance_track", "seed"), &JukeboxResource::advance_track, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("reset_index"), &JukeboxResource::reset_index);
}

godot::Ref<godot::AudioStream> JukeboxResource::get_current_track() const {
    if (tracks_.is_empty() || current_index_ < 0 || current_index_ >= tracks_.size()) {
        return godot::Ref<godot::AudioStream>();
    }
    return tracks_[current_index_];
}

godot::Ref<godot::AudioStream> JukeboxResource::advance_track(uint32_t p_seed) {
    const int32_t count = static_cast<int32_t>(tracks_.size());
    if (count <= 0) {
        return godot::Ref<godot::AudioStream>();
    }
    if (loop_mode_ == JUKEBOX_LOOP_SINGLE) {
        return get_current_track();
    }
    if (randomize_tracks_ && count > 1) {
        uint32_t s = (p_seed == 0u) ? static_cast<uint32_t>(current_index_ * 2654435761u + 17u) : p_seed;
        s ^= s << 13u;
        s ^= s >> 17u;
        s ^= s << 5u;
        int32_t next = static_cast<int32_t>(s % static_cast<uint32_t>(count));
        if (next == current_index_) {
            next = (next + 1) % count;
        }
        current_index_ = next;
        return get_current_track();
    }

    ++current_index_;
    if (current_index_ >= count) {
        if (loop_mode_ == JUKEBOX_LOOP_ALL) {
            current_index_ = 0;
        } else {
            current_index_ = count - 1;
            return godot::Ref<godot::AudioStream>();
        }
    }
    return get_current_track();
}

// ============================================================================
// MusicZoneNode3D
// ============================================================================

std::vector<godot::Ref<JukeboxResource>> &MusicZoneNode3D::global_playlist_stack_() {
    static std::vector<godot::Ref<JukeboxResource>> stack;
    return stack;
}

void MusicZoneNode3D::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "playlist_activated",
        PropertyInfo(Variant::OBJECT, "playlist", PROPERTY_HINT_RESOURCE_TYPE, "JukeboxResource")
    ));

    ClassDB::bind_method(D_METHOD("set_local_playlist", "playlist"), &MusicZoneNode3D::set_local_playlist);
    ClassDB::bind_method(D_METHOD("get_local_playlist"), &MusicZoneNode3D::get_local_playlist);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "local_playlist", PROPERTY_HINT_RESOURCE_TYPE, "JukeboxResource"),
        "set_local_playlist",
        "get_local_playlist"
    );

    ClassDB::bind_method(D_METHOD("set_zone_priority", "priority"), &MusicZoneNode3D::set_zone_priority);
    ClassDB::bind_method(D_METHOD("get_zone_priority"), &MusicZoneNode3D::get_zone_priority);
    ADD_PROPERTY(PropertyInfo(Variant::INT, "zone_priority"), "set_zone_priority", "get_zone_priority");

    ClassDB::bind_method(D_METHOD("set_listener_group", "group"), &MusicZoneNode3D::set_listener_group);
    ClassDB::bind_method(D_METHOD("get_listener_group"), &MusicZoneNode3D::get_listener_group);
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "listener_group"), "set_listener_group", "get_listener_group");

    ClassDB::bind_method(D_METHOD("push_zone_playlist"), &MusicZoneNode3D::push_zone_playlist);
    ClassDB::bind_method(D_METHOD("pop_zone_playlist"), &MusicZoneNode3D::pop_zone_playlist);
    ClassDB::bind_static_method(
        "MusicZoneNode3D",
        D_METHOD("get_active_global_playlist"),
        &MusicZoneNode3D::get_active_global_playlist
    );
    ClassDB::bind_static_method(
        "MusicZoneNode3D",
        D_METHOD("clear_global_playlist_stack"),
        &MusicZoneNode3D::clear_global_playlist_stack
    );

    ClassDB::bind_method(D_METHOD("_on_body_entered", "body"), &MusicZoneNode3D::_on_body_entered);
    ClassDB::bind_method(D_METHOD("_on_body_exited", "body"), &MusicZoneNode3D::_on_body_exited);
}

MusicZoneNode3D::MusicZoneNode3D() = default;

void MusicZoneNode3D::_ready() {
    connect("body_entered", godot::Callable(this, "_on_body_entered"));
    connect("body_exited", godot::Callable(this, "_on_body_exited"));
}

void MusicZoneNode3D::push_zone_playlist() {
    if (local_playlist_.is_null()) {
        return;
    }
    auto &stack = global_playlist_stack_();
    std::erase(stack, local_playlist_);
    stack.push_back(local_playlist_);
    emit_signal("playlist_activated", local_playlist_);
}

void MusicZoneNode3D::pop_zone_playlist() {
    if (local_playlist_.is_null()) {
        return;
    }
    auto &stack = global_playlist_stack_();
    std::erase(stack, local_playlist_);
    emit_signal("playlist_activated", get_active_global_playlist());
}

godot::Ref<JukeboxResource> MusicZoneNode3D::get_active_global_playlist() {
    const auto &stack = global_playlist_stack_();
    if (stack.empty()) {
        return godot::Ref<JukeboxResource>();
    }
    return stack.back();
}

void MusicZoneNode3D::clear_global_playlist_stack() {
    global_playlist_stack_().clear();
}

void MusicZoneNode3D::_on_body_entered(godot::Node *p_body) {
    if (!p_body) {
        return;
    }
    if (listener_group_ == godot::StringName() || p_body->is_in_group(listener_group_)) {
        push_zone_playlist();
    }
}

void MusicZoneNode3D::_on_body_exited(godot::Node *p_body) {
    if (!p_body) {
        return;
    }
    if (listener_group_ == godot::StringName() || p_body->is_in_group(listener_group_)) {
        pop_zone_playlist();
    }
}

} // namespace ideam::godot_ext
