#pragma once

/**
 * jukebox_resource.h
 * Spatial `MusicZoneNode3D` & `JukeboxResource` playlist state machine (`src/godot/cymatics/`).
 * Ports `MusicZone.cs` and `Jukebox.cs`:
 *   - Local vs. global playlist stack push/pop
 *   - `LoopMode` (`LOOP_ALL`, `LOOP_SINGLE`, `LOOP_NONE`) and `randomize_tracks`
 *   - Automatic enter/exit zone triggers (`Area3D` compatible with `SplineZone3D` volumes)
 */

#include "../../core/cymatics/cymatics_common.h"

#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

#include <vector>

namespace ideam::godot_ext {

enum JukeboxLoopModeEnum : int32_t {
    JUKEBOX_LOOP_ALL    = static_cast<int32_t>(core::JukeboxLoopMode::LOOP_ALL),
    JUKEBOX_LOOP_SINGLE = static_cast<int32_t>(core::JukeboxLoopMode::LOOP_SINGLE),
    JUKEBOX_LOOP_NONE   = static_cast<int32_t>(core::JukeboxLoopMode::LOOP_NONE)
};

class JukeboxResource : public godot::Resource {
    GDCLASS(JukeboxResource, godot::Resource)

private:
    godot::StringName playlist_id_;
    godot::TypedArray<godot::AudioStream> tracks_;
    JukeboxLoopModeEnum loop_mode_ = JUKEBOX_LOOP_ALL;
    bool randomize_tracks_         = false;
    float crossfade_seconds_       = 1.25f;
    int32_t current_index_         = 0;

protected:
    static void _bind_methods();

public:
    JukeboxResource() = default;
    ~JukeboxResource() override = default;

    void set_playlist_id(const godot::StringName &p_id) { playlist_id_ = p_id; }
    [[nodiscard]] godot::StringName get_playlist_id() const { return playlist_id_; }

    void set_tracks(const godot::TypedArray<godot::AudioStream> &p_tracks) { tracks_ = p_tracks; }
    [[nodiscard]] godot::TypedArray<godot::AudioStream> get_tracks() const { return tracks_; }

    void set_loop_mode(JukeboxLoopModeEnum p_mode) { loop_mode_ = p_mode; }
    [[nodiscard]] JukeboxLoopModeEnum get_loop_mode() const noexcept { return loop_mode_; }

    void set_randomize_tracks(bool p_rand) { randomize_tracks_ = p_rand; }
    [[nodiscard]] bool get_randomize_tracks() const noexcept { return randomize_tracks_; }

    void set_crossfade_seconds(float p_sec) { crossfade_seconds_ = p_sec >= 0.0f ? p_sec : 0.0f; }
    [[nodiscard]] float get_crossfade_seconds() const noexcept { return crossfade_seconds_; }

    [[nodiscard]] int32_t get_current_index() const noexcept { return current_index_; }
    [[nodiscard]] godot::Ref<godot::AudioStream> get_current_track() const;
    [[nodiscard]] godot::Ref<godot::AudioStream> advance_track(uint32_t p_seed = 0u);
    void reset_index() noexcept { current_index_ = 0; }
};

class MusicZoneNode3D : public godot::Area3D {
    GDCLASS(MusicZoneNode3D, godot::Area3D)

private:
    godot::Ref<JukeboxResource> local_playlist_;
    int32_t zone_priority_      = 0;
    godot::StringName listener_group_ = godot::StringName("player");

    static std::vector<godot::Ref<JukeboxResource>> &global_playlist_stack_();

protected:
    static void _bind_methods();

public:
    MusicZoneNode3D();
    ~MusicZoneNode3D() override = default;

    void _ready() override;

    void set_local_playlist(const godot::Ref<JukeboxResource> &p_playlist) { local_playlist_ = p_playlist; }
    [[nodiscard]] godot::Ref<JukeboxResource> get_local_playlist() const { return local_playlist_; }

    void set_zone_priority(int32_t p_priority) { zone_priority_ = p_priority; }
    [[nodiscard]] int32_t get_zone_priority() const noexcept { return zone_priority_; }

    void set_listener_group(const godot::StringName &p_group) { listener_group_ = p_group; }
    [[nodiscard]] godot::StringName get_listener_group() const { return listener_group_; }

    void push_zone_playlist();
    void pop_zone_playlist();

    static godot::Ref<JukeboxResource> get_active_global_playlist();
    static void clear_global_playlist_stack();

    void _on_body_entered(godot::Node *p_body);
    void _on_body_exited(godot::Node *p_body);
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::JukeboxLoopModeEnum);
