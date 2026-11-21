#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Character;
class Location;

class Prop : public Narreme {
    GDCLASS(Prop, Narreme)

private:
    // DOD NOTE: Storing direct references forces pointer-chasing and breaks
    // spatial locality. In IDEAM Core, 'possessor' and 'current_location'
    // will just be 32-bit or 64-bit integer IDs mapped via SparseSetView.
    godot::Ref<Character> possessor;
    godot::Ref<Location> current_location;

    void _apply_possessor_direct(const godot::Ref<Character> &p_new_possessor, bool p_as_lost = false);
    void _apply_location_direct(const godot::Ref<Location> &p_new_location);

protected:
    static void _bind_methods();

public:
    Prop();
    ~Prop() override;

    virtual void initialize() override;

    // Getters / Setters
    void set_possessor(const godot::Ref<Character> &p_possessor);
    [[nodiscard]] godot::Ref<Character> get_possessor() const;

    void set_current_location(const godot::Ref<Location> &p_location);
    [[nodiscard]] godot::Ref<Location> get_current_location() const;

    // Class Functions (IdeamUndoRedo-aware)
    void taken_by(const godot::Ref<Character> &new_possessor);
    void released_by(const godot::Ref<Character> &old_possessor);
    void become_lost();
    void found_by(const godot::Ref<Character> &finder);

    void enter_location(const godot::Ref<Location> &new_location);
    void leave_current_location();

    // Overrides
    virtual godot::TypedArray<godot::Dictionary> get_narrative_condition_descriptors(Narreme *p_narreme = nullptr) const override;
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext