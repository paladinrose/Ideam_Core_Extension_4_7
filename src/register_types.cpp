#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

// UI/Editor Handshake
#include "godot/editor/ideam_editor_plugin.h"
#include "godot/editor/ideam_editor_inspector_plugin.h"

// --- Utilities ---
#include "godot/utilities/ideam_undo_redo.h"
#include "godot/utilities/node_retargeter.h"
#include "godot/utilities/signal_connector.h"

// --- Controls ---
#include "godot/controls/runtime_resource_picker.h"
#include "godot/controls/runtime_inspector.h"
#include "godot/controls/heatmap_grid.h"
#include "godot/controls/circular_gauge.h"
#include "godot/controls/selection_list.h"

#include "godot/controls/theme_registry.h"
#include "godot/controls/theme_selector.h"

// --- Graphs ---
#include "godot/graphs/ideam_graphs_plugin.h"

#include "godot/graphs/ideam_graph_resource.h"
#include "godot/graphs/ideam_graph_node_resource.h"
#include "godot/graphs/ideam_graph_edit.h"
#include "godot/graphs/ideam_graph_group_resource.h"
#include "godot/graphs/ideam_graph_node.h"
#include "godot/graphs/ideam_graph_inspector.h"
#include "godot/graphs/graph_composer.h"

// --- Memory ---
#include "godot/memory/ideam_memory_plugin.h"

#include "godot/memory/memory_manager_resource.h"
#include "godot/memory/memory_profiler.h"
#include "godot/memory/memory_ribbon.h"
#include "godot/memory/memory_ribbon_button.h"
#include "godot/memory/memory_block_button.h"
#include "godot/memory/block_grant_badge.h"
#include "godot/memory/memory_manager_inspector.h"

#include "godot/memory/memory_buffer_column_panel.h"
#include "godot/memory/memory_buffer_linkage_panel.h"
#include "godot/memory/managed_buffer_resource.h"
#include "godot/memory/managed_buffer_view.h"
#include "godot/memory/memory_buffer_tooltip.h"

#include "godot/memory/memory_buffer_resource.h"
#include "godot/memory/memory_buffer_view.h"

#include "godot/memory/aos_visualizer.h"
#include "godot/memory/flat_visualizer.h"
#include "godot/memory/memory_page_grid.h"
#include "godot/memory/ring_visualizer.h"
#include "godot/memory/soa_visualizer.h"
#include "godot/memory/sparse_set_visualizer.h"
#include "godot/memory/tiled_soa_visualizer.h"

#include "godot/memory/memory_grant_resource.h"
#include "godot/memory/memory_grant_view.h"

#include "godot/memory/grant_part_resource.h"
#include "godot/memory/grant_part_view.h"

#include "godot/memory/memory_buffer_selection_resource.h"
#include "godot/memory/memory_buffer_selection_view.h"
#include "godot/memory/grant_request_window.h"
#include "godot/memory/memory_inspectors.h"

#include "godot/memory/memory_graph_resource.h"
#include "godot/memory/memory_graph_edit.h"
#include "godot/memory/memory_graph_inspector.h"

#include "godot/memory/memory_graph_node_resource.h"
#include "godot/memory/memory_graph_node.h"

// --- Tasks ---
#include "godot/tasks/ideam_tasks_plugin.h"

#include "godot/tasks/task_graph_resource.h"
#include "godot/tasks/task_graph_edit.h"
#include "godot/tasks/task_graph_inspector.h"

#include "godot/tasks/task_resource.h"
#include "godot/tasks/task_graph_node.h"
#include "godot/tasks/task_graph_host.h" 

#include "godot/tasks/query_task_graph_node.h"
#include "godot/tasks/query_task_resource.h"

#include "godot/tasks/metadata_task_graph_node.h"
#include "godot/tasks/metadata_task_resource.h"

#include "godot/tasks/transform_task_graph_node.h"
#include "godot/tasks/transform_task_resource.h"

#include "godot/tasks/sub_graph_task_graph_node.h"
#include "godot/tasks/sub_graph_task_resource.h"

#include "godot/tasks/entry_fill_task_graph_node.h"
#include "godot/tasks/entry_fill_task_resource.h"

// --- Task Registration & Domain Packs ---
#include "core/tasks/registration/ideam_task_registry.h"
#include "core/tasks/registration/task_manifest.h"
#include "core/tasks/domains/games/registration/games_task_domain_pack.h"
#include "core/tasks/domains/geometry/registration/geometry_task_domain_pack.h"
#include "core/tasks/domains/minds/registration/minds_task_domain_pack.h"
#include "core/tasks/domains/narratives/registration/narratives_task_domain_pack.h"
#include "core/tasks/domains/splines/registration/splines_task_domain_pack.h"
#include "core/tasks/domains/tiles/registration/tiles_task_domain_pack.h"
#include "core/tasks/domains/procedural_generation/registration/procedural_generation_task_domain_pack.h"
#include "core/tasks/domains/wave_function_collapse/registration/wfc_task_domain_pack.h"
#include "core/tasks/domains/quantum_darwinism/registration/quantum_darwinism_task_domain_pack.h"
#include "core/tasks/domains/linguistics/registration/linguistics_task_domain_pack.h"
#include "core/tasks/domains/communications/registration/communications_task_domain_pack.h"

// --- Geometry (Shared Resources, 2D-to-3D Projector, 2D Canvases & Controls, DOD Bridge, Inspectors & Plugin) ---
#include "godot/geometry/geometry_resource.h"
#include "godot/geometry/geometry_2d_projector_3d.h"
#include "godot/geometry/resources/polygon_2d_resource.h"
#include "godot/geometry/resources/voronoi_diagram_2d_resource.h"
#include "godot/geometry/resources/sdf_grid_2d_resource.h"
#include "godot/geometry/resources/tensor_field_2d_resource.h"
#include "godot/geometry/ui/geometry_canvas_2d_base.h"
#include "godot/geometry/ui/voronoi_2d_canvas.h"
#include "godot/geometry/ui/sdf_grid_2d_canvas.h"
#include "godot/geometry/ui/geometry_csg_bridge_control.h"
#include "godot/geometry/ui/polygon_2d_control.h"
#include "godot/geometry/editor/geometry_editor_inspector_base.h"
#include "godot/geometry/ideam_geometry_plugin.h"
#include "godot/tasks/domains/geometry/data_oriented_geometry.h"
#include "godot/tasks/domains/geometry/geometry_task_host.h"

// --- Splines (Resources, SceneTree Nodes, Projectors, Tweener, DOD Bridge, UI & Plugin) ---
#include "godot/splines/spline_resource.h"
#include "godot/splines/ui/spline_canvas_base.h"
#include "godot/splines/ui/spline_control_base.h"
#include "godot/splines/ui/spline_point_field.h"
#include "godot/splines/ui/spline_section_field.h"
#include "godot/splines/ui/spline_path_container.h"
#include "godot/splines/ui/spline_hitbox_button.h"
#include "godot/splines/ui/spline_envelope_slider.h"
#include "godot/splines/ui/spline_tweener.h"

#include "godot/splines/spline_1d/spline_1d_resource.h"
#include "godot/splines/spline_1d/spline_node.h"
#include "godot/splines/spline_1d/spline_1d_control.h"

#include "godot/splines/spline_2d/spline_2d_resource.h"
#include "godot/splines/spline_2d/spline_node2d.h"
#include "godot/splines/spline_2d/spline_polygon_2d.h"
#include "godot/splines/spline_2d/spline_2d_projector_3d.h"
#include "godot/splines/spline_2d/spline_2d_control.h"

#include "godot/splines/spline_3d/spline_3d_resource.h"
#include "godot/splines/spline_3d/spline_node3d.h"
#include "godot/splines/spline_3d/spline_3d_control.h"
#include "godot/splines/spline_3d/spline_3d_gizmo.h"
#include "godot/splines/spline_3d/spline_extrude_mesh_3d.h"
#include "godot/splines/spline_3d/spline_multi_mesh_3d.h"
#include "godot/splines/spline_3d/spline_zone_3d.h"

#include "godot/splines/spline_4d/spline_4d_resource.h"
#include "godot/splines/spline_4d/spline_node4d.h"
#include "godot/splines/spline_4d/spline_4d_control.h"
#include "godot/splines/spline_4d/color_spline_control.h"
#include "godot/splines/spline_4d/spline_4d_gizmo.h"

#include "godot/splines/editor/spline_editor_inspector_base.h"
#include "godot/splines/editor/spline_1d_resource_inspector.h"
#include "godot/splines/editor/spline_2d_resource_inspector.h"
#include "godot/splines/editor/spline_3d_resource_inspector.h"
#include "godot/splines/editor/spline_4d_resource_inspector.h"
#include "godot/splines/ideam_splines_plugin.h"
#include "godot/tasks/domains/splines/data_oriented_splines.h"
#include "godot/tasks/domains/splines/spline_task_host.h"

// --- Tiles (Resources, SceneTree Nodes, 2D-to-3D Projector, Non-Euclidean & Planetary Tilings, DOD Bridge, UI Controls, Inspectors & Plugin) ---
#include "godot/tiles/tiling_resource.h"
#include "godot/tiles/wallpaper_tiling_resource.h"
#include "godot/tiles/composite_tiling_resource.h"
#include "godot/tiles/hyperbolic_poincare_tiling_resource.h"
#include "godot/tiles/spherical_goldberg_polyhedron_resource.h"
#include "godot/tiles/tiling_region_node_2d.h"
#include "godot/tiles/tiling_surface_node_3d.h"
#include "godot/tiles/tile_region_2d_projector_3d.h"
#include "godot/tiles/ui/tile_fundamental_domain_canvas.h"
#include "godot/tiles/ui/tiling_region_preview_canvas.h"
#include "godot/tiles/ui/wallpaper_group_selector_control.h"
#include "godot/tiles/ui/wallpaper_tiling_control.h"
#include "godot/tiles/ui/composite_tiling_control.h"
#include "godot/tiles/editor/tiling_editor_inspector_base.h"
#include "godot/tiles/editor/wallpaper_tiling_resource_inspector.h"
#include "godot/tiles/editor/composite_tiling_resource_inspector.h"
#include "godot/tiles/ideam_tiles_plugin.h"
#include "godot/tasks/domains/tiles/data_oriented_tiling.h"
#include "godot/tasks/domains/tiles/tile_task_host.h"

// --- Narratives (Resources, Helpers, DOD Bridge & Plugin) ---
#include "godot/narratives/narreme.h"
#include "godot/narratives/narremes/character.h"
#include "godot/narratives/narremes/location.h"
#include "godot/narratives/narremes/prop.h"
#include "godot/narratives/narremes/incident.h"
#include "godot/narratives/narremes/plot.h"
#include "godot/narratives/narremes/narrative.h"

#include "godot/narratives/helpers/relationship.h"
#include "godot/narratives/helpers/incident_condition.h"
#include "godot/narratives/helpers/causal_condition.h"
#include "godot/narratives/helpers/gameplay_condition.h"
#include "godot/narratives/helpers/narrative_condition.h"
#include "godot/narratives/helpers/plot_event.h"

#include "godot/narratives/ideam_narratives_plugin.h"
#include "godot/tasks/domains/narratives/data_oriented_narrative.h"
#include "godot/tasks/domains/narratives/narrative_task_host.h"

// --- Minds (Akashic Tome & Reference Catalogue Resources, Nodes, DOD Bridge & Plugin) ---
#include "godot/minds/information_matrix_resource.h"
#include "godot/minds/agent_mind_resource.h"
#include "godot/minds/mind_agent_node.h"
#include "godot/minds/ideam_minds_plugin.h"
#include "godot/tasks/domains/minds/data_oriented_mind.h"
#include "godot/tasks/domains/minds/mind_task_host.h"

// --- Design (3D Styles, Themes, Sockets, Containers & Plugin) ---
#include "godot/design/style_3d_resource.h"
#include "godot/design/socket_3d.h"
#include "godot/design/container_3d.h"
#include "godot/design/ideam_design_plugin.h"

// --- Characters (Parametric Character Blueprints, Apparel, Nodes & Plugin) ---
#include "godot/characters/character_blueprint_resource.h"
#include "godot/characters/ideam_characters_plugin.h"

// --- Procedural Generation, Wave Function Collapse & Quantum Darwinism ---
#include "godot/procedural_generation/procedural_generator_resource.h"
#include "godot/procedural_generation/analysis_rule.h"
#include "godot/procedural_generation/generation_rule.h"
#include "godot/procedural_generation/procedural_pattern.h"
#include "godot/procedural_generation/procedural_patterner.h"
#include "godot/procedural_generation/editor/procedural_generator_inspector_base.h"
#include "godot/procedural_generation/ideam_procedural_generation_plugin.h"
#include "godot/tasks/domains/procedural_generation/data_oriented_procedural_generation.h"
#include "godot/tasks/domains/procedural_generation/procedural_generation_task_host.h"
#include "godot/wave_function_collapse/wfc_rule_tensor_resource.h"
#include "godot/wave_function_collapse/ideam_wave_function_collapse_plugin.h"
#include "godot/tasks/domains/wave_function_collapse/data_oriented_wfc.h"
#include "godot/tasks/domains/wave_function_collapse/wfc_task_host.h"
#include "godot/quantum_darwinism/quantum_darwinism_stabilizer_resource.h"
#include "godot/quantum_darwinism/editor/quantum_darwinism_inspector.h"
#include "godot/quantum_darwinism/ideam_quantum_darwinism_plugin.h"
#include "godot/tasks/domains/quantum_darwinism/data_oriented_quantum_darwinism.h"
#include "godot/tasks/domains/quantum_darwinism/quantum_darwinism_task_host.h"

// --- Linguistics & Communications ---
#include "godot/linguistics/sememe_resource.h"
#include "godot/linguistics/lexicon_resource.h"
#include "godot/linguistics/semantic_prompt_parser.h"
#include "godot/linguistics/editor/linguistics_inspector.h"
#include "godot/linguistics/ideam_linguistics_plugin.h"
#include "godot/tasks/domains/linguistics/data_oriented_linguistics.h"
#include "godot/tasks/domains/linguistics/linguistics_task_host.h"
#include "godot/communications/communication_medium_resource.h"
#include "godot/communications/discourse_resource.h"
#include "godot/communications/transceiver_nodes.h"
#include "godot/communications/editor/communications_inspector.h"
#include "godot/communications/ideam_communications_plugin.h"
#include "godot/tasks/domains/communications/data_oriented_communications.h"
#include "godot/tasks/domains/communications/communications_task_host.h"

// --- Games Plugin & DOD Bridge ---
#include "godot/games/ideam_games_plugin.h"

#include "godot/games/game_hub.h"
#include "godot/games/game.h"

#include "godot/games/game_player_manager.h"
#include "godot/games/game_player.h"
#include "godot/games/game_player_profile.h"

#include "godot/games/game_entity.h"
#include "godot/games/game_entities/game_board.h"
#include "godot/games/game_entities/game_agent.h"
#include "godot/games/game_entities/actions/game_agent_action.h"
#include "godot/games/game_entities/game_piece.h"
#include "godot/games/game_entities/actions/game_piece_action.h"

#include "godot/games/game_entities/actions/game_interaction.h"

#include "godot/games/game_property.h"
#include "godot/games/game_property_modifier.h"

#include "godot/games/scene_transition.h"
#include "godot/tasks/domains/games/data_oriented_game.h"
#include "godot/tasks/domains/games/data_oriented_game_board.h"

// --- Gameplay ---
#include "godot/games/gameplay/gameplay_resource.h"
#include "godot/games/gameplay/gameplay_accessibility_option.h"
#include "godot/games/gameplay/gameplay_audio_option.h"
#include "godot/games/gameplay/gameplay_graphics_option.h"
#include "godot/games/gameplay/gameplay_difficulty.h"
#include "godot/games/gameplay/gameplay_control_map.h"
#include "godot/games/gameplay/gameplay_mode.h"
#include "godot/games/gameplay/gameplay_perspective.h"
#include "godot/games/gameplay/gameplay_rule.h"
#include "godot/games/gameplay/gameplay_style.h"

// --- Games UI ---
#include "godot/games/ui/game_hub_ui.h"
#include "godot/games/ui/game_menu.h"
#include "godot/games/ui/game_options_menu.h"
#include "godot/games/ui/game_pause_menu.h"
#include "godot/games/ui/chapter_select.h"

#include "godot/games/ui/game_agent_action_tool.h"
#include "godot/games/ui/game_agent_action_sequencer.h"

#include "godot/games/ui/accessibility_live_preview_card.h"
#include "godot/games/ui/control_action_rebind_matrix.h"
#include "godot/games/ui/control_stick_deadzone_visualizer.h"
#include "godot/games/ui/cymatic_audio_compass.h"
#include "godot/games/ui/difficulty_profile_radar.h"
#include "godot/games/ui/gameplay_context_hud_indicator.h"
#include "godot/games/ui/game_property_gauge.h"
#include "godot/games/ui/perspective_frustum_visualizer.h"

// --- Games Editors ---
#include "godot/games/editor/game_agent_editor_inspector_plugin.h"
#include "godot/games/editor/game_agent_action_editor_inspector_plugin.h"

#include "godot/games/editor/gameplay_editor_debugger_plugin.h"
#include "godot/games/editor/gameplay_resource_editor_inspector_plugin.h"
#include "godot/games/editor/gameplay_rule_editor_inspector_plugin.h"
#include "godot/games/editor/gameplay_style_cascade_dock.h"

using namespace godot;
using namespace ideam::core;

void initialize_ideam_core_module(ModuleInitializationLevel p_level) {
    // ========================================================================
    // SCENE LEVEL
    // ========================================================================
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        
        GDREGISTER_CLASS(ideam::core::IdeamTaskRegistry);
        GDREGISTER_CLASS(ideam::core::TaskManifest);
        
        // --- Utilities ---
        GDREGISTER_CLASS(ideam::godot_ext::IdeamUndoRedo);
        GDREGISTER_CLASS(ideam::godot_ext::NodeRetargeter);
        GDREGISTER_CLASS(ideam::godot_ext::SignalConnector);

        // --- Controls ---
        GDREGISTER_CLASS(ideam::godot_ext::RuntimeResourcePicker);
        GDREGISTER_CLASS(ideam::godot_ext::RuntimeInspector);
        GDREGISTER_CLASS(ideam::godot_ext::CircularGauge);
        GDREGISTER_CLASS(ideam::godot_ext::HeatmapGrid);
        GDREGISTER_CLASS(ideam::godot_ext::SelectionList);

        GDREGISTER_CLASS(ideam::godot_ext::ThemeRegistry);
        GDREGISTER_CLASS(ideam::godot_ext::ThemeSelector);
        
        // --- Graphs ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphResource);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphNodeResource);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphEdit);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamGraphNode);

        GDREGISTER_CLASS(ideam::godot_ext::GraphComposer);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphGroupResource);
        
        // --- Memory ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphResource);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphNodeResource);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphEdit);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::MemoryGraphNode);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryManagerResource);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferColumnPanel);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferLinkagePanel);
        GDREGISTER_CLASS(ideam::godot_ext::ManagedBufferResource);
        GDREGISTER_CLASS(ideam::godot_ext::ManagedBufferView);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferResource);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferView);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferTooltip);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryPageGrid);
        GDREGISTER_CLASS(ideam::godot_ext::AoSVisualizer);
        GDREGISTER_CLASS(ideam::godot_ext::FlatVisualizer);
        GDREGISTER_CLASS(ideam::godot_ext::RingVisualizer);
        GDREGISTER_CLASS(ideam::godot_ext::SoAVisualizer);
        GDREGISTER_CLASS(ideam::godot_ext::SparseSetVisualizer);
        GDREGISTER_CLASS(ideam::godot_ext::TiledSoAVisualizer);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferSelectionResource);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryBufferSelectionView);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryGrantResource);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryGrantView);

        GDREGISTER_CLASS(ideam::godot_ext::GrantPartResource);
        GDREGISTER_CLASS(ideam::godot_ext::GrantPartView);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryGrantInspector);
        GDREGISTER_CLASS(ideam::godot_ext::MemorySelectionInspector);
        GDREGISTER_CLASS(ideam::godot_ext::GrantRequestWindow);

        GDREGISTER_CLASS(ideam::godot_ext::MemoryProfiler);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryRibbon);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryRibbonButton);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryBlockButton);
        GDREGISTER_CLASS(ideam::godot_ext::BlockGrantBadge);
        
        // --- Tasks ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::TaskResource);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::TaskGraphNode);

        GDREGISTER_CLASS(ideam::godot_ext::TaskGraphResource);
        GDREGISTER_CLASS(ideam::godot_ext::TaskGraphHost);
        GDREGISTER_CLASS(ideam::godot_ext::TaskGraphEdit);
        
        GDREGISTER_CLASS(ideam::godot_ext::MetadataTaskResource);
        GDREGISTER_CLASS(ideam::godot_ext::MetadataTaskGraphNode);
        
        GDREGISTER_CLASS(ideam::godot_ext::QueryTaskResource);
        GDREGISTER_CLASS(ideam::godot_ext::QueryTaskGraphNode);

        GDREGISTER_CLASS(ideam::godot_ext::TransformTaskResource);
        GDREGISTER_CLASS(ideam::godot_ext::TransformTaskGraphNode);

        GDREGISTER_CLASS(ideam::godot_ext::SubGraphTaskResource);
        GDREGISTER_CLASS(ideam::godot_ext::SubGraphTaskGraphNode);

        GDREGISTER_CLASS(ideam::godot_ext::EntryFillTaskResource);
        GDREGISTER_CLASS(ideam::godot_ext::EntryFillTaskGraphNode);

        // --- Geometry (Base Resource, Shared Geometry Resources, 2D-to-3D Projector, DOD Bridge, Canvases & UI Controls) ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::GeometryResource);
        GDREGISTER_CLASS(ideam::godot_ext::Polygon2DResource);
        GDREGISTER_CLASS(ideam::godot_ext::VoronoiDiagram2DResource);
        GDREGISTER_CLASS(ideam::godot_ext::SDFGrid2DResource);
        GDREGISTER_CLASS(ideam::godot_ext::Geometry2DProjector3D);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedGeometry);
        GDREGISTER_CLASS(ideam::godot_ext::GeometryTaskHost);

        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::GeometryCanvas2DBase);
        GDREGISTER_CLASS(ideam::godot_ext::Polygon2DCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::Voronoi2DCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::Voronoi2DControl);
        GDREGISTER_CLASS(ideam::godot_ext::SDFGrid2DCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::SDFGrid2DControl);
        GDREGISTER_CLASS(ideam::godot_ext::GeometryCSGPreviewCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::GeometryCSGBridgeControl);
        GDREGISTER_CLASS(ideam::godot_ext::Polygon2DControl);

        // --- Splines (Resources, SceneTree Nodes, 3D Tools, Tweener, DOD Bridge & Runtime/Editor UI Controls) ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::SplineResource);
        GDREGISTER_CLASS(ideam::godot_ext::Spline1DResource);
        GDREGISTER_CLASS(ideam::godot_ext::Spline2DResource);
        GDREGISTER_CLASS(ideam::godot_ext::Spline3DResource);
        GDREGISTER_CLASS(ideam::godot_ext::Spline4DResource);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedSplines);
        GDREGISTER_CLASS(ideam::godot_ext::SplineTaskHost);

        GDREGISTER_CLASS(ideam::godot_ext::SplineNode);
        GDREGISTER_CLASS(ideam::godot_ext::SplineNode2D);
        GDREGISTER_CLASS(ideam::godot_ext::SplinePolygon2D);
        GDREGISTER_CLASS(ideam::godot_ext::Spline2DProjector3D);
        GDREGISTER_CLASS(ideam::godot_ext::SplineNode3D);
        GDREGISTER_CLASS(ideam::godot_ext::SplineExtrudeMesh3D);
        GDREGISTER_CLASS(ideam::godot_ext::SplineMultiMesh3D);
        GDREGISTER_CLASS(ideam::godot_ext::SplineZone3D);
        GDREGISTER_CLASS(ideam::godot_ext::SplineNode4D);

        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::SplineCanvasBase);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::SplineControlBase);
        GDREGISTER_CLASS(ideam::godot_ext::SplinePointField);
        GDREGISTER_CLASS(ideam::godot_ext::SplineSectionField);

        GDREGISTER_CLASS(ideam::godot_ext::Spline1DEqualizerCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::Spline1DControl);
        GDREGISTER_CLASS(ideam::godot_ext::Spline2DCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::Spline2DControl);
        GDREGISTER_CLASS(ideam::godot_ext::Spline3DControl);
        GDREGISTER_CLASS(ideam::godot_ext::Spline4DWChannelCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::Spline4DControl);
        GDREGISTER_CLASS(ideam::godot_ext::ColorSplineWheelCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::ColorSplineControl);

        GDREGISTER_CLASS(ideam::godot_ext::SplinePathContainer);
        GDREGISTER_CLASS(ideam::godot_ext::SplineHitboxButton);
        GDREGISTER_CLASS(ideam::godot_ext::SplineRegionControl);
        GDREGISTER_CLASS(ideam::godot_ext::SplineEnvelopeSlider);

        GDREGISTER_CLASS(ideam::godot_ext::SplineTweenTrack);
        GDREGISTER_CLASS(ideam::godot_ext::SplineTweenResource);
        GDREGISTER_CLASS(ideam::godot_ext::SplineTweener);
        GDREGISTER_CLASS(ideam::godot_ext::SplineTweenerControl);

        // --- Tiles (Resources, SceneTree Nodes, 2D-to-3D Projector, Non-Euclidean & Planetary Tilings, DOD Bridge & Runtime/Editor UI Controls) ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::TilingResource);
        GDREGISTER_CLASS(ideam::godot_ext::WallpaperTilingResource);
        GDREGISTER_CLASS(ideam::godot_ext::CompositeTilingResource);
        GDREGISTER_CLASS(ideam::godot_ext::HyperbolicPoincareTilingResource);
        GDREGISTER_CLASS(ideam::godot_ext::SphericalGoldbergPolyhedronResource);
        GDREGISTER_CLASS(ideam::godot_ext::TilingRegionNode2D);
        GDREGISTER_CLASS(ideam::godot_ext::TilingSurfaceNode3D);
        GDREGISTER_CLASS(ideam::godot_ext::TileRegion2DProjector3D);
        GDREGISTER_CLASS(ideam::godot_ext::GeodesicPlanetNode3D);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedTiling);
        GDREGISTER_CLASS(ideam::godot_ext::TileTaskHost);

        GDREGISTER_CLASS(ideam::godot_ext::TileFundamentalDomainCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::TilingRegionPreviewCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::WallpaperGroupSelectorControl);
        GDREGISTER_CLASS(ideam::godot_ext::WallpaperTilingControl);
        GDREGISTER_CLASS(ideam::godot_ext::CompositeTilingControl);
        GDREGISTER_CLASS(ideam::godot_ext::PoincareDiskTilingCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::HyperbolicPoincareTilingControl);
        GDREGISTER_CLASS(ideam::godot_ext::SphericalGoldbergPreviewCanvas);
        GDREGISTER_CLASS(ideam::godot_ext::SphericalGoldbergControl);

        // --- Narratives & DOD Bridge ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::Narreme);

        GDREGISTER_CLASS(ideam::godot_ext::Narrative);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedNarrative);
        GDREGISTER_CLASS(ideam::godot_ext::NarrativeTaskHost);
        GDREGISTER_CLASS(ideam::godot_ext::Character);
        GDREGISTER_CLASS(ideam::godot_ext::Location);
        GDREGISTER_CLASS(ideam::godot_ext::Prop);
        GDREGISTER_CLASS(ideam::godot_ext::Incident);
        GDREGISTER_CLASS(ideam::godot_ext::Plot);

        // --- Narrative Helper Classes ---
        GDREGISTER_CLASS(ideam::godot_ext::Relationship);
        GDREGISTER_CLASS(ideam::godot_ext::Incident_Condition);
        GDREGISTER_CLASS(ideam::godot_ext::Causal_Condition);
        GDREGISTER_CLASS(ideam::godot_ext::Gameplay_Condition);
        GDREGISTER_CLASS(ideam::godot_ext::Narrative_Condition);
        GDREGISTER_CLASS(ideam::godot_ext::Plot_Event);

        // --- Minds & DOD Bridge (Section 1.1 & Section 2.2) ---
        GDREGISTER_CLASS(ideam::godot_ext::InformationMatrixResource);
        GDREGISTER_CLASS(ideam::godot_ext::AgentMindResource);
        GDREGISTER_CLASS(ideam::godot_ext::MindAgentNode);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedMind);
        GDREGISTER_CLASS(ideam::godot_ext::MindTaskHost);

        // --- Design (Section 1.1) ---
        GDREGISTER_CLASS(ideam::godot_ext::Style3DResource);
        GDREGISTER_CLASS(ideam::godot_ext::Theme3DResource);
        GDREGISTER_CLASS(ideam::godot_ext::Socket3D);
        GDREGISTER_CLASS(ideam::godot_ext::Container3D);
        GDREGISTER_CLASS(ideam::godot_ext::RadialContainer3D);
        GDREGISTER_CLASS(ideam::godot_ext::SemanticStructure3D);

        // --- Characters (Section 1.1) ---
        GDREGISTER_CLASS(ideam::godot_ext::CharacterBlueprintResource);
        GDREGISTER_CLASS(ideam::godot_ext::CharacterApparelResource);
        GDREGISTER_CLASS(ideam::godot_ext::ParametricCharacterNode3D);

        // --- Procedural Generation, Wave Function Collapse & Quantum Darwinism ---
        GDREGISTER_CLASS(ideam::godot_ext::ProceduralGeneratorResource);
        GDREGISTER_CLASS(ideam::godot_ext::ProceduralGenerationHost);
        GDREGISTER_CLASS(ideam::godot_ext::AnalysisRule);
        GDREGISTER_CLASS(ideam::godot_ext::AnalysisStyle);
        GDREGISTER_CLASS(ideam::godot_ext::GenerationRule);
        GDREGISTER_CLASS(ideam::godot_ext::ProceduralPattern);
        GDREGISTER_CLASS(ideam::godot_ext::ProceduralPatterner);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedProceduralGeneration);
        GDREGISTER_CLASS(ideam::godot_ext::ProceduralGenerationTaskHost);
        GDREGISTER_CLASS(ideam::godot_ext::WFCRuleTensorResource);
        GDREGISTER_CLASS(ideam::godot_ext::WFCSceneAnalyzer);
        GDREGISTER_CLASS(ideam::godot_ext::WFCSolverNode);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedWaveFunctionCollapse);
        GDREGISTER_CLASS(ideam::godot_ext::WaveFunctionCollapseTaskHost);
        GDREGISTER_CLASS(ideam::godot_ext::QuantumDarwinismStabilizerResource);
        GDREGISTER_CLASS(ideam::godot_ext::QuantumDarwinismStabilizerNode);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedQuantumDarwinism);
        GDREGISTER_CLASS(ideam::godot_ext::QuantumDarwinismTaskHost);

        // --- Linguistics & Communications ---
        GDREGISTER_CLASS(ideam::godot_ext::SememeResource);
        GDREGISTER_CLASS(ideam::godot_ext::SemanticTagSetResource);
        GDREGISTER_CLASS(ideam::godot_ext::GrammarRuleResource);
        GDREGISTER_CLASS(ideam::godot_ext::LexiconResource);
        GDREGISTER_CLASS(ideam::godot_ext::LanguageAssetResource);
        GDREGISTER_CLASS(ideam::godot_ext::SemanticPromptParser);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedLinguistics);
        GDREGISTER_CLASS(ideam::godot_ext::LinguisticsTaskHost);
        GDREGISTER_CLASS(ideam::godot_ext::CommunicationMediumResource);
        GDREGISTER_CLASS(ideam::godot_ext::UtteranceResource);
        GDREGISTER_CLASS(ideam::godot_ext::DiscourseResource);
        GDREGISTER_CLASS(ideam::godot_ext::CommunicationTransmitterNode);
        GDREGISTER_CLASS(ideam::godot_ext::CommunicationReceiverNode);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedCommunications);
        GDREGISTER_CLASS(ideam::godot_ext::CommunicationsTaskHost);

        // --- Games & DOD Bridge ---
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::GameEntity);
        
        GDREGISTER_CLASS(ideam::godot_ext::GameHub);
        GDREGISTER_CLASS(ideam::godot_ext::Game);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedGame);
        GDREGISTER_CLASS(ideam::godot_ext::GamePlayer);
        GDREGISTER_CLASS(ideam::godot_ext::GamePlayerProfile);
        GDREGISTER_CLASS(ideam::godot_ext::GamePlayerManager);
        GDREGISTER_CLASS(ideam::godot_ext::GameBoard);
        GDREGISTER_CLASS(ideam::godot_ext::DataOrientedGameBoard);
        GDREGISTER_CLASS(ideam::godot_ext::GameAgent);
        GDREGISTER_CLASS(ideam::godot_ext::GameAgentAction);
        GDREGISTER_CLASS(ideam::godot_ext::GamePiece);
        GDREGISTER_CLASS(ideam::godot_ext::GamePieceAction);
        GDREGISTER_CLASS(ideam::godot_ext::GameInteraction);

        GDREGISTER_CLASS(ideam::godot_ext::GameProperty);
        GDREGISTER_CLASS(ideam::godot_ext::GamePropertyModifier);
        GDREGISTER_CLASS(ideam::godot_ext::SceneTransition);
        
        // --- Gameplay ---
        GDREGISTER_CLASS(ideam::godot_ext::GameplayResource);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayAccessibilityOption);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayAudioOption);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayGraphicsOption);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayDifficulty);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayControlMap);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayMode);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayPerspective);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayRule);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayStyle);

        // --- Games UI ---
        GDREGISTER_CLASS(ideam::godot_ext::GameHubUI);
        GDREGISTER_CLASS(ideam::godot_ext::GameMenu);
        GDREGISTER_CLASS(ideam::godot_ext::GameOptionsMenu);
        GDREGISTER_CLASS(ideam::godot_ext::GamePauseMenu);
        GDREGISTER_CLASS(ideam::godot_ext::ChapterSelect);

        GDREGISTER_CLASS(ideam::godot_ext::GameAgentActionTool);
        GDREGISTER_CLASS(ideam::godot_ext::GameAgentActionSequencer);

        GDREGISTER_CLASS(ideam::godot_ext::AccessibilityLivePreviewCard);
        GDREGISTER_CLASS(ideam::godot_ext::ControlActionRebindMatrix);
        GDREGISTER_CLASS(ideam::godot_ext::ControlStickDeadzoneVisualizer);
        GDREGISTER_CLASS(ideam::godot_ext::CymaticAudioCompass);
        GDREGISTER_CLASS(ideam::godot_ext::DifficultyProfileRadar);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayContextHUDIndicator);
        GDREGISTER_CLASS(ideam::godot_ext::GamePropertyGauge);
        GDREGISTER_CLASS(ideam::godot_ext::PerspectiveFrustumVisualizer);

        // --- Native Task & Domain Pack Registration ---
        IdeamTaskRegistry::init();
        GamesTaskDomainPack::register_domain();
        GeometryTaskDomainPack::register_domain();
        MindsTaskDomainPack::register_domain();
        NarrativesTaskDomainPack::register_domain();
        SplinesTaskDomainPack::register_domain();
        TilesTaskDomainPack::register_domain();
        ProceduralGenerationTaskDomainPack::register_domain();
        WaveFunctionCollapseTaskDomainPack::register_domain();
        QuantumDarwinismTaskDomainPack::register_domain();
        LinguisticsTaskDomainPack::register_domain();
        CommunicationsTaskDomainPack::register_domain();
    }

    // ========================================================================
    // EDITOR LEVEL
    // ========================================================================
    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        
        // Ecosystem Base Classes - Registered as Abstract to expose API but block direct instantiation
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamEditorPlugin);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::IdeamEditorInspectorPlugin);
        
        // Graph tooling
        GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamGraphsPlugin);

        // Memory tooling
        GDREGISTER_CLASS(ideam::godot_ext::MemoryGraphInspector);
        GDREGISTER_CLASS(ideam::godot_ext::MemoryManagerInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamMemoryPlugin);

        // Tasks tooling
        GDREGISTER_CLASS(ideam::godot_ext::TaskGraphInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamTasksPlugin);

        // Geometry tooling (Base Inspector must be registered before Splines/Tiles Inspectors)
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::GeometryEditorInspectorBase);
        GDREGISTER_CLASS(ideam::godot_ext::Polygon2DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::VoronoiDiagram2DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::SDFGrid2DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamGeometryPlugin);

        // Splines tooling
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::SplineEditorInspectorBase);
        GDREGISTER_CLASS(ideam::godot_ext::Spline1DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::Spline2DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::Spline3DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::Spline4DResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::SplineTweenResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::Spline3DGizmo);
        GDREGISTER_CLASS(ideam::godot_ext::Spline4DGizmo);
        GDREGISTER_CLASS(ideam::godot_ext::SplineExtrudeGizmo);
        GDREGISTER_CLASS(ideam::godot_ext::SplineScatterGizmo);
        GDREGISTER_CLASS(ideam::godot_ext::SplineCorridorGizmo);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamSplinesPlugin);

        // Tiles tooling
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::TilingEditorInspectorBase);
        GDREGISTER_CLASS(ideam::godot_ext::WallpaperTilingResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::CompositeTilingResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::HyperbolicPoincareTilingResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::SphericalGoldbergResourceInspector);
        GDREGISTER_CLASS(ideam::godot_ext::GeodesicPlanetGizmo);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamTilesPlugin);

        // Narratives tooling
        GDREGISTER_CLASS(ideam::godot_ext::IdeamNarrativesPlugin);

        // Minds, Design, Characters, Procedural Generation & WFC tooling (Section 1.1 & 2.2)
        GDREGISTER_CLASS(ideam::godot_ext::IdeamMindsPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamDesignPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamCharactersPlugin);
        GDREGISTER_ABSTRACT_CLASS(ideam::godot_ext::ProceduralGeneratorInspectorBase);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamProceduralGenerationPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::WFCRuleTensorInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamWaveFunctionCollapsePlugin);
        GDREGISTER_CLASS(ideam::godot_ext::QuantumDarwinismInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamQuantumDarwinismPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::LinguisticsInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamLinguisticsPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::CommunicationsInspector);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamCommunicationsPlugin);

        // --- Games Editors ---
        GDREGISTER_CLASS(ideam::godot_ext::GameAgentEditorInspectorPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::GameAgentActionEditorInspectorPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayEditorDebuggerPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayResourceEditorInspectorPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayRuleEditorInspectorPlugin);
        GDREGISTER_CLASS(ideam::godot_ext::GameplayStyleCascadeDock);
        GDREGISTER_CLASS(ideam::godot_ext::IdeamGamesPlugin);
    }
}

void uninitialize_ideam_core_module(ModuleInitializationLevel p_level) {
    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        // Flush the static factories and dictionaries to prevent Godot memory leak warnings on exit
        IdeamTaskRegistry::cleanup();
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        
    }
}

extern "C" {
// Initialization.
GDExtensionBool GDE_EXPORT ideam_core_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_ideam_core_module);
    init_obj.register_terminator(uninitialize_ideam_core_module);
    
    // We set minimum level to SCENE, but our initialization function handles both SCENE and EDITOR
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}