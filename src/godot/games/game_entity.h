#pragma once

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Game;
class GameplayStyle;

// DOD NOTE: While extending `Node` is necessary for Godot integration, `GameEntity` 
// objects inherently suffer from Array of Structures (AoS) memory bloat and vtable 
// scattering across the heap. For critical `_process` loops, consider extracting 
// hot data (like `time_scale`, `enabled` state) into parallel contiguous arrays 
// (Structure of Arrays - SoA) managed by the `GameBoard` or a custom ECS architecture.
class GameEntity : public godot::Node {
    GDCLASS(GameEntity, godot::Node)

protected:
    static void _bind_methods();

    bool enabled = true;
    godot::String title;
    godot::Node* root_node = nullptr;
    Game* _game = nullptr;
    float time_scale = 1.0f;
    godot::Ref<GameplayStyle> gameplay_style;

    bool entity_is_initialized = false;
    bool game_processed = false;
    bool entity_is_paused = false;

    godot::Ref<GameplayStyle> _resolved_gameplay_style;

    // --- Section 2.4 XR_RayCast3D_Target Merger: 3D Ray / Gaze Hover & Dwell Tracking ---
    bool ray_target_enabled = true;
    int default_target_action_id = 0;
    godot::Array hovering_rays;
    godot::PackedFloat32Array hover_dwell_times; // Cumulative dwell seconds (`times_over`) per hovering ray

public:
    GameEntity();
    ~GameEntity();

    virtual void _ready() override;

    // Setters / Getters
    void set_enabled(bool p_enabled);
    bool get_enabled() const;

    void set_title(const godot::String& p_title);
    godot::String get_title() const;

    void set_root_node(godot::Node* p_root);
    godot::Node* get_root_node() const;

    void set_game(Game* p_game);
    Game* get_game() const;

    void set_time_scale(float p_scale);
    float get_time_scale() const;

    void set_gameplay_style(const godot::Ref<GameplayStyle>& p_style);
    godot::Ref<GameplayStyle> get_gameplay_style() const;
    godot::Ref<GameplayStyle> get_resolved_gameplay_style() const;

    // DOD Accessors for strict internal state
    bool get_entity_is_paused() const;
    void set_entity_is_paused(bool p_paused);

    // --- Section 2.4 3D Ray / Gaze Hover & Target Action API ---
    void set_ray_target_enabled(bool p_enabled);
    bool get_ray_target_enabled() const;

    void set_default_target_action_id(int p_action_id);
    int get_default_target_action_id() const;

    void on_ray_entered(godot::Node* p_ray);
    void on_ray_stay(godot::Node* p_ray, float p_delta);
    void on_ray_exited(godot::Node* p_ray);

    bool is_ray_hovering() const;
    int get_hovering_ray_count() const;
    float get_ray_dwell_time(godot::Node* p_ray = nullptr) const;
    godot::PackedFloat32Array get_hover_dwell_times() const;

    virtual void use_target_action(godot::Node* p_instigator = nullptr);

    // Class Functions
    void validate_game();
    
    virtual void game_start();
    virtual void enter_game();
    virtual void game_end();
    virtual void exit_game();
    virtual void game_pause();
    virtual void game_continue();
    
    // DOD NOTE: Relying on virtual function overriding for `game_process` forces 
    // pointer dereferencing and vtable lookups every frame, stalling the CPU pipeline. 
    // Batch processing standard entity behaviors using a Data-Oriented Job System 
    // will yield massive cache coherence improvements.
    virtual void game_process(double delta);
    virtual void game_process_clear();
    
    void toggle_game_entity_enabled();
    virtual void enable_game_entity();
    virtual void disable_game_entity();
    
    // DOD NOTE: Passing highly dynamic string-keyed Dictionaries forces expensive 
    // Variant hashing. Refactoring `consequences` into a tightly packed POD Struct 
    // (e.g., `EntityCommand`) or an integer bitmask will significantly improve throughput.
    virtual void action_consequences(int score, const godot::Dictionary& consequences);
    virtual void apply_gameplay_style(const godot::Ref<GameplayStyle>& newStyle);
    
    virtual godot::Dictionary save_data() const;
    virtual void load_data(const godot::Dictionary& data);
};

} // namespace ideam::godot_ext