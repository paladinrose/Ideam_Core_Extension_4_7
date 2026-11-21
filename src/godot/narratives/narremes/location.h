#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Character;
class Prop;

class Location : public Narreme {
    GDCLASS(Location, Narreme)

private:
    // DOD NOTE: Storing discrete TypedArrays for every Location instance causes
    // scattered heap allocations. In IDEAM Core, we will manage this relationship
    // via a BridgeView linking a Parent (Location) to a subdivided Child block
    // (Characters/Props/ConnectedLocations), or a SparseSetView for O(1) ECS tracking.
    godot::TypedArray<Character> characters;
    godot::TypedArray<Prop> props;
    godot::TypedArray<Location> connected_locations;

    // Direct internal mutators used by IdeamUndoRedo callables and fallback paths
    void _insert_character_at_direct(int p_index, const godot::Ref<Character> &p_character);
    void _remove_character_at_direct(int p_index);
    void _insert_prop_at_direct(int p_index, const godot::Ref<Prop> &p_prop);
    void _remove_prop_at_direct(int p_index);
    void _insert_connected_location_at_direct(int p_index, const godot::Ref<Location> &p_target);
    void _remove_connected_location_at_direct(int p_index);
    void _remove_connected_location_direct(const godot::Ref<Location> &p_target);

protected:
    static void _bind_methods();

public:
    Location();
    ~Location() override;

    virtual void initialize() override;

    // Getters / Setters
    void set_characters(const godot::TypedArray<Character> &p_characters);
    [[nodiscard]] godot::TypedArray<Character> get_characters() const;

    void set_props(const godot::TypedArray<Prop> &p_props);
    [[nodiscard]] godot::TypedArray<Prop> get_props() const;

    void set_connected_locations(const godot::TypedArray<Location> &p_locations);
    [[nodiscard]] godot::TypedArray<Location> get_connected_locations() const;

    // Character Management (IdeamUndoRedo-aware)
    int character_entrance(const godot::Ref<Character> &character);
    [[nodiscard]] int get_character_id(const godot::Ref<Character> &character) const;
    [[nodiscard]] bool has_character(const godot::Ref<Character> &character) const;
    bool character_exit(const godot::Ref<Character> &character);
    bool character_exit_at(int character_ID);

    // Prop Management (IdeamUndoRedo-aware)
    int prop_entrance(const godot::Ref<Prop> &prop);
    [[nodiscard]] int get_prop_id(const godot::Ref<Prop> &prop) const;
    [[nodiscard]] bool has_prop(const godot::Ref<Prop> &prop) const;
    bool prop_exit(const godot::Ref<Prop> &prop);
    bool prop_exit_at(int prop_ID);

    // Directed Narrative Topology Management (IdeamUndoRedo-aware)
    int connect_location(const godot::Ref<Location> &p_target, bool p_bidirectional = false);
    bool disconnect_location(const godot::Ref<Location> &p_target, bool p_bidirectional = false);
    bool disconnect_location_at(int p_connection_id);
    [[nodiscard]] int get_connected_location_id(const godot::Ref<Location> &p_target) const;
    [[nodiscard]] bool is_connected_to(const godot::Ref<Location> &p_target) const;
    [[nodiscard]] bool has_connections() const;
    [[nodiscard]] int get_connection_count() const;

    // Directed Graph Pathfinding (BFS shortest path, cycle-safe)
    [[nodiscard]] godot::TypedArray<Location> find_path_to(
        const godot::Ref<Location> &p_target,
        int p_max_depth = 64
    ) const;
    [[nodiscard]] bool has_path_to(
        const godot::Ref<Location> &p_target,
        int p_max_depth = 64
    ) const;
    [[nodiscard]] int get_distance_to(
        const godot::Ref<Location> &p_target,
        int p_max_depth = 64
    ) const;
    [[nodiscard]] godot::TypedArray<Location> get_reachable_locations(int p_max_depth = 64) const;

    // Overrides
    virtual godot::TypedArray<godot::Dictionary> get_narrative_condition_descriptors(Narreme *p_narreme = nullptr) const override;
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext