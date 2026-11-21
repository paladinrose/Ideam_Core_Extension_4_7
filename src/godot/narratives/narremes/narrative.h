#pragma once

#include "../narreme.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace ideam::godot_ext {

// Forward Declarations
class Character;
class Prop;
class Location;
class Incident;
class Plot;
class Relationship;

enum BestNarrativeFit : int8_t {
    FIT_BASIC = 0,
    FIT_SYMBOLIC = 1,
    FIT_LITERAL = 2,
    FIT_THOROUGH = 3
};

enum NarrativeReplacement : int8_t {
    REPLACE_SYMBOLIC = 0,
    REPLACE_LITERAL = 1,
    REPLACE_THOROUGH = 2
};

enum NarremeFilterMode : int8_t {
    FILTER_ALL = 0,
    FILTER_LITERAL_ONLY = 1,
    FILTER_SYMBOLIC_ONLY = 2
};

class Narrative : public Narreme {
    GDCLASS(Narrative, Narreme)

private:
    bool begin_narrative_on_ready = true;

    // Characters
    godot::TypedArray<Character> characters;
    godot::TypedArray<Character> roles;
    godot::TypedArray<int> role_assignments;

    // Props
    godot::TypedArray<Prop> props;
    godot::TypedArray<Prop> symbols;
    godot::TypedArray<int> symbol_assignments;

    // Locations
    godot::TypedArray<Location> locations;
    godot::TypedArray<Location> places;
    godot::TypedArray<int> place_assignments;

    // Incidents
    godot::TypedArray<Incident> incidents;
    godot::TypedArray<Incident> moments;
    godot::TypedArray<int> moment_assignments;

    // Plots
    godot::TypedArray<Plot> plots;
    godot::TypedArray<Plot> stories;
    godot::TypedArray<int> story_assignments;

    // Atomic roster snapshot helpers for IdeamUndoRedo
    [[nodiscard]] godot::Dictionary _capture_roster_snapshot() const;
    void _apply_roster_snapshot(const godot::Dictionary &p_snapshot);
    void _record_roster_undo(const godot::String &p_action_name, const godot::Dictionary &p_before_snapshot);

    static void _shift_assignments_after_literal_removal(godot::TypedArray<int> &p_assignments, int p_removed_literal_id);

protected:
    static void _bind_methods();

public:
    Narrative();
    ~Narrative() override;

    virtual void initialize() override;

    // Propagates `p_undo_redo` to this Narrative and all owned Narremes/Helpers
    void propagate_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo);

    // Properties Getters & Setters
    void set_begin_narrative_on_ready(bool p_val);
    [[nodiscard]] bool get_begin_narrative_on_ready() const;

    void set_characters(const godot::TypedArray<Character> &p_arr);
    [[nodiscard]] godot::TypedArray<Character> get_characters() const;
    void set_roles(const godot::TypedArray<Character> &p_arr);
    [[nodiscard]] godot::TypedArray<Character> get_roles() const;
    void set_role_assignments(const godot::TypedArray<int> &p_arr);
    [[nodiscard]] godot::TypedArray<int> get_role_assignments() const;

    void set_props(const godot::TypedArray<Prop> &p_arr);
    [[nodiscard]] godot::TypedArray<Prop> get_props() const;
    void set_symbols(const godot::TypedArray<Prop> &p_arr);
    [[nodiscard]] godot::TypedArray<Prop> get_symbols() const;
    void set_symbol_assignments(const godot::TypedArray<int> &p_arr);
    [[nodiscard]] godot::TypedArray<int> get_symbol_assignments() const;

    void set_locations(const godot::TypedArray<Location> &p_arr);
    [[nodiscard]] godot::TypedArray<Location> get_locations() const;
    void set_places(const godot::TypedArray<Location> &p_arr);
    [[nodiscard]] godot::TypedArray<Location> get_places() const;
    void set_place_assignments(const godot::TypedArray<int> &p_arr);
    [[nodiscard]] godot::TypedArray<int> get_place_assignments() const;

    void set_incidents(const godot::TypedArray<Incident> &p_arr);
    [[nodiscard]] godot::TypedArray<Incident> get_incidents() const;
    void set_moments(const godot::TypedArray<Incident> &p_arr);
    [[nodiscard]] godot::TypedArray<Incident> get_moments() const;
    void set_moment_assignments(const godot::TypedArray<int> &p_arr);
    [[nodiscard]] godot::TypedArray<int> get_moment_assignments() const;

    void set_plots(const godot::TypedArray<Plot> &p_arr);
    [[nodiscard]] godot::TypedArray<Plot> get_plots() const;
    void set_stories(const godot::TypedArray<Plot> &p_arr);
    [[nodiscard]] godot::TypedArray<Plot> get_stories() const;
    void set_story_assignments(const godot::TypedArray<int> &p_arr);
    [[nodiscard]] godot::TypedArray<int> get_story_assignments() const;

    // Narrative Base Functions
    void setup_narrative();
    void begin_narrative();
    void end_narrative();
    [[nodiscard]] godot::TypedArray<godot::String> get_symbolic_forms() const;
    [[nodiscard]] godot::TypedArray<godot::String> get_symbolic_lists() const;
    [[nodiscard]] godot::Ref<Narreme> find_literal_from_symbolic(const godot::Ref<Narreme> &narreme) const;
    [[nodiscard]] godot::Ref<Narreme> find_symbolic_from_literal(const godot::Ref<Narreme> &narreme) const;
    [[nodiscard]] godot::Ref<Narreme> find_narreme_from_symbolic_relationship(const godot::Ref<Relationship> &relationship) const;

    // Unified Query & Filtering Helpers (used by NarrativeConditionField & NarrativeControl)
    [[nodiscard]] godot::TypedArray<Narreme> get_all_narremes(
        NarremeFilterMode p_filter = FILTER_ALL,
        const godot::String &p_class_filter = ""
    ) const;
    [[nodiscard]] bool is_narreme_symbolic(const godot::Ref<Narreme> &p_narreme) const;
    [[nodiscard]] bool is_narreme_literal(const godot::Ref<Narreme> &p_narreme) const;

    // Real <-> Symbolic Conversion & Instantiation Utilities (IdeamUndoRedo-aware)
    bool convert_character_to_role(int p_character_id);
    bool convert_role_to_character(int p_role_id);
    bool convert_prop_to_symbol(int p_prop_id);
    bool convert_symbol_to_prop(int p_symbol_id);
    bool convert_location_to_place(int p_location_id);
    bool convert_place_to_location(int p_place_id);
    bool convert_incident_to_moment(int p_incident_id);
    bool convert_moment_to_incident(int p_moment_id);
    bool convert_plot_to_story(int p_plot_id);
    bool convert_story_to_plot(int p_story_id);
    bool convert_to_symbolic(const godot::Ref<Narreme> &p_narreme);
    bool convert_to_literal(const godot::Ref<Narreme> &p_narreme);

    godot::Ref<Character> instantiate_character_from_role(
        int p_role_id,
        const godot::String &p_new_name = "",
        bool p_auto_fill = true,
        NarrativeReplacement p_replace_type = REPLACE_SYMBOLIC
    );
    godot::Ref<Prop> instantiate_prop_from_symbol(
        int p_symbol_id,
        const godot::String &p_new_name = "",
        bool p_auto_fill = true,
        NarrativeReplacement p_replace_type = REPLACE_SYMBOLIC
    );
    godot::Ref<Location> instantiate_location_from_place(
        int p_place_id,
        const godot::String &p_new_name = "",
        bool p_auto_fill = true,
        NarrativeReplacement p_replace_type = REPLACE_SYMBOLIC
    );
    godot::Ref<Incident> instantiate_incident_from_moment(
        int p_moment_id,
        const godot::String &p_new_name = "",
        bool p_auto_fill = true,
        NarrativeReplacement p_replace_type = REPLACE_SYMBOLIC
    );
    godot::Ref<Plot> instantiate_plot_from_story(
        int p_story_id,
        const godot::String &p_new_name = "",
        bool p_auto_fill = true,
        NarrativeReplacement p_replace_type = REPLACE_SYMBOLIC
    );
    godot::Ref<Narreme> instantiate_literal_from_symbolic(
        const godot::Ref<Narreme> &p_symbolic,
        const godot::String &p_new_name = "",
        bool p_auto_fill = true
    );

    // Character / Role Functions
    [[nodiscard]] godot::TypedArray<godot::String> gather_character_names() const;
    int add_character(const godot::Ref<Character> &new_character);
    [[nodiscard]] int get_character_id(const godot::Ref<Character> &character) const;
    [[nodiscard]] bool has_character(const godot::Ref<Character> &character) const;
    bool remove_character(const godot::Ref<Character> &character);
    bool remove_character_at(int character_ID);
    [[nodiscard]] godot::TypedArray<int> get_all_roles_character_is_filling(int character_ID) const;

    void gather_roles();
    [[nodiscard]] godot::TypedArray<godot::String> gather_role_names() const;
    int add_role(const godot::Ref<Character> &new_role);
    [[nodiscard]] int get_role_id(const godot::Ref<Character> &role) const;
    [[nodiscard]] bool has_role(const godot::Ref<Character> &role) const;
    bool remove_role(const godot::Ref<Character> &role);
    bool remove_role_at(int role_ID);
    int get_best_fit_character_for_role(int role_ID, const godot::TypedArray<int> &character_mask, BestNarrativeFit best_fit);
    bool fill_role(int role_ID, int character_ID, NarrativeReplacement replace_type);
    bool vacate_role(int role_ID);

    // Prop / Symbol Functions
    [[nodiscard]] godot::TypedArray<godot::String> gather_prop_names() const;
    int add_prop(const godot::Ref<Prop> &new_prop);
    [[nodiscard]] int get_prop_id(const godot::Ref<Prop> &prop) const;
    [[nodiscard]] bool has_prop(const godot::Ref<Prop> &prop) const;
    bool remove_prop(const godot::Ref<Prop> &prop);
    bool remove_prop_at(int prop_ID);
    [[nodiscard]] godot::TypedArray<int> get_all_symbols_prop_is_filling(int prop_ID) const;

    void gather_symbols();
    [[nodiscard]] godot::TypedArray<godot::String> gather_symbol_names() const;
    int add_symbol(const godot::Ref<Prop> &new_symbol);
    [[nodiscard]] int get_symbol_id(const godot::Ref<Prop> &symbol) const;
    [[nodiscard]] bool has_symbol(const godot::Ref<Prop> &symbol) const;
    bool remove_symbol(const godot::Ref<Prop> &symbol);
    bool remove_symbol_at(int symbol_ID);
    int get_best_fit_prop_for_symbol(int symbol_ID, const godot::TypedArray<int> &prop_mask, BestNarrativeFit best_fit);
    bool fill_symbol(int symbol_ID, int prop_ID, NarrativeReplacement replace_type);
    bool vacate_symbol(int symbol_ID);

    // Location / Place Functions
    [[nodiscard]] godot::TypedArray<godot::String> gather_location_names() const;
    int add_location(const godot::Ref<Location> &new_location);
    [[nodiscard]] int get_location_id(const godot::Ref<Location> &location) const;
    [[nodiscard]] bool has_location(const godot::Ref<Location> &location) const;
    bool remove_location(const godot::Ref<Location> &location);
    bool remove_location_at(int location_ID);
    [[nodiscard]] godot::TypedArray<int> get_all_places_location_is_filling(int location_ID) const;

    void gather_places();
    [[nodiscard]] godot::TypedArray<godot::String> gather_place_names() const;
    int add_place(const godot::Ref<Location> &new_place);
    [[nodiscard]] int get_place_id(const godot::Ref<Location> &place) const;
    [[nodiscard]] bool has_place(const godot::Ref<Location> &place) const;
    bool remove_place(const godot::Ref<Location> &place);
    bool remove_place_at(int place_ID);
    int get_best_fit_location_for_place(int place_ID, const godot::TypedArray<int> &location_mask, BestNarrativeFit best_fit);
    bool fill_place(int place_ID, int location_ID, NarrativeReplacement replace_type);
    bool vacate_place(int place_ID);

    // Incident / Moment Functions
    [[nodiscard]] godot::TypedArray<godot::String> gather_incident_names() const;
    int add_incident(const godot::Ref<Incident> &new_incident);
    [[nodiscard]] int get_incident_id(const godot::Ref<Incident> &incident) const;
    [[nodiscard]] bool has_incident(const godot::Ref<Incident> &incident) const;
    bool remove_incident(const godot::Ref<Incident> &incident);
    bool remove_incident_at(int incident_ID);
    [[nodiscard]] godot::TypedArray<int> get_all_moments_incident_is_filling(int incident_ID) const;

    void gather_moments();
    [[nodiscard]] godot::TypedArray<godot::String> gather_moment_names() const;
    int add_moment(const godot::Ref<Incident> &new_moment);
    [[nodiscard]] int get_moment_id(const godot::Ref<Incident> &moment) const;
    [[nodiscard]] bool has_moment(const godot::Ref<Incident> &moment) const;
    bool remove_moment(const godot::Ref<Incident> &moment);
    bool remove_moment_at(int moment_ID);
    int get_best_fit_incident_for_moment(int moment_ID, const godot::TypedArray<int> &incident_mask, BestNarrativeFit best_fit);
    bool fill_moment(int moment_ID, int incident_ID, NarrativeReplacement replace_type);
    bool vacate_moment(int moment_ID);

    // Plot / Story Functions
    [[nodiscard]] godot::TypedArray<godot::String> gather_plot_names() const;
    int add_plot(const godot::Ref<Plot> &new_plot);
    [[nodiscard]] int get_plot_id(const godot::Ref<Plot> &plot) const;
    [[nodiscard]] bool has_plot(const godot::Ref<Plot> &plot) const;
    bool remove_plot(const godot::Ref<Plot> &plot);
    bool remove_plot_at(int plot_ID);
    [[nodiscard]] godot::TypedArray<int> get_all_storys_plot_is_filling(int plot_ID) const;

    void gather_stories();
    [[nodiscard]] godot::TypedArray<godot::String> gather_story_names() const;
    int add_story(const godot::Ref<Plot> &new_story);
    [[nodiscard]] int get_story_id(const godot::Ref<Plot> &story) const;
    [[nodiscard]] bool has_story(const godot::Ref<Plot> &story) const;
    bool remove_story(const godot::Ref<Plot> &story);
    bool remove_story_at(int story_ID);
    int get_best_fit_plot_for_story(int story_ID, const godot::TypedArray<int> &plot_mask, BestNarrativeFit best_fit);
    bool fill_story(int story_ID, int plot_ID, NarrativeReplacement replace_type);
    bool vacate_story(int story_ID);

    // Utilities / Overrides
    void path_has_changed(const godot::Ref<Narreme> &node);
    virtual godot::TypedArray<godot::Dictionary> get_narrative_condition_descriptors(Narreme *p_narreme = nullptr) const override;
    virtual godot::Array get_narrative_conditions(Narreme *p_narreme = nullptr) const override;
    virtual NarremeConditionStatus check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const override;
    virtual godot::String get_class_name_str() const override;
};

} // namespace ideam::godot_ext

VARIANT_ENUM_CAST(ideam::godot_ext::BestNarrativeFit);
VARIANT_ENUM_CAST(ideam::godot_ext::NarrativeReplacement);
VARIANT_ENUM_CAST(ideam::godot_ext::NarremeFilterMode);