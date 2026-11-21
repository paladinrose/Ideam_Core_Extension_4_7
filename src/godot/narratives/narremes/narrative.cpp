#include "narrative.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "character.h"
#include "prop.h"
#include "location.h"
#include "incident.h"
#include "plot.h"
#include "../helpers/relationship.h"
#include "../helpers/incident_condition.h"
#include "../helpers/plot_event.h"
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace ideam::godot_ext {

void Narrative::_bind_methods() {
    // Basic Properties
    godot::ClassDB::bind_method(godot::D_METHOD("set_begin_narrative_on_ready", "begin_narrative_on_ready"), &Narrative::set_begin_narrative_on_ready);
    godot::ClassDB::bind_method(godot::D_METHOD("get_begin_narrative_on_ready"), &Narrative::get_begin_narrative_on_ready);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::BOOL, "begin_narrative_on_ready"), "set_begin_narrative_on_ready", "get_begin_narrative_on_ready");

    // Characters / Roles
    godot::ClassDB::bind_method(godot::D_METHOD("set_characters", "characters"), &Narrative::set_characters);
    godot::ClassDB::bind_method(godot::D_METHOD("get_characters"), &Narrative::get_characters);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "characters", godot::PROPERTY_HINT_ARRAY_TYPE, "Character"), "set_characters", "get_characters");
    godot::ClassDB::bind_method(godot::D_METHOD("set_roles", "roles"), &Narrative::set_roles);
    godot::ClassDB::bind_method(godot::D_METHOD("get_roles"), &Narrative::get_roles);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "roles", godot::PROPERTY_HINT_ARRAY_TYPE, "Character"), "set_roles", "get_roles");
    godot::ClassDB::bind_method(godot::D_METHOD("set_role_assignments", "role_assignments"), &Narrative::set_role_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_role_assignments"), &Narrative::get_role_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "role_assignments", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_role_assignments", "get_role_assignments");

    // Props / Symbols
    godot::ClassDB::bind_method(godot::D_METHOD("set_props", "props"), &Narrative::set_props);
    godot::ClassDB::bind_method(godot::D_METHOD("get_props"), &Narrative::get_props);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "props", godot::PROPERTY_HINT_ARRAY_TYPE, "Prop"), "set_props", "get_props");
    godot::ClassDB::bind_method(godot::D_METHOD("set_symbols", "symbols"), &Narrative::set_symbols);
    godot::ClassDB::bind_method(godot::D_METHOD("get_symbols"), &Narrative::get_symbols);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "symbols", godot::PROPERTY_HINT_ARRAY_TYPE, "Prop"), "set_symbols", "get_symbols");
    godot::ClassDB::bind_method(godot::D_METHOD("set_symbol_assignments", "symbol_assignments"), &Narrative::set_symbol_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_symbol_assignments"), &Narrative::get_symbol_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "symbol_assignments", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_symbol_assignments", "get_symbol_assignments");

    // Locations / Places
    godot::ClassDB::bind_method(godot::D_METHOD("set_locations", "locations"), &Narrative::set_locations);
    godot::ClassDB::bind_method(godot::D_METHOD("get_locations"), &Narrative::get_locations);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "locations", godot::PROPERTY_HINT_ARRAY_TYPE, "Location"), "set_locations", "get_locations");
    godot::ClassDB::bind_method(godot::D_METHOD("set_places", "places"), &Narrative::set_places);
    godot::ClassDB::bind_method(godot::D_METHOD("get_places"), &Narrative::get_places);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "places", godot::PROPERTY_HINT_ARRAY_TYPE, "Location"), "set_places", "get_places");
    godot::ClassDB::bind_method(godot::D_METHOD("set_place_assignments", "place_assignments"), &Narrative::set_place_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_place_assignments"), &Narrative::get_place_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "place_assignments", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_place_assignments", "get_place_assignments");

    // Incidents / Moments
    godot::ClassDB::bind_method(godot::D_METHOD("set_incidents", "incidents"), &Narrative::set_incidents);
    godot::ClassDB::bind_method(godot::D_METHOD("get_incidents"), &Narrative::get_incidents);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "incidents", godot::PROPERTY_HINT_ARRAY_TYPE, "Incident"), "set_incidents", "get_incidents");
    godot::ClassDB::bind_method(godot::D_METHOD("set_moments", "moments"), &Narrative::set_moments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_moments"), &Narrative::get_moments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "moments", godot::PROPERTY_HINT_ARRAY_TYPE, "Incident"), "set_moments", "get_moments");
    godot::ClassDB::bind_method(godot::D_METHOD("set_moment_assignments", "moment_assignments"), &Narrative::set_moment_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_moment_assignments"), &Narrative::get_moment_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "moment_assignments", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_moment_assignments", "get_moment_assignments");

    // Plots / Stories
    godot::ClassDB::bind_method(godot::D_METHOD("set_plots", "plots"), &Narrative::set_plots);
    godot::ClassDB::bind_method(godot::D_METHOD("get_plots"), &Narrative::get_plots);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "plots", godot::PROPERTY_HINT_ARRAY_TYPE, "Plot"), "set_plots", "get_plots");
    godot::ClassDB::bind_method(godot::D_METHOD("set_stories", "stories"), &Narrative::set_stories);
    godot::ClassDB::bind_method(godot::D_METHOD("get_stories"), &Narrative::get_stories);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "stories", godot::PROPERTY_HINT_ARRAY_TYPE, "Plot"), "set_stories", "get_stories");
    godot::ClassDB::bind_method(godot::D_METHOD("set_story_assignments", "story_assignments"), &Narrative::set_story_assignments);
    godot::ClassDB::bind_method(godot::D_METHOD("get_story_assignments"), &Narrative::get_story_assignments);
    ADD_PROPERTY(godot::PropertyInfo(godot::Variant::ARRAY, "story_assignments", godot::PROPERTY_HINT_ARRAY_TYPE, "int"), "set_story_assignments", "get_story_assignments");

    // Utilities
    godot::ClassDB::bind_method(godot::D_METHOD("setup_narrative"), &Narrative::setup_narrative);
    godot::ClassDB::bind_method(godot::D_METHOD("begin_narrative"), &Narrative::begin_narrative);
    godot::ClassDB::bind_method(godot::D_METHOD("end_narrative"), &Narrative::end_narrative);
    godot::ClassDB::bind_method(godot::D_METHOD("get_symbolic_forms"), &Narrative::get_symbolic_forms);
    godot::ClassDB::bind_method(godot::D_METHOD("get_symbolic_lists"), &Narrative::get_symbolic_lists);
    godot::ClassDB::bind_method(godot::D_METHOD("find_literal_from_symbolic", "narreme"), &Narrative::find_literal_from_symbolic);
    godot::ClassDB::bind_method(godot::D_METHOD("find_narreme_from_symbolic_relationship", "relationship"), &Narrative::find_narreme_from_symbolic_relationship);
    godot::ClassDB::bind_method(godot::D_METHOD("path_has_changed", "node"), &Narrative::path_has_changed);
    godot::ClassDB::bind_method(godot::D_METHOD("get_class_name_str"), &Narrative::get_class_name_str);

    // Foundational Undo/Redo, Query, Conversion & Instantiation Methods
    godot::ClassDB::bind_method(godot::D_METHOD("propagate_undo_redo", "undo_redo"), &Narrative::propagate_undo_redo);
    godot::ClassDB::bind_method(godot::D_METHOD("find_symbolic_from_literal", "narreme"), &Narrative::find_symbolic_from_literal);
    godot::ClassDB::bind_method(
        godot::D_METHOD("get_all_narremes", "filter", "class_filter"),
        &Narrative::get_all_narremes,
        DEFVAL(FILTER_ALL),
        DEFVAL(godot::String())
    );
    godot::ClassDB::bind_method(godot::D_METHOD("is_narreme_symbolic", "narreme"), &Narrative::is_narreme_symbolic);
    godot::ClassDB::bind_method(godot::D_METHOD("is_narreme_literal", "narreme"), &Narrative::is_narreme_literal);

    godot::ClassDB::bind_method(godot::D_METHOD("convert_character_to_role", "character_id"), &Narrative::convert_character_to_role);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_role_to_character", "role_id"), &Narrative::convert_role_to_character);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_prop_to_symbol", "prop_id"), &Narrative::convert_prop_to_symbol);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_symbol_to_prop", "symbol_id"), &Narrative::convert_symbol_to_prop);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_location_to_place", "location_id"), &Narrative::convert_location_to_place);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_place_to_location", "place_id"), &Narrative::convert_place_to_location);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_incident_to_moment", "incident_id"), &Narrative::convert_incident_to_moment);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_moment_to_incident", "moment_id"), &Narrative::convert_moment_to_incident);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_plot_to_story", "plot_id"), &Narrative::convert_plot_to_story);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_story_to_plot", "story_id"), &Narrative::convert_story_to_plot);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_to_symbolic", "narreme"), &Narrative::convert_to_symbolic);
    godot::ClassDB::bind_method(godot::D_METHOD("convert_to_literal", "narreme"), &Narrative::convert_to_literal);

    godot::ClassDB::bind_method(
        godot::D_METHOD("instantiate_character_from_role", "role_id", "new_name", "auto_fill", "replace_type"),
        &Narrative::instantiate_character_from_role,
        DEFVAL(godot::String()), DEFVAL(true), DEFVAL(REPLACE_SYMBOLIC)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("instantiate_prop_from_symbol", "symbol_id", "new_name", "auto_fill", "replace_type"),
        &Narrative::instantiate_prop_from_symbol,
        DEFVAL(godot::String()), DEFVAL(true), DEFVAL(REPLACE_SYMBOLIC)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("instantiate_location_from_place", "place_id", "new_name", "auto_fill", "replace_type"),
        &Narrative::instantiate_location_from_place,
        DEFVAL(godot::String()), DEFVAL(true), DEFVAL(REPLACE_SYMBOLIC)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("instantiate_incident_from_moment", "moment_id", "new_name", "auto_fill", "replace_type"),
        &Narrative::instantiate_incident_from_moment,
        DEFVAL(godot::String()), DEFVAL(true), DEFVAL(REPLACE_SYMBOLIC)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("instantiate_plot_from_story", "story_id", "new_name", "auto_fill", "replace_type"),
        &Narrative::instantiate_plot_from_story,
        DEFVAL(godot::String()), DEFVAL(true), DEFVAL(REPLACE_SYMBOLIC)
    );
    godot::ClassDB::bind_method(
        godot::D_METHOD("instantiate_literal_from_symbolic", "symbolic", "new_name", "auto_fill"),
        &Narrative::instantiate_literal_from_symbolic,
        DEFVAL(godot::String()), DEFVAL(true)
    );

    // Character / Role Roster Methods
    godot::ClassDB::bind_method(godot::D_METHOD("gather_character_names"), &Narrative::gather_character_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_character", "new_character"), &Narrative::add_character);
    godot::ClassDB::bind_method(godot::D_METHOD("get_character_id", "character"), &Narrative::get_character_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_character", "character"), &Narrative::has_character);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_character", "character"), &Narrative::remove_character);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_character_at", "character_ID"), &Narrative::remove_character_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_all_roles_character_is_filling", "character_ID"), &Narrative::get_all_roles_character_is_filling);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_role_names"), &Narrative::gather_role_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_role", "new_role"), &Narrative::add_role);
    godot::ClassDB::bind_method(godot::D_METHOD("get_role_id", "role"), &Narrative::get_role_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_role", "role"), &Narrative::has_role);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_role", "role"), &Narrative::remove_role);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_role_at", "role_ID"), &Narrative::remove_role_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_best_fit_character_for_role", "role_ID", "character_mask", "best_fit"), &Narrative::get_best_fit_character_for_role);
    godot::ClassDB::bind_method(godot::D_METHOD("fill_role", "role_ID", "character_ID", "replace_type"), &Narrative::fill_role);
    godot::ClassDB::bind_method(godot::D_METHOD("vacate_role", "role_ID"), &Narrative::vacate_role);

    // Prop / Symbol Roster Methods
    godot::ClassDB::bind_method(godot::D_METHOD("gather_prop_names"), &Narrative::gather_prop_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_prop", "new_prop"), &Narrative::add_prop);
    godot::ClassDB::bind_method(godot::D_METHOD("get_prop_id", "prop"), &Narrative::get_prop_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_prop", "prop"), &Narrative::has_prop);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_prop", "prop"), &Narrative::remove_prop);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_prop_at", "prop_ID"), &Narrative::remove_prop_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_all_symbols_prop_is_filling", "prop_ID"), &Narrative::get_all_symbols_prop_is_filling);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_symbol_names"), &Narrative::gather_symbol_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_symbol", "new_symbol"), &Narrative::add_symbol);
    godot::ClassDB::bind_method(godot::D_METHOD("get_symbol_id", "symbol"), &Narrative::get_symbol_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_symbol", "symbol"), &Narrative::has_symbol);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_symbol", "symbol"), &Narrative::remove_symbol);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_symbol_at", "symbol_ID"), &Narrative::remove_symbol_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_best_fit_prop_for_symbol", "symbol_ID", "prop_mask", "best_fit"), &Narrative::get_best_fit_prop_for_symbol);
    godot::ClassDB::bind_method(godot::D_METHOD("fill_symbol", "symbol_ID", "prop_ID", "replace_type"), &Narrative::fill_symbol);
    godot::ClassDB::bind_method(godot::D_METHOD("vacate_symbol", "symbol_ID"), &Narrative::vacate_symbol);

    // Location / Place Roster Methods
    godot::ClassDB::bind_method(godot::D_METHOD("gather_location_names"), &Narrative::gather_location_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_location", "new_location"), &Narrative::add_location);
    godot::ClassDB::bind_method(godot::D_METHOD("get_location_id", "location"), &Narrative::get_location_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_location", "location"), &Narrative::has_location);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_location", "location"), &Narrative::remove_location);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_location_at", "location_ID"), &Narrative::remove_location_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_all_places_location_is_filling", "location_ID"), &Narrative::get_all_places_location_is_filling);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_place_names"), &Narrative::gather_place_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_place", "new_place"), &Narrative::add_place);
    godot::ClassDB::bind_method(godot::D_METHOD("get_place_id", "place"), &Narrative::get_place_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_place", "place"), &Narrative::has_place);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_place", "place"), &Narrative::remove_place);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_place_at", "place_ID"), &Narrative::remove_place_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_best_fit_location_for_place", "place_ID", "location_mask", "best_fit"), &Narrative::get_best_fit_location_for_place);
    godot::ClassDB::bind_method(godot::D_METHOD("fill_place", "place_ID", "location_ID", "replace_type"), &Narrative::fill_place);
    godot::ClassDB::bind_method(godot::D_METHOD("vacate_place", "place_ID"), &Narrative::vacate_place);

    // Incident / Moment Roster Methods
    godot::ClassDB::bind_method(godot::D_METHOD("gather_incident_names"), &Narrative::gather_incident_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_incident", "new_incident"), &Narrative::add_incident);
    godot::ClassDB::bind_method(godot::D_METHOD("get_incident_id", "incident"), &Narrative::get_incident_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_incident", "incident"), &Narrative::has_incident);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_incident", "incident"), &Narrative::remove_incident);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_incident_at", "incident_ID"), &Narrative::remove_incident_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_all_moments_incident_is_filling", "incident_ID"), &Narrative::get_all_moments_incident_is_filling);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_moment_names"), &Narrative::gather_moment_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_moment", "new_moment"), &Narrative::add_moment);
    godot::ClassDB::bind_method(godot::D_METHOD("get_moment_id", "moment"), &Narrative::get_moment_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_moment", "moment"), &Narrative::has_moment);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_moment", "moment"), &Narrative::remove_moment);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_moment_at", "moment_ID"), &Narrative::remove_moment_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_best_fit_incident_for_moment", "moment_ID", "incident_mask", "best_fit"), &Narrative::get_best_fit_incident_for_moment);
    godot::ClassDB::bind_method(godot::D_METHOD("fill_moment", "moment_ID", "incident_ID", "replace_type"), &Narrative::fill_moment);
    godot::ClassDB::bind_method(godot::D_METHOD("vacate_moment", "moment_ID"), &Narrative::vacate_moment);

    // Plot / Story Roster Methods
    godot::ClassDB::bind_method(godot::D_METHOD("gather_plot_names"), &Narrative::gather_plot_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_plot", "new_plot"), &Narrative::add_plot);
    godot::ClassDB::bind_method(godot::D_METHOD("get_plot_id", "plot"), &Narrative::get_plot_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_plot", "plot"), &Narrative::has_plot);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_plot", "plot"), &Narrative::remove_plot);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_plot_at", "plot_ID"), &Narrative::remove_plot_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_all_storys_plot_is_filling", "plot_ID"), &Narrative::get_all_storys_plot_is_filling);
    godot::ClassDB::bind_method(godot::D_METHOD("gather_story_names"), &Narrative::gather_story_names);
    godot::ClassDB::bind_method(godot::D_METHOD("add_story", "new_story"), &Narrative::add_story);
    godot::ClassDB::bind_method(godot::D_METHOD("get_story_id", "story"), &Narrative::get_story_id);
    godot::ClassDB::bind_method(godot::D_METHOD("has_story", "story"), &Narrative::has_story);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_story", "story"), &Narrative::remove_story);
    godot::ClassDB::bind_method(godot::D_METHOD("remove_story_at", "story_ID"), &Narrative::remove_story_at);
    godot::ClassDB::bind_method(godot::D_METHOD("get_best_fit_plot_for_story", "story_ID", "plot_mask", "best_fit"), &Narrative::get_best_fit_plot_for_story);
    godot::ClassDB::bind_method(godot::D_METHOD("fill_story", "story_ID", "plot_ID", "replace_type"), &Narrative::fill_story);
    godot::ClassDB::bind_method(godot::D_METHOD("vacate_story", "story_ID"), &Narrative::vacate_story);

    // Enums
    BIND_ENUM_CONSTANT(FIT_BASIC);
    BIND_ENUM_CONSTANT(FIT_SYMBOLIC);
    BIND_ENUM_CONSTANT(FIT_LITERAL);
    BIND_ENUM_CONSTANT(FIT_THOROUGH);
    BIND_ENUM_CONSTANT(REPLACE_SYMBOLIC);
    BIND_ENUM_CONSTANT(REPLACE_LITERAL);
    BIND_ENUM_CONSTANT(REPLACE_THOROUGH);
    BIND_ENUM_CONSTANT(FILTER_ALL);
    BIND_ENUM_CONSTANT(FILTER_LITERAL_ONLY);
    BIND_ENUM_CONSTANT(FILTER_SYMBOLIC_ONLY);

    // Base & Roster Signals
    ADD_SIGNAL(godot::MethodInfo("narrative_begun"));
    ADD_SIGNAL(godot::MethodInfo("narrative_ended"));
    ADD_SIGNAL(godot::MethodInfo("roster_changed"));

    ADD_SIGNAL(godot::MethodInfo("character_added", godot::PropertyInfo(godot::Variant::OBJECT, "character", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("character_removed", godot::PropertyInfo(godot::Variant::OBJECT, "character", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("role_added", godot::PropertyInfo(godot::Variant::OBJECT, "role", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("role_removed", godot::PropertyInfo(godot::Variant::OBJECT, "role", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("role_filled", godot::PropertyInfo(godot::Variant::OBJECT, "role", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character"), godot::PropertyInfo(godot::Variant::OBJECT, "character", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));
    ADD_SIGNAL(godot::MethodInfo("role_vacated", godot::PropertyInfo(godot::Variant::OBJECT, "role", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character"), godot::PropertyInfo(godot::Variant::OBJECT, "character", godot::PROPERTY_HINT_RESOURCE_TYPE, "Character")));

    ADD_SIGNAL(godot::MethodInfo("prop_added", godot::PropertyInfo(godot::Variant::OBJECT, "prop", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("prop_removed", godot::PropertyInfo(godot::Variant::OBJECT, "prop", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("symbol_added", godot::PropertyInfo(godot::Variant::OBJECT, "symbol", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("symbol_removed", godot::PropertyInfo(godot::Variant::OBJECT, "symbol", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("symbol_filled", godot::PropertyInfo(godot::Variant::OBJECT, "symbol", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop"), godot::PropertyInfo(godot::Variant::OBJECT, "prop", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));
    ADD_SIGNAL(godot::MethodInfo("symbol_vacated", godot::PropertyInfo(godot::Variant::OBJECT, "symbol", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop"), godot::PropertyInfo(godot::Variant::OBJECT, "prop", godot::PROPERTY_HINT_RESOURCE_TYPE, "Prop")));

    ADD_SIGNAL(godot::MethodInfo("location_added", godot::PropertyInfo(godot::Variant::OBJECT, "location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("location_removed", godot::PropertyInfo(godot::Variant::OBJECT, "location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("place_added", godot::PropertyInfo(godot::Variant::OBJECT, "place", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("place_removed", godot::PropertyInfo(godot::Variant::OBJECT, "place", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("place_filled", godot::PropertyInfo(godot::Variant::OBJECT, "place", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location"), godot::PropertyInfo(godot::Variant::OBJECT, "location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));
    ADD_SIGNAL(godot::MethodInfo("place_vacated", godot::PropertyInfo(godot::Variant::OBJECT, "place", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location"), godot::PropertyInfo(godot::Variant::OBJECT, "location", godot::PROPERTY_HINT_RESOURCE_TYPE, "Location")));

    ADD_SIGNAL(godot::MethodInfo("incident_added", godot::PropertyInfo(godot::Variant::OBJECT, "incident", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident")));
    ADD_SIGNAL(godot::MethodInfo("incident_removed", godot::PropertyInfo(godot::Variant::OBJECT, "incident", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident")));
    ADD_SIGNAL(godot::MethodInfo("moment_added", godot::PropertyInfo(godot::Variant::OBJECT, "moment", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident")));
    ADD_SIGNAL(godot::MethodInfo("moment_removed", godot::PropertyInfo(godot::Variant::OBJECT, "moment", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident")));
    ADD_SIGNAL(godot::MethodInfo("moment_filled", godot::PropertyInfo(godot::Variant::OBJECT, "moment", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident"), godot::PropertyInfo(godot::Variant::OBJECT, "incident", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident")));
    ADD_SIGNAL(godot::MethodInfo("moment_vacated", godot::PropertyInfo(godot::Variant::OBJECT, "moment", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident"), godot::PropertyInfo(godot::Variant::OBJECT, "incident", godot::PROPERTY_HINT_RESOURCE_TYPE, "Incident")));

    ADD_SIGNAL(godot::MethodInfo("plot_added", godot::PropertyInfo(godot::Variant::OBJECT, "plot", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("plot_removed", godot::PropertyInfo(godot::Variant::OBJECT, "plot", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("story_added", godot::PropertyInfo(godot::Variant::OBJECT, "story", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("story_removed", godot::PropertyInfo(godot::Variant::OBJECT, "story", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("story_filled", godot::PropertyInfo(godot::Variant::OBJECT, "story", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot"), godot::PropertyInfo(godot::Variant::OBJECT, "plot", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
    ADD_SIGNAL(godot::MethodInfo("story_vacated", godot::PropertyInfo(godot::Variant::OBJECT, "story", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot"), godot::PropertyInfo(godot::Variant::OBJECT, "plot", godot::PROPERTY_HINT_RESOURCE_TYPE, "Plot")));
}

Narrative::Narrative() {}
Narrative::~Narrative() {}

void Narrative::initialize() {
    Narreme::initialize();
    
    setup_narrative();
    
    if (begin_narrative_on_ready && !godot::Engine::get_singleton()->is_editor_hint()) {
        begin_narrative();
    }
}

// ----------------------------------------------------------------------------
// GETTERS & SETTERS 
// ----------------------------------------------------------------------------
void Narrative::set_begin_narrative_on_ready(bool p_val) { begin_narrative_on_ready = p_val; }
bool Narrative::get_begin_narrative_on_ready() const { return begin_narrative_on_ready; }
void Narrative::set_characters(const godot::TypedArray<Character> &p_arr) { characters = p_arr; }
godot::TypedArray<Character> Narrative::get_characters() const { return characters; }
void Narrative::set_roles(const godot::TypedArray<Character> &p_arr) { roles = p_arr; }
godot::TypedArray<Character> Narrative::get_roles() const { return roles; }
void Narrative::set_role_assignments(const godot::TypedArray<int> &p_arr) { role_assignments = p_arr; }
godot::TypedArray<int> Narrative::get_role_assignments() const { return role_assignments; }
// Props / Symbols
void Narrative::set_props(const godot::TypedArray<Prop> &p_arr) { props = p_arr; }
godot::TypedArray<Prop> Narrative::get_props() const { return props; }
void Narrative::set_symbols(const godot::TypedArray<Prop> &p_arr) { symbols = p_arr; }
godot::TypedArray<Prop> Narrative::get_symbols() const { return symbols; }
void Narrative::set_symbol_assignments(const godot::TypedArray<int> &p_arr) { symbol_assignments = p_arr; }
godot::TypedArray<int> Narrative::get_symbol_assignments() const { return symbol_assignments; }

// Locations / Places
void Narrative::set_locations(const godot::TypedArray<Location> &p_arr) { locations = p_arr; }
godot::TypedArray<Location> Narrative::get_locations() const { return locations; }
void Narrative::set_places(const godot::TypedArray<Location> &p_arr) { places = p_arr; }
godot::TypedArray<Location> Narrative::get_places() const { return places; }
void Narrative::set_place_assignments(const godot::TypedArray<int> &p_arr) { place_assignments = p_arr; }
godot::TypedArray<int> Narrative::get_place_assignments() const { return place_assignments; }

// Incidents / Moments
void Narrative::set_incidents(const godot::TypedArray<Incident> &p_arr) { incidents = p_arr; }
godot::TypedArray<Incident> Narrative::get_incidents() const { return incidents; }
void Narrative::set_moments(const godot::TypedArray<Incident> &p_arr) { moments = p_arr; }
godot::TypedArray<Incident> Narrative::get_moments() const { return moments; }
void Narrative::set_moment_assignments(const godot::TypedArray<int> &p_arr) { moment_assignments = p_arr; }
godot::TypedArray<int> Narrative::get_moment_assignments() const { return moment_assignments; }

// Plots / Stories
void Narrative::set_plots(const godot::TypedArray<Plot> &p_arr) { plots = p_arr; }
godot::TypedArray<Plot> Narrative::get_plots() const { return plots; }
void Narrative::set_stories(const godot::TypedArray<Plot> &p_arr) { stories = p_arr; }
godot::TypedArray<Plot> Narrative::get_stories() const { return stories; }
void Narrative::set_story_assignments(const godot::TypedArray<int> &p_arr) { story_assignments = p_arr; }
godot::TypedArray<int> Narrative::get_story_assignments() const { return story_assignments; }
// ----------------------------------------------------------------------------
// BASE NARRATIVE LOGIC
// ----------------------------------------------------------------------------

void Narrative::setup_narrative() {
    // Note: Since we use Resources, we don't 'gather' from children nodes anymore.
    // The authoring UI must ensure the TypedArrays are populated.
    // We just ensure the arrays match length if needed.
    
    // gather_roles();
    // gather_symbols();
    // gather_places();
    // gather_moments();
    // gather_stories();
}

void Narrative::begin_narrative() {
    emit_signal("narrative_begun");
}

void Narrative::end_narrative() {
    emit_signal("narrative_ended");
}

godot::TypedArray<godot::String> Narrative::get_symbolic_forms() const {
    godot::TypedArray<godot::String> forms;
    forms.append("role");
    forms.append("symbol");
    forms.append("place");
    forms.append("moment");
    forms.append("story");
    return forms;
}

godot::TypedArray<godot::String> Narrative::get_symbolic_lists() const {
    godot::TypedArray<godot::String> lists;
    lists.append("roles");
    lists.append("symbols");
    lists.append("places");
    lists.append("moments");
    lists.append("stories");
    return lists;
}

godot::Ref<Narreme> Narrative::find_literal_from_symbolic(const godot::Ref<Narreme> &narreme) const {
    if (!narreme.is_valid()) return nullptr;
    
    godot::String class_name = narreme->get_class_name_str();
    
    if (class_name == "Character") {
        for (int i = 0; i < roles.size(); i++) {
            // Explicitly cast the Variant to a Ref before comparison
            godot::Ref<Character> role = roles[i];
            if (role == narreme) {
                // Explicitly cast the Variant to an int before mathematical comparison and index use
                if (i < role_assignments.size() && (int)role_assignments[i] >= 0) return characters[(int)role_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Prop") {
        for (int i = 0; i < symbols.size(); i++) {
            godot::Ref<Prop> symbol = symbols[i];
            if (symbol == narreme) {
                if (i < symbol_assignments.size() && (int)symbol_assignments[i] >= 0) return props[(int)symbol_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Location") {
        for (int i = 0; i < places.size(); i++) {
            godot::Ref<Location> place = places[i];
            if (place == narreme) {
                if (i < place_assignments.size() && (int)place_assignments[i] >= 0) return locations[(int)place_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Incident") {
        for (int i = 0; i < moments.size(); i++) {
            godot::Ref<Incident> moment = moments[i];
            if (moment == narreme) {
                if (i < moment_assignments.size() && (int)moment_assignments[i] >= 0) return incidents[(int)moment_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Plot") {
        for (int i = 0; i < stories.size(); i++) {
            godot::Ref<Plot> story = stories[i];
            if (story == narreme) {
                if (i < story_assignments.size() && (int)story_assignments[i] >= 0) return plots[(int)story_assignments[i]];
                return nullptr;
            }
        }
    }
    
    return nullptr;
}

godot::Ref<Narreme> Narrative::find_narreme_from_symbolic_relationship(const godot::Ref<Relationship> &relationship) const {
    if (!relationship.is_valid() || !relationship->get_relation().is_valid()) return nullptr;
    
    godot::Ref<Narreme> relation = relationship->get_relation();
    godot::String title = relationship->get_title();
    godot::String class_name = relation->get_class_name_str();
    
    if (class_name == "Character") {
        for (int i = 0; i < roles.size(); i++) {
            godot::Ref<Character> role = roles[i];
            if (role.is_valid() && role->get_official_name() == title) {
                if (i < role_assignments.size() && (int)role_assignments[i] >= 0) return characters[(int)role_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Prop") {
        for (int i = 0; i < symbols.size(); i++) {
            godot::Ref<Prop> symbol = symbols[i];
            if (symbol.is_valid() && symbol->get_official_name() == title) {
                if (i < symbol_assignments.size() && (int)symbol_assignments[i] >= 0) return props[(int)symbol_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Location") {
        for (int i = 0; i < places.size(); i++) {
            godot::Ref<Location> place = places[i];
            if (place.is_valid() && place->get_official_name() == title) {
                if (i < place_assignments.size() && (int)place_assignments[i] >= 0) return locations[(int)place_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Incident") {
        for (int i = 0; i < moments.size(); i++) {
            godot::Ref<Incident> moment = moments[i];
            if (moment.is_valid() && moment->get_official_name() == title) {
                if (i < moment_assignments.size() && (int)moment_assignments[i] >= 0) return incidents[(int)moment_assignments[i]];
                return nullptr;
            }
        }
    } else if (class_name == "Plot") {
        for (int i = 0; i < stories.size(); i++) {
            godot::Ref<Plot> story = stories[i];
            if (story.is_valid() && story->get_official_name() == title) {
                if (i < story_assignments.size() && (int)story_assignments[i] >= 0) return plots[(int)story_assignments[i]];
                return nullptr;
            }
        }
    }
    
    return nullptr;
}

void Narrative::path_has_changed(const godot::Ref<Narreme> &node) {
    // Left empty to mirror GDScript `pass` implementation
}

// ----------------------------------------------------------------------------
// CHARACTER FUNCTIONS
// ----------------------------------------------------------------------------

godot::TypedArray<godot::String> Narrative::gather_character_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (character.is_valid()) {
            names.append(character->get_official_name());
        }
    }
    return names;
}

int Narrative::add_character(const godot::Ref<Character> &new_character) {
    int id = get_character_id(new_character);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = characters.size();
        characters.append(new_character);
        // Note: connect to path_has_changed omitted as it is an empty stub in base script
        emit_signal("character_added", new_character);
        _record_roster_undo("Add Character to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_character_id(const godot::Ref<Character> &character) const {
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> c = characters[i];
        if (c == character) return i;
    }
    return -1;
}

bool Narrative::has_character(const godot::Ref<Character> &character) const {
    return get_character_id(character) >= 0;
}

bool Narrative::remove_character(const godot::Ref<Character> &character) {
    int id = get_character_id(character);
    if (id >= 0) return remove_character_at(id);
    return false;
}

bool Narrative::remove_character_at(int character_ID) {
    if (character_ID < 0 || character_ID >= characters.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Character> character = characters[character_ID];
    godot::TypedArray<int> role_IDs = get_all_roles_character_is_filling(character_ID);
    
    for (int i = 0; i < role_IDs.size(); i++) {
        vacate_role((int)role_IDs[i]);
    }
    
    characters.remove_at(character_ID);
    _shift_assignments_after_literal_removal(role_assignments, character_ID);
    emit_signal("character_removed", character);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Character from Narrative", _undo_before);
    return true;
}

godot::TypedArray<int> Narrative::get_all_roles_character_is_filling(int character_ID) const {
    godot::TypedArray<int> role_IDs;
    for (int i = 0; i < role_assignments.size(); i++) {
        if ((int)role_assignments[i] == character_ID) {
            role_IDs.append(i);
        }
    }
    return role_IDs;
}


// ----------------------------------------------------------------------------
// ROLE FUNCTIONS
// ----------------------------------------------------------------------------

void Narrative::gather_roles() {
    // Roles are populated by the Resource inspector, no node traversal needed.
}

godot::TypedArray<godot::String> Narrative::gather_role_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < roles.size(); i++) {
        godot::Ref<Character> role = roles[i];
        if (role.is_valid()) {
            names.append(role->get_official_name());
        }
    }
    return names;
}

int Narrative::add_role(const godot::Ref<Character> &new_role) {
    int id = get_role_id(new_role);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = roles.size();
        roles.append(new_role);
        role_assignments.append(-1);
        emit_signal("role_added", new_role);
        _record_roster_undo("Add Role to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_role_id(const godot::Ref<Character> &role) const {
    for (int i = 0; i < roles.size(); i++) {
        godot::Ref<Character> r = roles[i];
        if (r == role) return i;
    }
    return -1;
}

bool Narrative::has_role(const godot::Ref<Character> &role) const {
    return get_role_id(role) >= 0;
}

bool Narrative::remove_role(const godot::Ref<Character> &role) {
    return remove_role_at(get_role_id(role));
}

bool Narrative::remove_role_at(int role_ID) {
    if (role_ID < 0 || role_ID >= roles.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Character> role = roles[role_ID];
    
    if (role_ID < role_assignments.size() && (int)role_assignments[role_ID] >= 0) {
        vacate_role(role_ID);
    }
    
    roles.remove_at(role_ID);
    role_assignments.remove_at(role_ID);
    
    emit_signal("role_removed", role);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Role from Narrative", _undo_before);
    return true;
}

int Narrative::get_best_fit_character_for_role(int role_ID, const godot::TypedArray<int> &character_mask, BestNarrativeFit best_fit) {
    // DOD NOTE: This is the exact kind of O(N^3) nested loop that Data-Oriented Design eliminates.
    // In IDEAM Core, 'characters' will be a Bitset Selection mask. 'Masking' out characters is just 
    // a SIMD bitwise AND-NOT[cite: 7]. Finding matches across relationships and locations will be done 
    // by intersecting Group Masks [cite: 4] across a BridgeView[cite: 52], evaluating thousands 
    // of constraints in nanoseconds. For the Authoring layer, we preserve the exact GDScript flow.
    
    if (role_ID < 0 || role_ID >= roles.size()) return -1;
    
    int best_score = 100000;
    int best_ID = -1;
    
    godot::Ref<Character> role_to_fill = roles[role_ID];
    if (!role_to_fill.is_valid()) return -1;
    
    godot::Ref<Character> current_character_in_role = nullptr;
    if (role_ID < role_assignments.size() && (int)role_assignments[role_ID] >= 0) {
        current_character_in_role = characters[(int)role_assignments[role_ID]];
    }
    
    for (int c = 0; c < characters.size(); c++) {
        godot::Ref<Character> character = characters[c];
        if (!character.is_valid()) continue;
        
        bool is_masked = false;
        for (int m = 0; m < character_mask.size(); m++) {
            if ((int)character_mask[m] == c) {
                is_masked = true;
                break;
            }
        }
        if (is_masked) continue;
        
        int score = character->score_similarity(role_to_fill.ptr());
        
        if (best_fit == FIT_LITERAL || best_fit == FIT_THOROUGH) {
            if (role_to_fill->get_current_location() != character->get_current_location()) {
                if (best_fit == FIT_LITERAL) score += 1;
                else { // THOROUGH
                    int place_ID = get_place_id(role_to_fill->get_current_location());
                    if (place_ID >= 0 && place_ID < place_assignments.size()) {
                        int place_assignment = (int)place_assignments[place_ID];
                        if (place_assignment >= 0){
                            godot::Ref<Location> loc = locations[place_assignment];
                            if (loc != character->get_current_location()) {
                                score += 1;
                            }
                        }
                    }
                }
            }
        } else if (best_fit == FIT_SYMBOLIC) {
            godot::Ref<Location> role_loc = role_to_fill->get_current_location();
            if (role_loc.is_valid()) {
                int place_ID = get_place_id(role_loc);
                if (place_ID >= 0 && place_ID < place_assignments.size()) {
                    int place_assignment = (int)place_assignments[place_ID];
                    if (place_assignment >= 0){
                        godot::Ref<Location> loc = locations[place_assignment];
                        if(loc != character->get_current_location()) {
                            score += 1;
                        }
                    }
                }
            } else if (character->get_current_location().is_valid()) {
                score += 1;
            }
        }
        
        // Relationships evaluation
        if (best_fit != FIT_BASIC) {
            godot::TypedArray<Relationship> role_rels = role_to_fill->get_relationships();
            godot::TypedArray<Relationship> char_rels = character->get_relationships();
            
            for (int r = 0; r < role_rels.size(); r++) {
                godot::Ref<Relationship> role_rel = role_rels[r];
                if (!role_rel.is_valid()) continue;
                
                bool has_title = false;
                bool has_narreme = false;
                
                for (int cr = 0; cr < char_rels.size(); cr++) {
                    godot::Ref<Relationship> char_rel = char_rels[cr];
                    if (!char_rel.is_valid()) continue;
                    
                    if (char_rel->get_title() == role_rel->get_title()) {
                        has_title = true;
                        
                        if (best_fit == FIT_LITERAL) {
                            if (role_rel->get_relation().is_valid() && role_rel->get_relation() == char_rel->get_relation()) has_narreme = true;
                            else if (!role_rel->get_relation().is_valid() && !char_rel->get_relation().is_valid()) has_narreme = true;
                        } else if (best_fit == FIT_SYMBOLIC) {
                            if (role_rel->get_relation().is_valid()) {
                                godot::Ref<Narreme> role_nar = find_narreme_from_symbolic_relationship(role_rel);
                                if (role_nar.is_valid() && role_nar == char_rel->get_relation()) has_narreme = true;
                            } else if (!role_rel->get_relation().is_valid() && !char_rel->get_relation().is_valid()) has_narreme = true;
                        } else { // THOROUGH
                            if (role_rel->get_relation().is_valid()) {
                                if (role_rel->get_relation() == char_rel->get_relation()) has_narreme = true;
                                else {
                                    godot::Ref<Narreme> role_nar = find_narreme_from_symbolic_relationship(role_rel);
                                    if (role_nar.is_valid() && role_nar == char_rel->get_relation()) has_narreme = true;
                                    else if (!role_nar.is_valid() && !char_rel->get_relation().is_valid()) has_narreme = true;
                                }
                            } else if (!char_rel->get_relation().is_valid()) has_narreme = true;
                        }
                        break;
                    }
                }
                if (!has_title) score += 2;
                if (!has_narreme) score += 1;
            }
        }
        
        // Possessions evaluation
        if (best_fit != FIT_BASIC) {
            godot::TypedArray<Prop> role_pos_arr = role_to_fill->get_possessions();
            godot::TypedArray<Prop> char_pos_arr = character->get_possessions();
            
            for (int p = 0; p < role_pos_arr.size(); p++) {
                godot::Ref<Prop> role_pos = role_pos_arr[p];
                if (!role_pos.is_valid()) continue;
                
                bool has_possession = false;
                godot::Ref<Prop> current_prop_as_symbol = nullptr;
                
                if (best_fit == FIT_SYMBOLIC || best_fit == FIT_THOROUGH) {
                    int symbol_ID = get_symbol_id(role_pos);
                    if (symbol_ID >= 0 && symbol_ID < symbol_assignments.size()) {
                        int symbol_assignment = (int)symbol_assignments[symbol_ID];
                        if (symbol_assignment >= 0) current_prop_as_symbol = props[symbol_assignment];
                    }
                }
                
                for (int cp = 0; cp < char_pos_arr.size(); cp++) {
                    godot::Ref<Prop> char_pos = char_pos_arr[cp];
                    if (!char_pos.is_valid()) continue;
                    
                    if (best_fit == FIT_LITERAL && role_pos == char_pos) { has_possession = true; break; }
                    if (best_fit == FIT_SYMBOLIC && current_prop_as_symbol == char_pos) { has_possession = true; break; }
                    if (best_fit == FIT_THOROUGH && (role_pos == char_pos || current_prop_as_symbol == char_pos)) { has_possession = true; break; }
                }
                if (!has_possession) score += 1;
            }
        }
        
        // Goals evaluation
        if (best_fit != FIT_BASIC) {
            godot::TypedArray<Plot> role_goal_arr = role_to_fill->get_goals();
            godot::TypedArray<Plot> char_goal_arr = character->get_goals();
            
            for (int g = 0; g < role_goal_arr.size(); g++) {
                godot::Ref<Plot> role_goal = role_goal_arr[g];
                if (!role_goal.is_valid()) continue;
                
                bool has_goal = false;
                godot::Ref<Plot> goal_story = nullptr;
                
                if (best_fit == FIT_SYMBOLIC || best_fit == FIT_THOROUGH) {
                    int story_ID = get_story_id(role_goal);
                    if (story_ID >= 0 && story_ID < story_assignments.size()) {
                        int story_assignment = (int)story_assignments[story_ID];
                        if (story_assignment >= 0) goal_story = plots[story_assignment];
                    }
                }
                
                for (int cg = 0; cg < char_goal_arr.size(); cg++) {
                    godot::Ref<Plot> char_goal = char_goal_arr[cg];
                    if (!char_goal.is_valid()) continue;
                    
                    if (best_fit == FIT_LITERAL && char_goal == role_goal) { has_goal = true; break; }
                    if (best_fit == FIT_SYMBOLIC && char_goal == goal_story) { has_goal = true; break; }
                    if (best_fit == FIT_THOROUGH && (char_goal == role_goal || char_goal == goal_story)) { has_goal = true; break; }
                }
                if (!has_goal) score += 1;
            }
        }
        
        if (score < best_score) {
            best_score = score;
            best_ID = c;
        }
    }
    
    return best_ID;
}

bool Narrative::fill_role(int role_ID, int character_ID, NarrativeReplacement replace_type) {
    if (role_ID < 0 || role_ID >= roles.size() || character_ID < 0 || character_ID >= characters.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Character> role = roles[role_ID];
    if (!role.is_valid()) return false;
    
    godot::Ref<Character> new_character = characters[character_ID];
    if (!new_character.is_valid()) return false;
    
    godot::Ref<Character> current_character = nullptr;
    if (role_ID < role_assignments.size() && (int)role_assignments[role_ID] >= 0) {
        current_character = characters[(int)role_assignments[role_ID]];
    }
    
    // Update relationships for all characters
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (!character.is_valid()) continue;
        
        godot::TypedArray<Relationship> char_rels = character->get_relationships();
        for (int r = 0; r < char_rels.size(); r++) {
            godot::Ref<Relationship> char_rel = char_rels[r];
            if (!char_rel.is_valid()) continue;
            
            if (replace_type == REPLACE_LITERAL) {
                if (current_character.is_valid() && char_rel->get_relation() == current_character) char_rel->set_relation(new_character);
            } else if (replace_type == REPLACE_SYMBOLIC) {
                if (char_rel->get_title() == role->get_official_name() || char_rel->get_relation() == role) char_rel->set_relation(new_character);
            } else { // THOROUGH
                if (current_character.is_valid() && char_rel->get_relation() == current_character) char_rel->set_relation(new_character);
                else if (char_rel->get_title() == role->get_official_name() || char_rel->get_relation() == role) char_rel->set_relation(new_character);
            }
        }
    }
    
    // Update conditions in all incidents
    // DOD NOTE: To access `subject` and `object`, we explicitly cast `Incident_Condition` to `Narrative_Condition`
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        // This is a dynamic method call lookup since `Incident_Condition` does not have `subject` properties natively.
        // We will cast to `godot::Object` to use `call` or `set`.
        godot::Array conds = incident->call("get_conditions");
        for (int j = 0; j < conds.size(); j++) {
            godot::Ref<godot::RefCounted> cond = conds[j];
            if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
            
            if (replace_type == REPLACE_LITERAL) {
                if (current_character.is_valid()) {
                    if (godot::Ref<Narreme>(cond->call("get_subject")) == current_character) cond->call("set_subject", new_character);
                    if (godot::Ref<Narreme>(cond->call("get_object")) == current_character) cond->call("set_object", new_character);
                }
            } else if (replace_type == REPLACE_SYMBOLIC) {
                if (cond->call("get_subject_is_symbolic") && godot::Ref<Narreme>(cond->call("get_subject")) == role) {
                    cond->call("set_subject", new_character);
                    cond->call("set_subject_is_symbolic", false);
                }
                if (cond->call("get_object_is_symbolic") && godot::Ref<Narreme>(cond->call("get_object"))->get_class_name_str() == "Character") {
                    cond->call("set_object", new_character);
                    cond->call("set_object_is_symbolic", false);
                }
            } else { // THOROUGH
                if (current_character.is_valid()) {
                    if (godot::Ref<Narreme>(cond->call("get_subject")) == current_character) cond->call("set_subject", new_character);
                    if (godot::Ref<Narreme>(cond->call("get_object")) == current_character) cond->call("set_object", new_character);
                } else {
                    if (cond->call("get_subject_is_symbolic") && godot::Ref<Narreme>(cond->call("get_subject")) == role) {
                        cond->call("set_subject", new_character);
                        cond->call("set_subject_is_symbolic", false);
                    }
                    if (cond->call("get_object_is_symbolic") && godot::Ref<Narreme>(cond->call("get_object"))->get_class_name_str() == "Character") {
                        cond->call("set_object", new_character);
                        cond->call("set_object_is_symbolic", false);
                    }
                }
            }
        }
    }
    
    if (current_character.is_valid()) {
        emit_signal("role_vacated", role, current_character);
    }
    
    // Expand assignments array if necessary, though it should be 1:1 with roles
    if (role_ID >= role_assignments.size()) role_assignments.resize(role_ID + 1);
    role_assignments[role_ID] = character_ID;
    
    emit_signal("role_filled", role, new_character);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Fill Role", _undo_before);
    return true;
}

bool Narrative::vacate_role(int role_ID) {
    if (role_ID < 0 || role_ID >= role_assignments.size() || (int)role_assignments[role_ID] < 0) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Character> role = roles[role_ID];
    if (!role.is_valid()) return false;
    
    godot::Ref<Character> current_character = characters[(int)role_assignments[role_ID]];
    if (!current_character.is_valid()) return false;
    
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (!character.is_valid()) continue;
        
        godot::TypedArray<Relationship> char_rels = character->get_relationships();
        for (int r = 0; r < char_rels.size(); r++) {
            godot::Ref<Relationship> char_rel = char_rels[r];
            if (!char_rel.is_valid()) continue;
            
            if (char_rel->get_relation() == current_character) char_rel->set_relation(nullptr);
            else if (char_rel->get_title() == role->get_official_name()) char_rel->set_relation(nullptr);
        }
    }
    
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        godot::Array conds = incident->call("get_conditions");
        for (int j = 0; j < conds.size(); j++) {
            godot::Ref<godot::RefCounted> cond = conds[j];
            if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
            
            if (godot::Ref<Narreme>(cond->call("get_subject")) == current_character) cond->call("set_subject", nullptr);
            if (godot::Ref<Narreme>(cond->call("get_object")) == current_character) cond->call("set_object", nullptr);
        }
    }
    
    // Set assignment to empty (-1)
    role_assignments[role_ID] = -1;
    
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Vacate Role", _undo_before);
    return true;
}

// ----------------------------------------------------------------------------
// PROP FUNCTIONS
// ----------------------------------------------------------------------------

godot::TypedArray<godot::String> Narrative::gather_prop_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < props.size(); i++) {
        godot::Ref<Prop> prop = props[i];
        if (prop.is_valid()) {
            names.append(prop->get_official_name());
        }
    }
    return names;
}

int Narrative::add_prop(const godot::Ref<Prop> &new_prop) {
    int id = get_prop_id(new_prop);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = props.size();
        props.append(new_prop);
        // new_prop->connect("path_changed", ...) omitted (stub)
        emit_signal("prop_added", new_prop);
        _record_roster_undo("Add Prop to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_prop_id(const godot::Ref<Prop> &prop) const {
    for (int i = 0; i < props.size(); i++) {
        godot::Ref<Prop> p = props[i];
        if (p == prop) return i;
    }
    return -1;
}

bool Narrative::has_prop(const godot::Ref<Prop> &prop) const {
    return get_prop_id(prop) >= 0;
}

bool Narrative::remove_prop(const godot::Ref<Prop> &prop) {
    int id = get_prop_id(prop);
    if (id >= 0) return remove_prop_at(id);
    return false;
}

bool Narrative::remove_prop_at(int prop_ID) {
    if (prop_ID < 0 || prop_ID >= props.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Prop> prop = props[prop_ID];
    godot::TypedArray<int> symbol_IDs = get_all_symbols_prop_is_filling(prop_ID);
    
    for (int i = 0; i < symbol_IDs.size(); i++) {
        vacate_symbol((int)symbol_IDs[i]);
    }
    
    props.remove_at(prop_ID);
    _shift_assignments_after_literal_removal(symbol_assignments, prop_ID);
    emit_signal("prop_removed", prop);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Prop from Narrative", _undo_before);
    return true;
}

godot::TypedArray<int> Narrative::get_all_symbols_prop_is_filling(int prop_ID) const {
    godot::TypedArray<int> symbol_IDs;
    for (int i = 0; i < symbol_assignments.size(); i++) {
        if ((int)symbol_assignments[i] == prop_ID) {
            symbol_IDs.append(i);
        }
    }
    return symbol_IDs;
}


// ----------------------------------------------------------------------------
// SYMBOL FUNCTIONS
// ----------------------------------------------------------------------------

void Narrative::gather_symbols() {
    // Symbols are populated by the Resource inspector
}

godot::TypedArray<godot::String> Narrative::gather_symbol_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < symbols.size(); i++) {
        godot::Ref<Prop> symbol = symbols[i];
        if (symbol.is_valid()) {
            names.append(symbol->get_official_name());
        }
    }
    return names;
}

int Narrative::add_symbol(const godot::Ref<Prop> &new_symbol) {
    int id = get_symbol_id(new_symbol);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = symbols.size();
        symbols.append(new_symbol);
        symbol_assignments.append(-1);
        emit_signal("symbol_added", new_symbol);
        _record_roster_undo("Add Symbol to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_symbol_id(const godot::Ref<Prop> &symbol) const {
    for (int i = 0; i < symbols.size(); i++) {
        godot::Ref<Prop> r = symbols[i];
        if (r == symbol) return i;
    }
    return -1;
}

bool Narrative::has_symbol(const godot::Ref<Prop> &symbol) const {
    return get_symbol_id(symbol) >= 0;
}

bool Narrative::remove_symbol(const godot::Ref<Prop> &symbol) {
    return remove_symbol_at(get_symbol_id(symbol));
}

bool Narrative::remove_symbol_at(int symbol_ID) {
    if (symbol_ID < 0 || symbol_ID >= symbols.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Prop> symbol = symbols[symbol_ID];
    
    if (symbol_ID < symbol_assignments.size() && (int)symbol_assignments[symbol_ID] >= 0) {
        vacate_symbol(symbol_ID);
    }
    
    symbols.remove_at(symbol_ID);
    symbol_assignments.remove_at(symbol_ID);
    
    emit_signal("symbol_removed", symbol);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Symbol from Narrative", _undo_before);
    return true;
}

int Narrative::get_best_fit_prop_for_symbol(int symbol_ID, const godot::TypedArray<int> &prop_mask, BestNarrativeFit best_fit) {
    if (symbol_ID < 0 || symbol_ID >= symbols.size()) return -1;
    
    int best_score = 100000;
    int best_ID = -1;
    
    godot::Ref<Prop> symbol_to_fill = symbols[symbol_ID];
    if (!symbol_to_fill.is_valid()) return -1;
    
    godot::Ref<Prop> current_prop_in_symbol = nullptr;
    if (symbol_ID < symbol_assignments.size() && (int)symbol_assignments[symbol_ID] >= 0) {
        current_prop_in_symbol = props[(int)symbol_assignments[symbol_ID]];
    }
    
    for (int c = 0; c < props.size(); c++) {
        godot::Ref<Prop> prop = props[c];
        if (!prop.is_valid()) continue;
        
        bool is_masked = false;
        for (int m = 0; m < prop_mask.size(); m++) {
            if ((int)prop_mask[m] == c) {
                is_masked = true;
                break;
            }
        }
        if (is_masked) continue;
        
        int score = prop->score_similarity(symbol_to_fill.ptr());
        
        if (best_fit == FIT_LITERAL) {
            if (symbol_to_fill->get_current_location() != prop->get_current_location()) score += 1;
            if (symbol_to_fill->get_possessor() != prop->get_possessor()) score += 1;
        } 
        else if (best_fit == FIT_SYMBOLIC) {
            godot::Ref<Location> symbol_loc = symbol_to_fill->get_current_location();
            if (symbol_loc.is_valid()) {
                int place_ID = get_place_id(symbol_loc);
                if (place_ID >= 0 && place_ID < place_assignments.size()) {
                    int place_assignment = (int)place_assignments[place_ID];
                    if (place_assignment >= 0) {
                        godot::Ref<Location> loc = locations[place_assignment];
                        if (loc != prop->get_current_location()) score += 1;
                    }
                }
            } else if (prop->get_current_location().is_valid()) {
                score += 1;
            }
            
            godot::Ref<Character> symbol_pos = symbol_to_fill->get_possessor();
            if (symbol_pos.is_valid()) {
                int role_ID = get_role_id(symbol_pos);
                if (role_ID >= 0 && role_ID < role_assignments.size()) {
                    int role_assignment = (int)role_assignments[role_ID];
                    if (role_assignment >= 0) {
                        godot::Ref<Character> role_char = characters[role_assignment];
                        if (role_char != prop->get_possessor()) score += 1;
                    }
                }
            }
        }
        else if (best_fit == FIT_THOROUGH) {
            if (symbol_to_fill->get_current_location() != prop->get_current_location()) {
                int place_ID = get_place_id(symbol_to_fill->get_current_location());
                if (place_ID >= 0 && place_ID < place_assignments.size()) {
                    int place_assignment = (int)place_assignments[place_ID];
                    if (place_assignment >= 0) {
                        godot::Ref<Location> loc = locations[place_assignment];
                        if (loc != prop->get_current_location()) score += 1;
                    }
                }
            }
            
            if (symbol_to_fill->get_possessor() != prop->get_possessor()) {
                int role_ID = get_role_id(symbol_to_fill->get_possessor());
                if (role_ID >= 0 && role_ID < role_assignments.size()) {
                    int role_assignment = (int)role_assignments[role_ID];
                    if (role_assignment >= 0) {
                        godot::Ref<Character> role_char = characters[role_assignment];
                        if (role_char != prop->get_possessor()) score += 1;
                    }
                }
            }
        }
        
        if (score < best_score) {
            best_score = score;
            best_ID = c;
        }
    }
    
    return best_ID;
}

bool Narrative::fill_symbol(int symbol_ID, int prop_ID, NarrativeReplacement replace_type) {
    if (symbol_ID < 0 || symbol_ID >= symbols.size() || prop_ID < 0 || prop_ID >= props.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Prop> symbol = symbols[symbol_ID];
    if (!symbol.is_valid()) return false;
    
    godot::Ref<Prop> new_prop = props[prop_ID];
    if (!new_prop.is_valid()) return false;
    
    godot::Ref<Prop> current_prop = nullptr;
    if (symbol_ID < symbol_assignments.size() && (int)symbol_assignments[symbol_ID] >= 0) {
        current_prop = props[(int)symbol_assignments[symbol_ID]];
    }
    
    if (replace_type == REPLACE_LITERAL || replace_type == REPLACE_THOROUGH) {
        if (current_prop.is_valid()) {
            for (int i = 0; i < characters.size(); i++) {
                godot::Ref<Character> character = characters[i];
                if (!character.is_valid()) continue;
                
                godot::TypedArray<Relationship> char_rels = character->get_relationships();
                for (int r = 0; r < char_rels.size(); r++) {
                    godot::Ref<Relationship> char_rel = char_rels[r];
                    if (char_rel.is_valid() && char_rel->get_relation() == current_prop) {
                        char_rel->set_relation(new_prop);
                    }
                }
            }
            
            for (int i = 0; i < incidents.size(); i++) {
                godot::Ref<Incident> incident = incidents[i];
                if (!incident.is_valid()) continue;
                
                godot::Array conds = incident->call("get_conditions");
                for (int j = 0; j < conds.size(); j++) {
                    godot::Ref<godot::RefCounted> cond = conds[j];
                    if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                    
                    if (godot::Ref<Narreme>(cond->call("get_subject")) == current_prop) cond->call("set_subject", new_prop);
                    if (godot::Ref<Narreme>(cond->call("get_object")) == current_prop) cond->call("set_object", new_prop);
                }
            }
        }
    }
    
    if (replace_type == REPLACE_SYMBOLIC || replace_type == REPLACE_THOROUGH) {
        for (int i = 0; i < characters.size(); i++) {
            godot::Ref<Character> character = characters[i];
            if (!character.is_valid()) continue;
            
            godot::TypedArray<Relationship> char_rels = character->get_relationships();
            for (int r = 0; r < char_rels.size(); r++) {
                godot::Ref<Relationship> char_rel = char_rels[r];
                if (!char_rel.is_valid()) continue;
                
                if (char_rel->get_title() == symbol->get_official_name() || char_rel->get_relation() == symbol) {
                    char_rel->set_relation(new_prop);
                }
            }
        }
        
        for (int i = 0; i < incidents.size(); i++) {
            godot::Ref<Incident> incident = incidents[i];
            if (!incident.is_valid()) continue;
            
            godot::Array conds = incident->call("get_conditions");
            for (int j = 0; j < conds.size(); j++) {
                godot::Ref<godot::RefCounted> cond = conds[j];
                if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                
                if (cond->call("get_subject_is_symbolic") && godot::Ref<Narreme>(cond->call("get_subject")) == symbol) {
                    cond->call("set_subject", new_prop);
                    cond->call("set_subject_is_symbolic", false);
                }
                
                godot::Ref<Narreme> obj_narreme = cond->call("get_object");
                if (cond->call("get_object_is_symbolic") && obj_narreme.is_valid() && obj_narreme->get_class_name_str() == "Prop") {
                    cond->call("set_object", new_prop);
                    cond->call("set_object_is_symbolic", false);
                }
            }
        }
    }
    
    if (current_prop.is_valid()) {
        emit_signal("symbol_vacated", symbol, current_prop);
    }
    
    if (symbol_ID >= symbol_assignments.size()) symbol_assignments.resize(symbol_ID + 1);
    symbol_assignments[symbol_ID] = prop_ID;
    
    emit_signal("symbol_filled", symbol, new_prop);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Fill Symbol", _undo_before);
    return true;
}

bool Narrative::vacate_symbol(int symbol_ID) {
    if (symbol_ID < 0 || symbol_ID >= symbol_assignments.size() || (int)symbol_assignments[symbol_ID] < 0) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Prop> symbol = symbols[symbol_ID];
    if (!symbol.is_valid()) return false;
    
    godot::Ref<Prop> current_prop = props[(int)symbol_assignments[symbol_ID]];
    if (!current_prop.is_valid()) return false;
    
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (!character.is_valid()) continue;
        
        godot::TypedArray<Relationship> char_rels = character->get_relationships();
        for (int r = 0; r < char_rels.size(); r++) {
            godot::Ref<Relationship> char_rel = char_rels[r];
            if (!char_rel.is_valid()) continue;
            
            if (char_rel->get_relation() == current_prop) char_rel->set_relation(nullptr);
            else if (char_rel->get_title() == symbol->get_official_name()) char_rel->set_relation(nullptr);
        }
    }
    
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        godot::Array conds = incident->call("get_conditions");
        for (int j = 0; j < conds.size(); j++) {
            godot::Ref<godot::RefCounted> cond = conds[j];
            if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
            
            if (godot::Ref<Narreme>(cond->call("get_subject")) == current_prop) cond->call("set_subject", nullptr);
            if (godot::Ref<Narreme>(cond->call("get_object")) == current_prop) cond->call("set_object", nullptr);
        }
    }
    
    symbol_assignments[symbol_ID] = -1;
    
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Vacate Symbol", _undo_before);
    return true;
}

// ----------------------------------------------------------------------------
// LOCATION FUNCTIONS
// ----------------------------------------------------------------------------

godot::TypedArray<godot::String> Narrative::gather_location_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < locations.size(); i++) {
        godot::Ref<Location> location = locations[i];
        if (location.is_valid()) {
            names.append(location->get_official_name());
        }
    }
    return names;
}

int Narrative::add_location(const godot::Ref<Location> &new_location) {
    int id = get_location_id(new_location);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = locations.size();
        locations.append(new_location);
        emit_signal("location_added", new_location);
        _record_roster_undo("Add Location to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_location_id(const godot::Ref<Location> &location) const {
    for (int i = 0; i < locations.size(); i++) {
        godot::Ref<Location> l = locations[i];
        if (l == location) return i;
    }
    return -1;
}

bool Narrative::has_location(const godot::Ref<Location> &location) const {
    return get_location_id(location) >= 0;
}

bool Narrative::remove_location(const godot::Ref<Location> &location) {
    int id = get_location_id(location);
    if (id >= 0) return remove_location_at(id);
    return false;
}

bool Narrative::remove_location_at(int location_ID) {
    if (location_ID < 0 || location_ID >= locations.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Location> location = locations[location_ID];
    godot::TypedArray<int> place_IDs = get_all_places_location_is_filling(location_ID);
    
    for (int i = 0; i < place_IDs.size(); i++) {
        vacate_place((int)place_IDs[i]);
    }
    
    locations.remove_at(location_ID);
    _shift_assignments_after_literal_removal(place_assignments, location_ID);
    emit_signal("location_removed", location);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Location from Narrative", _undo_before);
    return true;
}

godot::TypedArray<int> Narrative::get_all_places_location_is_filling(int location_ID) const {
    godot::TypedArray<int> place_IDs;
    for (int i = 0; i < place_assignments.size(); i++) {
        if ((int)place_assignments[i] == location_ID) {
            place_IDs.append(i);
        }
    }
    return place_IDs;
}


// ----------------------------------------------------------------------------
// PLACE FUNCTIONS
// ----------------------------------------------------------------------------

void Narrative::gather_places() {
    // Places are populated by the Resource inspector
}

godot::TypedArray<godot::String> Narrative::gather_place_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < places.size(); i++) {
        godot::Ref<Location> place = places[i];
        if (place.is_valid()) {
            names.append(place->get_official_name());
        }
    }
    return names;
}

int Narrative::add_place(const godot::Ref<Location> &new_place) {
    int id = get_place_id(new_place);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = places.size();
        places.append(new_place);
        place_assignments.append(-1);
        emit_signal("place_added", new_place);
        _record_roster_undo("Add Place to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_place_id(const godot::Ref<Location> &place) const {
    for (int i = 0; i < places.size(); i++) {
        godot::Ref<Location> p = places[i];
        if (p == place) return i;
    }
    return -1;
}

bool Narrative::has_place(const godot::Ref<Location> &place) const {
    return get_place_id(place) >= 0;
}

bool Narrative::remove_place(const godot::Ref<Location> &place) {
    return remove_place_at(get_place_id(place));
}

bool Narrative::remove_place_at(int place_ID) {
    if (place_ID < 0 || place_ID >= places.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Location> place = places[place_ID];
    
    if (place_ID < place_assignments.size() && (int)place_assignments[place_ID] >= 0) {
        vacate_place(place_ID);
    }
    
    places.remove_at(place_ID);
    place_assignments.remove_at(place_ID);
    
    emit_signal("place_removed", place);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Place from Narrative", _undo_before);
    return true;
}

int Narrative::get_best_fit_location_for_place(int place_ID, const godot::TypedArray<int> &location_mask, BestNarrativeFit best_fit) {
    if (place_ID < 0 || place_ID >= places.size()) return -1;
    
    int best_score = 100000;
    int best_ID = -1;
    
    godot::Ref<Location> place_to_fill = places[place_ID];
    if (!place_to_fill.is_valid()) return -1;
    
    for (int l = 0; l < locations.size(); l++) {
        godot::Ref<Location> location = locations[l];
        if (!location.is_valid()) continue;
        
        bool is_masked = false;
        for (int m = 0; m < location_mask.size(); m++) {
            if ((int)location_mask[m] == l) {
                is_masked = true;
                break;
            }
        }
        if (is_masked) continue;
        
        int score = location->score_similarity(place_to_fill.ptr());
        
        // Locations primarily score based on missing explicit Relationships, 
        // unlike Characters/Props which have active goals and possessors.
        if (best_fit != FIT_BASIC) {
            // Note: Currently omitted deep relationship scan for Locations as their footprint 
            // is largely handled by Characters and Props entering them. 
            // (Placeholder block for symmetric expansion if Locations gain complex relation arrays).
        }
        
        if (score < best_score) {
            best_score = score;
            best_ID = l;
        }
    }
    
    return best_ID;
}

bool Narrative::fill_place(int place_ID, int location_ID, NarrativeReplacement replace_type) {
    if (place_ID < 0 || place_ID >= places.size() || location_ID < 0 || location_ID >= locations.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Location> place = places[place_ID];
    if (!place.is_valid()) return false;
    
    godot::Ref<Location> new_location = locations[location_ID];
    if (!new_location.is_valid()) return false;
    
    godot::Ref<Location> current_location = nullptr;
    if (place_ID < place_assignments.size() && (int)place_assignments[place_ID] >= 0) {
        current_location = locations[(int)place_assignments[place_ID]];
    }
    
    if (replace_type == REPLACE_LITERAL || replace_type == REPLACE_THOROUGH) {
        if (current_location.is_valid()) {
            
            // Sweep Characters
            for (int i = 0; i < characters.size(); i++) {
                godot::Ref<Character> character = characters[i];
                if (!character.is_valid()) continue;
                
                if (character->get_current_location() == current_location) {
                    character->enter_location(new_location);
                }
                
                godot::TypedArray<Relationship> char_rels = character->get_relationships();
                for (int r = 0; r < char_rels.size(); r++) {
                    godot::Ref<Relationship> char_rel = char_rels[r];
                    if (char_rel.is_valid() && char_rel->get_relation() == current_location) {
                        char_rel->set_relation(new_location);
                    }
                }
            }
            
            // Sweep Props
            for (int i = 0; i < props.size(); i++) {
                godot::Ref<Prop> prop = props[i];
                if (!prop.is_valid()) continue;
                
                if (prop->get_current_location() == current_location) {
                    prop->enter_location(new_location);
                }
            }
            
            // Sweep Incidents
            for (int i = 0; i < incidents.size(); i++) {
                godot::Ref<Incident> incident = incidents[i];
                if (!incident.is_valid()) continue;
                
                godot::Array conds = incident->call("get_conditions");
                for (int j = 0; j < conds.size(); j++) {
                    godot::Ref<godot::RefCounted> cond = conds[j];
                    if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                    
                    if (godot::Ref<Narreme>(cond->call("get_subject")) == current_location) cond->call("set_subject", new_location);
                    if (godot::Ref<Narreme>(cond->call("get_object")) == current_location) cond->call("set_object", new_location);
                }
            }
        }
    }
    
    if (replace_type == REPLACE_SYMBOLIC || replace_type == REPLACE_THOROUGH) {
        // Sweep Characters for symbolic relationships
        for (int i = 0; i < characters.size(); i++) {
            godot::Ref<Character> character = characters[i];
            if (!character.is_valid()) continue;
            
            godot::TypedArray<Relationship> char_rels = character->get_relationships();
            for (int r = 0; r < char_rels.size(); r++) {
                godot::Ref<Relationship> char_rel = char_rels[r];
                if (!char_rel.is_valid()) continue;
                
                if (char_rel->get_title() == place->get_official_name() || char_rel->get_relation() == place) {
                    char_rel->set_relation(new_location);
                }
            }
        }
        
        // Sweep Incidents for symbolic conditions
        for (int i = 0; i < incidents.size(); i++) {
            godot::Ref<Incident> incident = incidents[i];
            if (!incident.is_valid()) continue;
            
            godot::Array conds = incident->call("get_conditions");
            for (int j = 0; j < conds.size(); j++) {
                godot::Ref<godot::RefCounted> cond = conds[j];
                if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                
                if (cond->call("get_subject_is_symbolic") && godot::Ref<Narreme>(cond->call("get_subject")) == place) {
                    cond->call("set_subject", new_location);
                    cond->call("set_subject_is_symbolic", false);
                }
                
                godot::Ref<Narreme> obj_narreme = cond->call("get_object");
                if (cond->call("get_object_is_symbolic") && obj_narreme.is_valid() && obj_narreme->get_class_name_str() == "Location") {
                    cond->call("set_object", new_location);
                    cond->call("set_object_is_symbolic", false);
                }
            }
        }
    }
    
    if (current_location.is_valid()) {
        emit_signal("place_vacated", place, current_location);
    }
    
    if (place_ID >= place_assignments.size()) place_assignments.resize(place_ID + 1);
    place_assignments[place_ID] = location_ID;
    
    emit_signal("place_filled", place, new_location);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Fill Place", _undo_before);
    return true;
}

bool Narrative::vacate_place(int place_ID) {
    if (place_ID < 0 || place_ID >= place_assignments.size() || (int)place_assignments[place_ID] < 0) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Location> place = places[place_ID];
    if (!place.is_valid()) return false;
    
    godot::Ref<Location> current_location = locations[(int)place_assignments[place_ID]];
    if (!current_location.is_valid()) return false;
    
    // Sweep Characters
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (!character.is_valid()) continue;
        
        if (character->get_current_location() == current_location) {
            character->leave_current_location();
        }
        
        godot::TypedArray<Relationship> char_rels = character->get_relationships();
        for (int r = 0; r < char_rels.size(); r++) {
            godot::Ref<Relationship> char_rel = char_rels[r];
            if (!char_rel.is_valid()) continue;
            
            if (char_rel->get_relation() == current_location) char_rel->set_relation(nullptr);
            else if (char_rel->get_title() == place->get_official_name()) char_rel->set_relation(nullptr);
        }
    }
    
    // Sweep Props
    for (int i = 0; i < props.size(); i++) {
        godot::Ref<Prop> prop = props[i];
        if (!prop.is_valid()) continue;
        
        if (prop->get_current_location() == current_location) {
            prop->leave_current_location();
        }
    }
    
    // Sweep Incidents
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        godot::Array conds = incident->call("get_conditions");
        for (int j = 0; j < conds.size(); j++) {
            godot::Ref<godot::RefCounted> cond = conds[j];
            if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
            
            if (godot::Ref<Narreme>(cond->call("get_subject")) == current_location) cond->call("set_subject", nullptr);
            if (godot::Ref<Narreme>(cond->call("get_object")) == current_location) cond->call("set_object", nullptr);
        }
    }
    
    place_assignments[place_ID] = -1;
    
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Vacate Place", _undo_before);
    return true;
}

// ----------------------------------------------------------------------------
// INCIDENT FUNCTIONS
// ----------------------------------------------------------------------------

godot::TypedArray<godot::String> Narrative::gather_incident_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (incident.is_valid()) {
            names.append(incident->get_official_name());
        }
    }
    return names;
}

int Narrative::add_incident(const godot::Ref<Incident> &new_incident) {
    int id = get_incident_id(new_incident);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = incidents.size();
        incidents.append(new_incident);
        emit_signal("incident_added", new_incident);
        _record_roster_undo("Add Incident to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_incident_id(const godot::Ref<Incident> &incident) const {
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> inc = incidents[i];
        if (inc == incident) return i;
    }
    return -1;
}

bool Narrative::has_incident(const godot::Ref<Incident> &incident) const {
    return get_incident_id(incident) >= 0;
}

bool Narrative::remove_incident(const godot::Ref<Incident> &incident) {
    int id = get_incident_id(incident);
    if (id >= 0) return remove_incident_at(id);
    return false;
}

bool Narrative::remove_incident_at(int incident_ID) {
    if (incident_ID < 0 || incident_ID >= incidents.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Incident> incident = incidents[incident_ID];
    godot::TypedArray<int> moment_IDs = get_all_moments_incident_is_filling(incident_ID);
    
    for (int i = 0; i < moment_IDs.size(); i++) {
        vacate_moment((int)moment_IDs[i]);
    }
    
    incidents.remove_at(incident_ID);
    _shift_assignments_after_literal_removal(moment_assignments, incident_ID);
    emit_signal("incident_removed", incident);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Incident from Narrative", _undo_before);
    return true;
}

godot::TypedArray<int> Narrative::get_all_moments_incident_is_filling(int incident_ID) const {
    godot::TypedArray<int> moment_IDs;
    for (int i = 0; i < moment_assignments.size(); i++) {
        if ((int)moment_assignments[i] == incident_ID) {
            moment_IDs.append(i);
        }
    }
    return moment_IDs;
}


// ----------------------------------------------------------------------------
// MOMENT FUNCTIONS
// ----------------------------------------------------------------------------

void Narrative::gather_moments() {
    // Moments are populated by the Resource inspector
}

godot::TypedArray<godot::String> Narrative::gather_moment_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < moments.size(); i++) {
        godot::Ref<Incident> moment = moments[i];
        if (moment.is_valid()) {
            names.append(moment->get_official_name());
        }
    }
    return names;
}

int Narrative::add_moment(const godot::Ref<Incident> &new_moment) {
    int id = get_moment_id(new_moment);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = moments.size();
        moments.append(new_moment);
        moment_assignments.append(-1);
        emit_signal("moment_added", new_moment);
        _record_roster_undo("Add Moment to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_moment_id(const godot::Ref<Incident> &moment) const {
    for (int i = 0; i < moments.size(); i++) {
        godot::Ref<Incident> m = moments[i];
        if (m == moment) return i;
    }
    return -1;
}

bool Narrative::has_moment(const godot::Ref<Incident> &moment) const {
    return get_moment_id(moment) >= 0;
}

bool Narrative::remove_moment(const godot::Ref<Incident> &moment) {
    return remove_moment_at(get_moment_id(moment));
}

bool Narrative::remove_moment_at(int moment_ID) {
    if (moment_ID < 0 || moment_ID >= moments.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Incident> moment = moments[moment_ID];
    
    if (moment_ID < moment_assignments.size() && (int)moment_assignments[moment_ID] >= 0) {
        vacate_moment(moment_ID);
    }
    
    moments.remove_at(moment_ID);
    moment_assignments.remove_at(moment_ID);
    
    emit_signal("moment_removed", moment);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Moment from Narrative", _undo_before);
    return true;
}

int Narrative::get_best_fit_incident_for_moment(int moment_ID, const godot::TypedArray<int> &incident_mask, BestNarrativeFit best_fit) {
    if (moment_ID < 0 || moment_ID >= moments.size()) return -1;
    
    int best_score = 100000;
    int best_ID = -1;
    
    godot::Ref<Incident> moment_to_fill = moments[moment_ID];
    if (!moment_to_fill.is_valid()) return -1;
    
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        bool is_masked = false;
        for (int m = 0; m < incident_mask.size(); m++) {
            if ((int)incident_mask[m] == i) {
                is_masked = true;
                break;
            }
        }
        if (is_masked) continue;
        
        int score = incident->score_similarity(moment_to_fill.ptr());
        
        // Incident matching logic primarily focuses on sub-conditions. 
        // We evaluate condition arrays dynamically via Godot's `call` to avoid rigid class casting.
        if (best_fit != FIT_BASIC) {
            godot::Array moment_conds = moment_to_fill->call("get_conditions");
            godot::Array incident_conds = incident->call("get_conditions");
            
            for (int mc = 0; mc < moment_conds.size(); mc++) {
                godot::Ref<godot::RefCounted> m_cond = moment_conds[mc];
                if (!m_cond.is_valid() || m_cond->get_class() != "Narrative_Condition") continue;
                
                bool has_match = false;
                for (int ic = 0; ic < incident_conds.size(); ic++) {
                    godot::Ref<godot::RefCounted> i_cond = incident_conds[ic];
                    if (!i_cond.is_valid() || i_cond->get_class() != "Narrative_Condition") continue;
                    
                    // Basic structural condition match
                    if ((int)m_cond->call("get_condition") == (int)i_cond->call("get_condition")) {
                        has_match = true;
                        break;
                    }
                }
                if (!has_match) score += 1;
            }
        }
        
        if (score < best_score) {
            best_score = score;
            best_ID = i;
        }
    }
    
    return best_ID;
}

bool Narrative::fill_moment(int moment_ID, int incident_ID, NarrativeReplacement replace_type) {
    if (moment_ID < 0 || moment_ID >= moments.size() || incident_ID < 0 || incident_ID >= incidents.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Incident> moment = moments[moment_ID];
    if (!moment.is_valid()) return false;
    
    godot::Ref<Incident> new_incident = incidents[incident_ID];
    if (!new_incident.is_valid()) return false;
    
    godot::Ref<Incident> current_incident = nullptr;
    if (moment_ID < moment_assignments.size() && (int)moment_assignments[moment_ID] >= 0) {
        current_incident = incidents[(int)moment_assignments[moment_ID]];
    }
    
    if (replace_type == REPLACE_LITERAL || replace_type == REPLACE_THOROUGH) {
        if (current_incident.is_valid()) {
            
            // Sweep Characters
            for (int i = 0; i < characters.size(); i++) {
                godot::Ref<Character> character = characters[i];
                if (!character.is_valid()) continue;
                
                godot::TypedArray<Relationship> char_rels = character->get_relationships();
                for (int r = 0; r < char_rels.size(); r++) {
                    godot::Ref<Relationship> char_rel = char_rels[r];
                    if (char_rel.is_valid() && char_rel->get_relation() == current_incident) {
                        char_rel->set_relation(new_incident);
                    }
                }
            }
            
            // Sweep Incidents
            for (int i = 0; i < incidents.size(); i++) {
                godot::Ref<Incident> incident = incidents[i];
                if (!incident.is_valid()) continue;
                
                godot::Array conds = incident->call("get_conditions");
                for (int j = 0; j < conds.size(); j++) {
                    godot::Ref<godot::RefCounted> cond = conds[j];
                    if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                    
                    if (godot::Ref<Narreme>(cond->call("get_subject")) == current_incident) cond->call("set_subject", new_incident);
                    if (godot::Ref<Narreme>(cond->call("get_object")) == current_incident) cond->call("set_object", new_incident);
                }
            }

            // Sweep Plots -> Plot Events
            for (int i = 0; i < plots.size(); i++) {
                godot::Ref<Plot> plot = plots[i];
                if (!plot.is_valid()) continue;

                godot::Array plot_events = plot->call("get_plot_events");
                for (int pe = 0; pe < plot_events.size(); pe++) {
                    godot::Ref<godot::RefCounted> plot_event = plot_events[pe];
                    if (plot_event.is_valid() && godot::Ref<Incident>(plot_event->call("get_incident")) == current_incident) {
                        plot_event->call("set_incident", new_incident);
                    }
                }
            }
        }
    }
    
    if (replace_type == REPLACE_SYMBOLIC || replace_type == REPLACE_THOROUGH) {
        
        // Sweep Characters
        for (int i = 0; i < characters.size(); i++) {
            godot::Ref<Character> character = characters[i];
            if (!character.is_valid()) continue;
            
            godot::TypedArray<Relationship> char_rels = character->get_relationships();
            for (int r = 0; r < char_rels.size(); r++) {
                godot::Ref<Relationship> char_rel = char_rels[r];
                if (!char_rel.is_valid()) continue;
                
                if (char_rel->get_title() == moment->get_official_name() || char_rel->get_relation() == moment) {
                    char_rel->set_relation(new_incident);
                }
            }
        }
        
        // Sweep Incidents
        for (int i = 0; i < incidents.size(); i++) {
            godot::Ref<Incident> incident = incidents[i];
            if (!incident.is_valid()) continue;
            
            godot::Array conds = incident->call("get_conditions");
            for (int j = 0; j < conds.size(); j++) {
                godot::Ref<godot::RefCounted> cond = conds[j];
                if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                
                if (cond->call("get_subject_is_symbolic") && godot::Ref<Narreme>(cond->call("get_subject")) == moment) {
                    cond->call("set_subject", new_incident);
                    cond->call("set_subject_is_symbolic", false);
                }
                
                godot::Ref<Narreme> obj_narreme = cond->call("get_object");
                if (cond->call("get_object_is_symbolic") && obj_narreme.is_valid() && obj_narreme->get_class_name_str() == "Incident") {
                    cond->call("set_object", new_incident);
                    cond->call("set_object_is_symbolic", false);
                }
            }
        }

        // Sweep Plots -> Plot Events
        for (int i = 0; i < plots.size(); i++) {
            godot::Ref<Plot> plot = plots[i];
            if (!plot.is_valid()) continue;

            godot::Array plot_events = plot->call("get_plot_events");
            for (int pe = 0; pe < plot_events.size(); pe++) {
                godot::Ref<godot::RefCounted> plot_event = plot_events[pe];
                if (!plot_event.is_valid()) continue;

                godot::Ref<Incident> pe_incident = plot_event->call("get_incident");
                godot::String pe_title = plot_event->call("get_title");

                if (pe_incident == moment || pe_title == moment->get_official_name()) {
                    plot_event->call("set_incident", new_incident);
                }
            }
        }
    }
    
    if (current_incident.is_valid()) {
        emit_signal("moment_vacated", moment, current_incident);
    }
    
    if (moment_ID >= moment_assignments.size()) moment_assignments.resize(moment_ID + 1);
    moment_assignments[moment_ID] = incident_ID;
    
    emit_signal("moment_filled", moment, new_incident);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Fill Moment", _undo_before);
    return true;
}

bool Narrative::vacate_moment(int moment_ID) {
    if (moment_ID < 0 || moment_ID >= moment_assignments.size() || (int)moment_assignments[moment_ID] < 0) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Incident> moment = moments[moment_ID];
    if (!moment.is_valid()) return false;
    
    godot::Ref<Incident> current_incident = incidents[(int)moment_assignments[moment_ID]];
    if (!current_incident.is_valid()) return false;
    
    // Sweep Characters
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (!character.is_valid()) continue;
        
        godot::TypedArray<Relationship> char_rels = character->get_relationships();
        for (int r = 0; r < char_rels.size(); r++) {
            godot::Ref<Relationship> char_rel = char_rels[r];
            if (!char_rel.is_valid()) continue;
            
            if (char_rel->get_relation() == current_incident) char_rel->set_relation(nullptr);
            else if (char_rel->get_title() == moment->get_official_name()) char_rel->set_relation(nullptr);
        }
    }
    
    // Sweep Incidents
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        godot::Array conds = incident->call("get_conditions");
        for (int j = 0; j < conds.size(); j++) {
            godot::Ref<godot::RefCounted> cond = conds[j];
            if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
            
            if (godot::Ref<Narreme>(cond->call("get_subject")) == current_incident) cond->call("set_subject", nullptr);
            if (godot::Ref<Narreme>(cond->call("get_object")) == current_incident) cond->call("set_object", nullptr);
        }
    }

    // Sweep Plots -> Plot Events
    for (int i = 0; i < plots.size(); i++) {
        godot::Ref<Plot> plot = plots[i];
        if (!plot.is_valid()) continue;

        godot::Array plot_events = plot->call("get_plot_events");
        for (int pe = 0; pe < plot_events.size(); pe++) {
            godot::Ref<godot::RefCounted> plot_event = plot_events[pe];
            if (!plot_event.is_valid()) continue;

            if (godot::Ref<Incident>(plot_event->call("get_incident")) == current_incident) {
                plot_event->call("set_incident", nullptr);
            } else if ((godot::String)plot_event->call("get_title") == moment->get_official_name()) {
                plot_event->call("set_incident", nullptr);
            }
        }
    }
    
    moment_assignments[moment_ID] = -1;
    
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Vacate Moment", _undo_before);
    return true;
}

// ----------------------------------------------------------------------------
// PLOT FUNCTIONS
// ----------------------------------------------------------------------------

godot::TypedArray<godot::String> Narrative::gather_plot_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < plots.size(); i++) {
        godot::Ref<Plot> plot = plots[i];
        if (plot.is_valid()) {
            names.append(plot->get_official_name());
        }
    }
    return names;
}

int Narrative::add_plot(const godot::Ref<Plot> &new_plot) {
    int id = get_plot_id(new_plot);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = plots.size();
        plots.append(new_plot);
        emit_signal("plot_added", new_plot);
        _record_roster_undo("Add Plot to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_plot_id(const godot::Ref<Plot> &plot) const {
    for (int i = 0; i < plots.size(); i++) {
        godot::Ref<Plot> p = plots[i];
        if (p == plot) return i;
    }
    return -1;
}

bool Narrative::has_plot(const godot::Ref<Plot> &plot) const {
    return get_plot_id(plot) >= 0;
}

bool Narrative::remove_plot(const godot::Ref<Plot> &plot) {
    int id = get_plot_id(plot);
    if (id >= 0) return remove_plot_at(id);
    return false;
}

bool Narrative::remove_plot_at(int plot_ID) {
    if (plot_ID < 0 || plot_ID >= plots.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Plot> plot = plots[plot_ID];
    godot::TypedArray<int> story_IDs = get_all_storys_plot_is_filling(plot_ID);
    
    for (int i = 0; i < story_IDs.size(); i++) {
        vacate_story((int)story_IDs[i]);
    }
    
    plots.remove_at(plot_ID);
    _shift_assignments_after_literal_removal(story_assignments, plot_ID);
    emit_signal("plot_removed", plot);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Plot from Narrative", _undo_before);
    return true;
}

godot::TypedArray<int> Narrative::get_all_storys_plot_is_filling(int plot_ID) const {
    godot::TypedArray<int> story_IDs;
    for (int i = 0; i < story_assignments.size(); i++) {
        if ((int)story_assignments[i] == plot_ID) {
            story_IDs.append(i);
        }
    }
    return story_IDs;
}


// ----------------------------------------------------------------------------
// STORY FUNCTIONS
// ----------------------------------------------------------------------------

void Narrative::gather_stories() {
    // Stories are populated by the Resource inspector
}

godot::TypedArray<godot::String> Narrative::gather_story_names() const {
    godot::TypedArray<godot::String> names;
    for (int i = 0; i < stories.size(); i++) {
        godot::Ref<Plot> story = stories[i];
        if (story.is_valid()) {
            names.append(story->get_official_name());
        }
    }
    return names;
}

int Narrative::add_story(const godot::Ref<Plot> &new_story) {
    int id = get_story_id(new_story);
    
    if (id < 0) {
        const godot::Dictionary _undo_before = _capture_roster_snapshot();
        id = stories.size();
        stories.append(new_story);
        story_assignments.append(-1);
        emit_signal("story_added", new_story);
        _record_roster_undo("Add Story to Narrative", _undo_before);
    }
    return id;
}

int Narrative::get_story_id(const godot::Ref<Plot> &story) const {
    for (int i = 0; i < stories.size(); i++) {
        godot::Ref<Plot> s = stories[i];
        if (s == story) return i;
    }
    return -1;
}

bool Narrative::has_story(const godot::Ref<Plot> &story) const {
    return get_story_id(story) >= 0;
}

bool Narrative::remove_story(const godot::Ref<Plot> &story) {
    return remove_story_at(get_story_id(story));
}

bool Narrative::remove_story_at(int story_ID) {
    if (story_ID < 0 || story_ID >= stories.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Plot> story = stories[story_ID];
    
    if (story_ID < story_assignments.size() && (int)story_assignments[story_ID] >= 0) {
        vacate_story(story_ID);
    }
    
    stories.remove_at(story_ID);
    story_assignments.remove_at(story_ID);
    
    emit_signal("story_removed", story);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Remove Story from Narrative", _undo_before);
    return true;
}

int Narrative::get_best_fit_plot_for_story(int story_ID, const godot::TypedArray<int> &plot_mask, BestNarrativeFit best_fit) {
    if (story_ID < 0 || story_ID >= stories.size()) return -1;
    
    int best_score = 100000;
    int best_ID = -1;
    
    godot::Ref<Plot> story_to_fill = stories[story_ID];
    if (!story_to_fill.is_valid()) return -1;
    
    for (int p = 0; p < plots.size(); p++) {
        godot::Ref<Plot> plot = plots[p];
        if (!plot.is_valid()) continue;
        
        bool is_masked = false;
        for (int m = 0; m < plot_mask.size(); m++) {
            if ((int)plot_mask[m] == p) {
                is_masked = true;
                break;
            }
        }
        if (is_masked) continue;
        
        int score = plot->score_similarity(story_to_fill.ptr());
        
        // Compare Plot Events between Story and Plot
        if (best_fit != FIT_BASIC) {
            godot::Array story_events = story_to_fill->call("get_plot_events");
            godot::Array plot_events = plot->call("get_plot_events");
            
            for (int se = 0; se < story_events.size(); se++) {
                godot::Ref<godot::RefCounted> s_event = story_events[se];
                if (!s_event.is_valid()) continue;
                
                bool has_match = false;
                for (int pe = 0; pe < plot_events.size(); pe++) {
                    godot::Ref<godot::RefCounted> p_event = plot_events[pe];
                    if (!p_event.is_valid()) continue;
                    
                    // Match requirements
                    if ((int)s_event->call("get_requirement") == (int)p_event->call("get_requirement")) {
                        has_match = true;
                        break;
                    }
                }
                if (!has_match) score += 1;
            }
        }
        
        if (score < best_score) {
            best_score = score;
            best_ID = p;
        }
    }
    
    return best_ID;
}

bool Narrative::fill_story(int story_ID, int plot_ID, NarrativeReplacement replace_type) {
    if (story_ID < 0 || story_ID >= stories.size() || plot_ID < 0 || plot_ID >= plots.size()) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Plot> story = stories[story_ID];
    if (!story.is_valid()) return false;
    
    godot::Ref<Plot> new_plot = plots[plot_ID];
    if (!new_plot.is_valid()) return false;
    
    godot::Ref<Plot> current_plot = nullptr;
    if (story_ID < story_assignments.size() && (int)story_assignments[story_ID] >= 0) {
        current_plot = plots[(int)story_assignments[story_ID]];
    }
    
    if (replace_type == REPLACE_LITERAL || replace_type == REPLACE_THOROUGH) {
        if (current_plot.is_valid()) {
            
            // Sweep Characters (Relationships and Goals)
            for (int i = 0; i < characters.size(); i++) {
                godot::Ref<Character> character = characters[i];
                if (!character.is_valid()) continue;
                
                // Update Goals
                godot::TypedArray<Plot> char_goals = character->get_goals();
                for (int g = 0; g < char_goals.size(); g++) {
                    godot::Ref<Plot> char_goal = char_goals[g];
                    if (char_goal == current_plot) {
                        // Removing and re-adding to maintain Character's internal signal structure
                        character->remove_goal(current_plot);
                        character->set_goal(new_plot);
                    }
                }
                
                // Update Relationships
                godot::TypedArray<Relationship> char_rels = character->get_relationships();
                for (int r = 0; r < char_rels.size(); r++) {
                    godot::Ref<Relationship> char_rel = char_rels[r];
                    if (char_rel.is_valid() && char_rel->get_relation() == current_plot) {
                        char_rel->set_relation(new_plot);
                    }
                }
            }
            
            // Sweep Incidents (Conditions)
            for (int i = 0; i < incidents.size(); i++) {
                godot::Ref<Incident> incident = incidents[i];
                if (!incident.is_valid()) continue;
                
                godot::Array conds = incident->call("get_conditions");
                for (int j = 0; j < conds.size(); j++) {
                    godot::Ref<godot::RefCounted> cond = conds[j];
                    if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                    
                    if (godot::Ref<Narreme>(cond->call("get_subject")) == current_plot) cond->call("set_subject", new_plot);
                    if (godot::Ref<Narreme>(cond->call("get_object")) == current_plot) cond->call("set_object", new_plot);
                }
            }
        }
    }
    
    if (replace_type == REPLACE_SYMBOLIC || replace_type == REPLACE_THOROUGH) {
        // Sweep Characters
        for (int i = 0; i < characters.size(); i++) {
            godot::Ref<Character> character = characters[i];
            if (!character.is_valid()) continue;
            
            godot::TypedArray<Relationship> char_rels = character->get_relationships();
            for (int r = 0; r < char_rels.size(); r++) {
                godot::Ref<Relationship> char_rel = char_rels[r];
                if (!char_rel.is_valid()) continue;
                
                if (char_rel->get_title() == story->get_official_name() || char_rel->get_relation() == story) {
                    char_rel->set_relation(new_plot);
                }
            }
        }
        
        // Sweep Incidents
        for (int i = 0; i < incidents.size(); i++) {
            godot::Ref<Incident> incident = incidents[i];
            if (!incident.is_valid()) continue;
            
            godot::Array conds = incident->call("get_conditions");
            for (int j = 0; j < conds.size(); j++) {
                godot::Ref<godot::RefCounted> cond = conds[j];
                if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
                
                if (cond->call("get_subject_is_symbolic") && godot::Ref<Narreme>(cond->call("get_subject")) == story) {
                    cond->call("set_subject", new_plot);
                    cond->call("set_subject_is_symbolic", false);
                }
                
                godot::Ref<Narreme> obj_narreme = cond->call("get_object");
                if (cond->call("get_object_is_symbolic") && obj_narreme.is_valid() && obj_narreme->get_class_name_str() == "Plot") {
                    cond->call("set_object", new_plot);
                    cond->call("set_object_is_symbolic", false);
                }
            }
        }
    }
    
    if (current_plot.is_valid()) {
        emit_signal("story_vacated", story, current_plot);
    }
    
    if (story_ID >= story_assignments.size()) story_assignments.resize(story_ID + 1);
    story_assignments[story_ID] = plot_ID;
    
    emit_signal("story_filled", story, new_plot);
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Fill Story", _undo_before);
    return true;
}

bool Narrative::vacate_story(int story_ID) {
    if (story_ID < 0 || story_ID >= story_assignments.size() || (int)story_assignments[story_ID] < 0) return false;
    const godot::Dictionary _undo_before = _capture_roster_snapshot();
    ScopedNarremeUndoSuppress _undo_guard(undo_redo_suppressed_);
    
    godot::Ref<Plot> story = stories[story_ID];
    if (!story.is_valid()) return false;
    
    godot::Ref<Plot> current_plot = plots[(int)story_assignments[story_ID]];
    if (!current_plot.is_valid()) return false;
    
    // Sweep Characters
    for (int i = 0; i < characters.size(); i++) {
        godot::Ref<Character> character = characters[i];
        if (!character.is_valid()) continue;
        
        // Remove Goals
        godot::TypedArray<Plot> char_goals = character->get_goals();
        for (int g = 0; g < char_goals.size(); g++) {
            godot::Ref<Plot> char_goal = char_goals[g];
            if (char_goal == current_plot) {
                character->remove_goal(current_plot);
            }
        }
        
        // Clean Relationships
        godot::TypedArray<Relationship> char_rels = character->get_relationships();
        for (int r = 0; r < char_rels.size(); r++) {
            godot::Ref<Relationship> char_rel = char_rels[r];
            if (!char_rel.is_valid()) continue;
            
            if (char_rel->get_relation() == current_plot) char_rel->set_relation(nullptr);
            else if (char_rel->get_title() == story->get_official_name()) char_rel->set_relation(nullptr);
        }
    }
    
    // Sweep Incidents
    for (int i = 0; i < incidents.size(); i++) {
        godot::Ref<Incident> incident = incidents[i];
        if (!incident.is_valid()) continue;
        
        godot::Array conds = incident->call("get_conditions");
        for (int j = 0; j < conds.size(); j++) {
            godot::Ref<godot::RefCounted> cond = conds[j];
            if (!cond.is_valid() || cond->get_class() != "Narrative_Condition") continue;
            
            if (godot::Ref<Narreme>(cond->call("get_subject")) == current_plot) cond->call("set_subject", nullptr);
            if (godot::Ref<Narreme>(cond->call("get_object")) == current_plot) cond->call("set_object", nullptr);
        }
    }
    
    story_assignments[story_ID] = -1;
    
    undo_redo_suppressed_ = _undo_guard.previous_value;
    _record_roster_undo("Vacate Story", _undo_before);
    return true;
}


// ----------------------------------------------------------------------------
// NARRATIVE CONDITIONS OVERRIDES
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// UNDO / REDO ROSTER SNAPSHOTS & PROPAGATION
// ----------------------------------------------------------------------------

void Narrative::propagate_undo_redo(const godot::Ref<IdeamUndoRedo> &p_undo_redo) {
    set_undo_redo(p_undo_redo);

    auto wire_character_list = [&](const godot::TypedArray<Character> &p_list) {
        for (int i = 0; i < p_list.size(); ++i) {
            godot::Ref<Character> c = p_list[i];
            if (!c.is_valid()) continue;
            c->set_undo_redo(p_undo_redo);
            godot::TypedArray<Relationship> rels = c->get_relationships();
            for (int r = 0; r < rels.size(); ++r) {
                godot::Ref<Relationship> rel = rels[r];
                if (rel.is_valid()) rel->set_undo_redo(p_undo_redo);
            }
        }
    };
    auto wire_prop_list = [&](const godot::TypedArray<Prop> &p_list) {
        for (int i = 0; i < p_list.size(); ++i) {
            godot::Ref<Prop> p = p_list[i];
            if (p.is_valid()) p->set_undo_redo(p_undo_redo);
        }
    };
    auto wire_location_list = [&](const godot::TypedArray<Location> &p_list) {
        for (int i = 0; i < p_list.size(); ++i) {
            godot::Ref<Location> l = p_list[i];
            if (l.is_valid()) l->set_undo_redo(p_undo_redo);
        }
    };
    auto wire_incident_list = [&](const godot::TypedArray<Incident> &p_list) {
        for (int i = 0; i < p_list.size(); ++i) {
            godot::Ref<Incident> inc = p_list[i];
            if (!inc.is_valid()) continue;
            inc->set_undo_redo(p_undo_redo);
            godot::TypedArray<Incident_Condition> conds = inc->get_conditions();
            for (int k = 0; k < conds.size(); ++k) {
                godot::Ref<Incident_Condition> cond = conds[k];
                if (cond.is_valid()) cond->set_undo_redo(p_undo_redo);
            }
        }
    };
    auto wire_plot_list = [&](const godot::TypedArray<Plot> &p_list) {
        for (int i = 0; i < p_list.size(); ++i) {
            godot::Ref<Plot> pl = p_list[i];
            if (!pl.is_valid()) continue;
            pl->set_undo_redo(p_undo_redo);
            godot::TypedArray<Plot_Event> evs = pl->get_plot_events();
            for (int e = 0; e < evs.size(); ++e) {
                godot::Ref<Plot_Event> ev = evs[e];
                if (ev.is_valid()) ev->set_undo_redo(p_undo_redo);
            }
        }
    };

    wire_character_list(characters);
    wire_character_list(roles);
    wire_prop_list(props);
    wire_prop_list(symbols);
    wire_location_list(locations);
    wire_location_list(places);
    wire_incident_list(incidents);
    wire_incident_list(moments);
    wire_plot_list(plots);
    wire_plot_list(stories);
}

godot::Dictionary Narrative::_capture_roster_snapshot() const {
    godot::Dictionary snap;
    snap["characters"]         = characters.duplicate();
    snap["roles"]              = roles.duplicate();
    snap["role_assignments"]   = role_assignments.duplicate();
    snap["props"]              = props.duplicate();
    snap["symbols"]            = symbols.duplicate();
    snap["symbol_assignments"] = symbol_assignments.duplicate();
    snap["locations"]          = locations.duplicate();
    snap["places"]             = places.duplicate();
    snap["place_assignments"]  = place_assignments.duplicate();
    snap["incidents"]          = incidents.duplicate();
    snap["moments"]            = moments.duplicate();
    snap["moment_assignments"] = moment_assignments.duplicate();
    snap["plots"]              = plots.duplicate();
    snap["stories"]            = stories.duplicate();
    snap["story_assignments"]  = story_assignments.duplicate();
    return snap;
}

void Narrative::_apply_roster_snapshot(const godot::Dictionary &p_snapshot) {
    if (p_snapshot.has("characters"))         characters         = p_snapshot["characters"].duplicate();
    if (p_snapshot.has("roles"))              roles              = p_snapshot["roles"].duplicate();
    if (p_snapshot.has("role_assignments"))   role_assignments   = p_snapshot["role_assignments"].duplicate();
    if (p_snapshot.has("props"))              props              = p_snapshot["props"].duplicate();
    if (p_snapshot.has("symbols"))            symbols            = p_snapshot["symbols"].duplicate();
    if (p_snapshot.has("symbol_assignments")) symbol_assignments = p_snapshot["symbol_assignments"].duplicate();
    if (p_snapshot.has("locations"))          locations          = p_snapshot["locations"].duplicate();
    if (p_snapshot.has("places"))             places             = p_snapshot["places"].duplicate();
    if (p_snapshot.has("place_assignments"))  place_assignments  = p_snapshot["place_assignments"].duplicate();
    if (p_snapshot.has("incidents"))          incidents          = p_snapshot["incidents"].duplicate();
    if (p_snapshot.has("moments"))            moments            = p_snapshot["moments"].duplicate();
    if (p_snapshot.has("moment_assignments")) moment_assignments = p_snapshot["moment_assignments"].duplicate();
    if (p_snapshot.has("plots"))              plots              = p_snapshot["plots"].duplicate();
    if (p_snapshot.has("stories"))            stories            = p_snapshot["stories"].duplicate();
    if (p_snapshot.has("story_assignments"))  story_assignments  = p_snapshot["story_assignments"].duplicate();
    emit_signal("roster_changed");
    emit_changed();
}

void Narrative::_record_roster_undo(const godot::String &p_action_name, const godot::Dictionary &p_before_snapshot) {
    if (!_can_record_undo()) {
        emit_signal("roster_changed");
        emit_changed();
        return;
    }
    const godot::Dictionary after_snapshot = _capture_roster_snapshot();
    undo_redo_->create_action(p_action_name);
    undo_redo_->add_do_method(callable_mp(this, &Narrative::_apply_roster_snapshot).bind(after_snapshot));
    undo_redo_->add_undo_method(callable_mp(this, &Narrative::_apply_roster_snapshot).bind(p_before_snapshot));
    undo_redo_->commit_action(false);
    emit_signal("roster_changed");
    emit_changed();
}

void Narrative::_shift_assignments_after_literal_removal(godot::TypedArray<int> &p_assignments, int p_removed_literal_id) {
    for (int i = 0; i < p_assignments.size(); ++i) {
        const int assigned = static_cast<int>(p_assignments[i]);
        if (assigned == p_removed_literal_id) {
            p_assignments[i] = -1;
        } else if (assigned > p_removed_literal_id) {
            p_assignments[i] = assigned - 1;
        }
    }
}

// ----------------------------------------------------------------------------
// UNIFIED QUERY, FILTERING, REAL <-> SYMBOLIC CONVERSION & INSTANTIATION
// ----------------------------------------------------------------------------

godot::Ref<Narreme> Narrative::find_symbolic_from_literal(const godot::Ref<Narreme> &narreme) const {
    if (!narreme.is_valid()) return nullptr;
    const godot::String class_name = narreme->get_class_name_str();

    if (class_name == "Character") {
        const int id = get_character_id(narreme);
        if (id >= 0) {
            for (int i = 0; i < role_assignments.size() && i < roles.size(); ++i) {
                if (static_cast<int>(role_assignments[i]) == id) return roles[i];
            }
        }
    } else if (class_name == "Prop") {
        const int id = get_prop_id(narreme);
        if (id >= 0) {
            for (int i = 0; i < symbol_assignments.size() && i < symbols.size(); ++i) {
                if (static_cast<int>(symbol_assignments[i]) == id) return symbols[i];
            }
        }
    } else if (class_name == "Location") {
        const int id = get_location_id(narreme);
        if (id >= 0) {
            for (int i = 0; i < place_assignments.size() && i < places.size(); ++i) {
                if (static_cast<int>(place_assignments[i]) == id) return places[i];
            }
        }
    } else if (class_name == "Incident") {
        const int id = get_incident_id(narreme);
        if (id >= 0) {
            for (int i = 0; i < moment_assignments.size() && i < moments.size(); ++i) {
                if (static_cast<int>(moment_assignments[i]) == id) return moments[i];
            }
        }
    } else if (class_name == "Plot") {
        const int id = get_plot_id(narreme);
        if (id >= 0) {
            for (int i = 0; i < story_assignments.size() && i < stories.size(); ++i) {
                if (static_cast<int>(story_assignments[i]) == id) return stories[i];
            }
        }
    }
    return nullptr;
}

godot::TypedArray<Narreme> Narrative::get_all_narremes(
    NarremeFilterMode p_filter,
    const godot::String &p_class_filter
) const {
    godot::TypedArray<Narreme> result;
    const bool include_literal  = (p_filter == FILTER_ALL || p_filter == FILTER_LITERAL_ONLY);
    const bool include_symbolic = (p_filter == FILTER_ALL || p_filter == FILTER_SYMBOLIC_ONLY);

    auto matches_class = [&](const char *p_cls) {
        return p_class_filter.is_empty() || p_class_filter == p_cls;
    };

    if (matches_class("Character")) {
        if (include_literal) {
            for (int i = 0; i < characters.size(); ++i) {
                godot::Ref<Narreme> n = characters[i];
                if (n.is_valid()) result.append(n);
            }
        }
        if (include_symbolic) {
            for (int i = 0; i < roles.size(); ++i) {
                godot::Ref<Narreme> n = roles[i];
                if (n.is_valid()) result.append(n);
            }
        }
    }
    if (matches_class("Prop")) {
        if (include_literal) {
            for (int i = 0; i < props.size(); ++i) {
                godot::Ref<Narreme> n = props[i];
                if (n.is_valid()) result.append(n);
            }
        }
        if (include_symbolic) {
            for (int i = 0; i < symbols.size(); ++i) {
                godot::Ref<Narreme> n = symbols[i];
                if (n.is_valid()) result.append(n);
            }
        }
    }
    if (matches_class("Location")) {
        if (include_literal) {
            for (int i = 0; i < locations.size(); ++i) {
                godot::Ref<Narreme> n = locations[i];
                if (n.is_valid()) result.append(n);
            }
        }
        if (include_symbolic) {
            for (int i = 0; i < places.size(); ++i) {
                godot::Ref<Narreme> n = places[i];
                if (n.is_valid()) result.append(n);
            }
        }
    }
    if (matches_class("Incident")) {
        if (include_literal) {
            for (int i = 0; i < incidents.size(); ++i) {
                godot::Ref<Narreme> n = incidents[i];
                if (n.is_valid()) result.append(n);
            }
        }
        if (include_symbolic) {
            for (int i = 0; i < moments.size(); ++i) {
                godot::Ref<Narreme> n = moments[i];
                if (n.is_valid()) result.append(n);
            }
        }
    }
    if (matches_class("Plot")) {
        if (include_literal) {
            for (int i = 0; i < plots.size(); ++i) {
                godot::Ref<Narreme> n = plots[i];
                if (n.is_valid()) result.append(n);
            }
        }
        if (include_symbolic) {
            for (int i = 0; i < stories.size(); ++i) {
                godot::Ref<Narreme> n = stories[i];
                if (n.is_valid()) result.append(n);
            }
        }
    }

    return result;
}

bool Narrative::is_narreme_symbolic(const godot::Ref<Narreme> &p_narreme) const {
    if (!p_narreme.is_valid()) return false;
    const godot::String cls = p_narreme->get_class_name_str();
    if (cls == "Character") return has_role(p_narreme);
    if (cls == "Prop")      return has_symbol(p_narreme);
    if (cls == "Location")  return has_place(p_narreme);
    if (cls == "Incident")  return has_moment(p_narreme);
    if (cls == "Plot")      return has_story(p_narreme);
    return false;
}

bool Narrative::is_narreme_literal(const godot::Ref<Narreme> &p_narreme) const {
    if (!p_narreme.is_valid()) return false;
    const godot::String cls = p_narreme->get_class_name_str();
    if (cls == "Character") return has_character(p_narreme);
    if (cls == "Prop")      return has_prop(p_narreme);
    if (cls == "Location")  return has_location(p_narreme);
    if (cls == "Incident")  return has_incident(p_narreme);
    if (cls == "Plot")      return has_plot(p_narreme);
    return false;
}

bool Narrative::convert_character_to_role(int p_character_id) {
    if (p_character_id < 0 || p_character_id >= characters.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Character> target   = characters[p_character_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_character_at(p_character_id);
        add_role(target);
    }
    _record_roster_undo("Convert Character to Role", before);
    return true;
}

bool Narrative::convert_role_to_character(int p_role_id) {
    if (p_role_id < 0 || p_role_id >= roles.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Character> target   = roles[p_role_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_role_at(p_role_id);
        add_character(target);
    }
    _record_roster_undo("Convert Role to Character", before);
    return true;
}

bool Narrative::convert_prop_to_symbol(int p_prop_id) {
    if (p_prop_id < 0 || p_prop_id >= props.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Prop> target        = props[p_prop_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_prop_at(p_prop_id);
        add_symbol(target);
    }
    _record_roster_undo("Convert Prop to Symbol", before);
    return true;
}

bool Narrative::convert_symbol_to_prop(int p_symbol_id) {
    if (p_symbol_id < 0 || p_symbol_id >= symbols.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Prop> target        = symbols[p_symbol_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_symbol_at(p_symbol_id);
        add_prop(target);
    }
    _record_roster_undo("Convert Symbol to Prop", before);
    return true;
}

bool Narrative::convert_location_to_place(int p_location_id) {
    if (p_location_id < 0 || p_location_id >= locations.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Location> target    = locations[p_location_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_location_at(p_location_id);
        add_place(target);
    }
    _record_roster_undo("Convert Location to Place", before);
    return true;
}

bool Narrative::convert_place_to_location(int p_place_id) {
    if (p_place_id < 0 || p_place_id >= places.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Location> target    = places[p_place_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_place_at(p_place_id);
        add_location(target);
    }
    _record_roster_undo("Convert Place to Location", before);
    return true;
}

bool Narrative::convert_incident_to_moment(int p_incident_id) {
    if (p_incident_id < 0 || p_incident_id >= incidents.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Incident> target    = incidents[p_incident_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_incident_at(p_incident_id);
        add_moment(target);
    }
    _record_roster_undo("Convert Incident to Moment", before);
    return true;
}

bool Narrative::convert_moment_to_incident(int p_moment_id) {
    if (p_moment_id < 0 || p_moment_id >= moments.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Incident> target    = moments[p_moment_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_moment_at(p_moment_id);
        add_incident(target);
    }
    _record_roster_undo("Convert Moment to Incident", before);
    return true;
}

bool Narrative::convert_plot_to_story(int p_plot_id) {
    if (p_plot_id < 0 || p_plot_id >= plots.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Plot> target        = plots[p_plot_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_plot_at(p_plot_id);
        add_story(target);
    }
    _record_roster_undo("Convert Plot to Story", before);
    return true;
}

bool Narrative::convert_story_to_plot(int p_story_id) {
    if (p_story_id < 0 || p_story_id >= stories.size()) return false;
    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Plot> target        = stories[p_story_id];
    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        remove_story_at(p_story_id);
        add_plot(target);
    }
    _record_roster_undo("Convert Story to Plot", before);
    return true;
}

bool Narrative::convert_to_symbolic(const godot::Ref<Narreme> &p_narreme) {
    if (!p_narreme.is_valid()) return false;
    const godot::String cls = p_narreme->get_class_name_str();
    if (cls == "Character") return convert_character_to_role(get_character_id(p_narreme));
    if (cls == "Prop")      return convert_prop_to_symbol(get_prop_id(p_narreme));
    if (cls == "Location")  return convert_location_to_place(get_location_id(p_narreme));
    if (cls == "Incident")  return convert_incident_to_moment(get_incident_id(p_narreme));
    if (cls == "Plot")      return convert_plot_to_story(get_plot_id(p_narreme));
    return false;
}

bool Narrative::convert_to_literal(const godot::Ref<Narreme> &p_narreme) {
    if (!p_narreme.is_valid()) return false;
    const godot::String cls = p_narreme->get_class_name_str();
    if (cls == "Character") return convert_role_to_character(get_role_id(p_narreme));
    if (cls == "Prop")      return convert_symbol_to_prop(get_symbol_id(p_narreme));
    if (cls == "Location")  return convert_place_to_location(get_place_id(p_narreme));
    if (cls == "Incident")  return convert_moment_to_incident(get_moment_id(p_narreme));
    if (cls == "Plot")      return convert_story_to_plot(get_story_id(p_narreme));
    return false;
}

godot::Ref<Character> Narrative::instantiate_character_from_role(
    int p_role_id,
    const godot::String &p_new_name,
    bool p_auto_fill,
    NarrativeReplacement p_replace_type
) {
    if (p_role_id < 0 || p_role_id >= roles.size()) return nullptr;
    godot::Ref<Character> role = roles[p_role_id];
    if (!role.is_valid()) return nullptr;

    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Character> inst = role->duplicate(false);
    if (!inst.is_valid()) {
        inst.instantiate();
    }
    const godot::String base_name = role->get_official_name().is_empty() ? godot::String("Character") : role->get_official_name();
    inst->set_official_name(p_new_name.is_empty() ? (base_name + " (Instance)") : p_new_name);
    if (role->get_semantic_tags().is_valid()) {
        inst->set_semantic_tags(role->get_semantic_tags()->duplicate(true));
    }
    if (undo_redo_.is_valid()) {
        inst->set_undo_redo(undo_redo_);
    }

    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        const int new_id = add_character(inst);
        if (p_auto_fill && new_id >= 0) {
            fill_role(p_role_id, new_id, p_replace_type);
        }
    }
    _record_roster_undo("Instantiate Character from Role", before);
    return inst;
}

godot::Ref<Prop> Narrative::instantiate_prop_from_symbol(
    int p_symbol_id,
    const godot::String &p_new_name,
    bool p_auto_fill,
    NarrativeReplacement p_replace_type
) {
    if (p_symbol_id < 0 || p_symbol_id >= symbols.size()) return nullptr;
    godot::Ref<Prop> symbol = symbols[p_symbol_id];
    if (!symbol.is_valid()) return nullptr;

    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Prop> inst = symbol->duplicate(false);
    if (!inst.is_valid()) {
        inst.instantiate();
    }
    const godot::String base_name = symbol->get_official_name().is_empty() ? godot::String("Prop") : symbol->get_official_name();
    inst->set_official_name(p_new_name.is_empty() ? (base_name + " (Instance)") : p_new_name);
    if (symbol->get_semantic_tags().is_valid()) {
        inst->set_semantic_tags(symbol->get_semantic_tags()->duplicate(true));
    }
    if (undo_redo_.is_valid()) {
        inst->set_undo_redo(undo_redo_);
    }

    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        const int new_id = add_prop(inst);
        if (p_auto_fill && new_id >= 0) {
            fill_symbol(p_symbol_id, new_id, p_replace_type);
        }
    }
    _record_roster_undo("Instantiate Prop from Symbol", before);
    return inst;
}

godot::Ref<Location> Narrative::instantiate_location_from_place(
    int p_place_id,
    const godot::String &p_new_name,
    bool p_auto_fill,
    NarrativeReplacement p_replace_type
) {
    if (p_place_id < 0 || p_place_id >= places.size()) return nullptr;
    godot::Ref<Location> place = places[p_place_id];
    if (!place.is_valid()) return nullptr;

    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Location> inst = place->duplicate(false);
    if (!inst.is_valid()) {
        inst.instantiate();
    }
    const godot::String base_name = place->get_official_name().is_empty() ? godot::String("Location") : place->get_official_name();
    inst->set_official_name(p_new_name.is_empty() ? (base_name + " (Instance)") : p_new_name);
    if (place->get_semantic_tags().is_valid()) {
        inst->set_semantic_tags(place->get_semantic_tags()->duplicate(true));
    }
    if (undo_redo_.is_valid()) {
        inst->set_undo_redo(undo_redo_);
    }

    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        const int new_id = add_location(inst);
        if (p_auto_fill && new_id >= 0) {
            fill_place(p_place_id, new_id, p_replace_type);
        }
    }
    _record_roster_undo("Instantiate Location from Place", before);
    return inst;
}

godot::Ref<Incident> Narrative::instantiate_incident_from_moment(
    int p_moment_id,
    const godot::String &p_new_name,
    bool p_auto_fill,
    NarrativeReplacement p_replace_type
) {
    if (p_moment_id < 0 || p_moment_id >= moments.size()) return nullptr;
    godot::Ref<Incident> moment = moments[p_moment_id];
    if (!moment.is_valid()) return nullptr;

    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Incident> inst = moment->duplicate(false);
    if (!inst.is_valid()) {
        inst.instantiate();
    }
    const godot::String base_name = moment->get_official_name().is_empty() ? godot::String("Incident") : moment->get_official_name();
    inst->set_official_name(p_new_name.is_empty() ? (base_name + " (Instance)") : p_new_name);
    if (moment->get_semantic_tags().is_valid()) {
        inst->set_semantic_tags(moment->get_semantic_tags()->duplicate(true));
    }
    if (undo_redo_.is_valid()) {
        inst->set_undo_redo(undo_redo_);
    }

    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        const int new_id = add_incident(inst);
        if (p_auto_fill && new_id >= 0) {
            fill_moment(p_moment_id, new_id, p_replace_type);
        }
    }
    _record_roster_undo("Instantiate Incident from Moment", before);
    return inst;
}

godot::Ref<Plot> Narrative::instantiate_plot_from_story(
    int p_story_id,
    const godot::String &p_new_name,
    bool p_auto_fill,
    NarrativeReplacement p_replace_type
) {
    if (p_story_id < 0 || p_story_id >= stories.size()) return nullptr;
    godot::Ref<Plot> story = stories[p_story_id];
    if (!story.is_valid()) return nullptr;

    const godot::Dictionary before = _capture_roster_snapshot();
    godot::Ref<Plot> inst = story->duplicate(false);
    if (!inst.is_valid()) {
        inst.instantiate();
    }
    const godot::String base_name = story->get_official_name().is_empty() ? godot::String("Plot") : story->get_official_name();
    inst->set_official_name(p_new_name.is_empty() ? (base_name + " (Instance)") : p_new_name);
    if (story->get_semantic_tags().is_valid()) {
        inst->set_semantic_tags(story->get_semantic_tags()->duplicate(true));
    }
    if (undo_redo_.is_valid()) {
        inst->set_undo_redo(undo_redo_);
    }

    {
        ScopedNarremeUndoSuppress guard(undo_redo_suppressed_);
        const int new_id = add_plot(inst);
        if (p_auto_fill && new_id >= 0) {
            fill_story(p_story_id, new_id, p_replace_type);
        }
    }
    _record_roster_undo("Instantiate Plot from Story", before);
    return inst;
}

godot::Ref<Narreme> Narrative::instantiate_literal_from_symbolic(
    const godot::Ref<Narreme> &p_symbolic,
    const godot::String &p_new_name,
    bool p_auto_fill
) {
    if (!p_symbolic.is_valid()) return nullptr;
    const godot::String cls = p_symbolic->get_class_name_str();
    if (cls == "Character") return instantiate_character_from_role(get_role_id(p_symbolic), p_new_name, p_auto_fill);
    if (cls == "Prop")      return instantiate_prop_from_symbol(get_symbol_id(p_symbolic), p_new_name, p_auto_fill);
    if (cls == "Location")  return instantiate_location_from_place(get_place_id(p_symbolic), p_new_name, p_auto_fill);
    if (cls == "Incident")  return instantiate_incident_from_moment(get_moment_id(p_symbolic), p_new_name, p_auto_fill);
    if (cls == "Plot")      return instantiate_plot_from_story(get_story_id(p_symbolic), p_new_name, p_auto_fill);
    return nullptr;
}

godot::TypedArray<godot::Dictionary> Narrative::get_narrative_condition_descriptors(Narreme *p_narreme) const {
    godot::TypedArray<godot::Dictionary> out;

    auto make_tags = [](std::initializer_list<const char *> p_list) {
        godot::PackedStringArray tags;
        for (const char *t : p_list) {
            tags.append(godot::String(t));
        }
        return tags;
    };

    if (p_narreme) {
        const godot::String class_name = p_narreme->get_class_name_str();
        if (class_name == "Narrative") {
            out.append(make_condition_descriptor(
                0, "narrative.is_same", "is",
                "Identity", "Entity", "Compare_entity",
                make_tags({"identity", "narrative"}), true, "Narrative"
            ));
        }
    } else {
        out.append(make_condition_descriptor(0, "narrative.has_characters", "has characters", "Inclusion", "Total", "", make_tags({"narrative", "character", "literal"}), false));
        out.append(make_condition_descriptor(1, "narrative.has_roles",      "has roles",      "Inclusion", "Total", "", make_tags({"narrative", "character", "symbolic"}), false));
        out.append(make_condition_descriptor(2, "narrative.has_props",      "has props",      "Inclusion", "Total", "", make_tags({"narrative", "prop", "literal"}), false));
        out.append(make_condition_descriptor(3, "narrative.has_symbols",    "has symbols",    "Inclusion", "Total", "", make_tags({"narrative", "prop", "symbolic"}), false));
        out.append(make_condition_descriptor(4, "narrative.has_locations",  "has locations",  "Inclusion", "Total", "", make_tags({"narrative", "location", "literal"}), false));
        out.append(make_condition_descriptor(5, "narrative.has_places",     "has places",     "Inclusion", "Total", "", make_tags({"narrative", "location", "symbolic"}), false));
        out.append(make_condition_descriptor(6, "narrative.has_incidents",  "has incidents",  "Inclusion", "Total", "", make_tags({"narrative", "incident", "literal"}), false));
        out.append(make_condition_descriptor(7, "narrative.has_moments",    "has moments",    "Inclusion", "Total", "", make_tags({"narrative", "incident", "symbolic"}), false));
        out.append(make_condition_descriptor(8, "narrative.has_plots",      "has plots",      "Inclusion", "Total", "", make_tags({"narrative", "plot", "literal"}), false));
        out.append(make_condition_descriptor(9, "narrative.has_stories",    "has stories",    "Inclusion", "Total", "", make_tags({"narrative", "plot", "symbolic"}), false));
    }

    return out;
}

godot::Array Narrative::get_narrative_conditions(Narreme *p_narreme) const {
    return Narreme::get_narrative_conditions(p_narreme);
}

NarremeConditionStatus Narrative::check_narrative_condition(int p_condition_id, Narreme *p_conditional_narreme) const {
    if (p_conditional_narreme) {
        godot::String class_name = p_conditional_narreme->get_class_name_str();
        if (class_name == "Narrative") {
            switch (p_condition_id) {
                case 0:
                    return (this == p_conditional_narreme) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            }
        }
    } else {
        switch (p_condition_id) {
            case 0: return (characters.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 1: return (roles.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 2: return (props.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 3: return (symbols.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 4: return (locations.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 5: return (places.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 6: return (incidents.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 7: return (moments.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 8: return (plots.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
            case 9: return (stories.size() > 0) ? NarremeConditionStatus::MET : NarremeConditionStatus::NOT_MET;
        }
    }
    return NarremeConditionStatus::UNKNOWN;
}

godot::String Narrative::get_class_name_str() const {
    return "Narrative";
}

} // namespace ideam::godot_ext