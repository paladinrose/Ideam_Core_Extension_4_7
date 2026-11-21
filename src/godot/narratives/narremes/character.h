#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations for other Narreme types
class Location;
class Prop;
class Relationship;
class Plot;
class Incident;
class Narrative;

class Character : public Narreme {
    GDCLASS(Character, Narreme)

private:
    // DOD NOTE: Storing these TypedArrays inside the Character object forces
    // Array of Structures (AoS) memory layout and heap fragmentation.
    // In IDEAM Core, these will be replaced by `BridgeView` lookups
    // linking a `Character ID` directly to a contiguous block of `Prop IDs` or `Plot IDs`.
    godot::Ref<Location> current_location;
    godot::TypedArray<Prop> possessions;
    godot::TypedArray<Relationship> relationships;
    godot::TypedArray<Plot> goals;

    // Direct internal mutators for IdeamUndoRedo callables and fallback execution
    void _apply_location_direct(const godot::Ref<Location> &p_new_location);
    void _insert_possession_at_direct(int p_index, const godot::Ref<Prop> &p_prop);
    void _remove_possession_at_direct(int p_index);
    void _insert_goal_at_direct(int p_index, const godot::Ref<Plot> &p_goal);
    void _remove_goal_at_direct(int p_index);
    void _insert_relationship_at_direct(int p_index, const godot::Ref<Relationship> &p_relationship);
    void _remove_relationship_at_direct(int p_index);
    void _set_relationship_relation_direct(int p_id, const godot::Ref<Narreme> &p_new_relation);
    void _set_relationship_title_direct(int p_id, const godot::String &p_new_title);

protected:
    static void _bind_methods();

public:
    Character();
    ~Character() override;

    virtual void initialize() override;

    // Location Management (IdeamUndoRedo-aware)
    void enter_location(const godot::Ref<Location> &p_new_location);
    void leave_current_location();

    [[nodiscard]] godot::Ref<Location> get_current_location() const;
    void set_current_location(const godot::Ref<Location> &p_location);

    // Possession Management (IdeamUndoRedo-aware)
    int take_possession(const godot::Ref<Prop> &p_possession);
    [[nodiscard]] int get_possession_id(const godot::Ref<Prop> &p_possession) const;
    [[nodiscard]] bool has_possession_of(const godot::Ref<Prop> &p_possession) const;
    void lose_possession(const godot::Ref<Prop> &p_possession);
    void lose_possession_at(int p_id);

    [[nodiscard]] godot::TypedArray<Prop> get_possessions() const;
    void set_possessions(const godot::TypedArray<Prop> &p_possessions);

    // Goal Management (IdeamUndoRedo-aware)
    int set_goal(const godot::Ref<Plot> &p_goal);
    [[nodiscard]] int get_goal_id(const godot::Ref<Plot> &p_goal) const;
    [[nodiscard]] bool has_goal(const godot::Ref<Plot> &p_goal) const;
    void remove_goal(const godot::Ref<Plot> &p_goal);
    void remove_goal_at(int p_goal_id);

    int check_goal_status(const godot::Ref<Plot> &p_goal);
    int check_goal_status_at(int p_goal_id);
    void goal_success(int p_goal_id);
    void goal_fail(int p_goal_id);

    [[nodiscard]] godot::TypedArray<Plot> get_goals() const;
    void set_goals(const godot::TypedArray<Plot> &p_goals);

    // Relationship Management (IdeamUndoRedo-aware)
    int begin_relationship(const godot::Ref<Narreme> &p_relation, const godot::String &p_title);
    bool end_relationship(const godot::Ref<Relationship> &p_relationship);
    bool end_relationship_at(int p_id);
    bool change_relation(int p_id, const godot::Ref<Narreme> &p_new_relation);
    bool change_relationship(int p_id, const godot::String &p_new_title);
    [[nodiscard]] int get_relationship_id(const godot::String &p_title) const;
    [[nodiscard]] godot::TypedArray<int> get_relation_ids(const godot::Ref<Narreme> &p_target) const;

    [[nodiscard]] godot::TypedArray<Relationship> get_relationships() const;
    void set_relationships(const godot::TypedArray<Relationship> &p_relationships);

    // Overrides
    virtual godot::TypedArray<godot::Dictionary> get_narrative_condition_descriptors(Narreme *p_narreme = nullptr) const override;
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext