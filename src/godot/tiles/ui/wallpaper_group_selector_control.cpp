#include "wallpaper_group_selector_control.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable.hpp>

namespace ideam::godot_ext {

void WallpaperGroupSelectorControl::_bind_methods() {
    using namespace godot;

    ADD_SIGNAL(MethodInfo(
        "wallpaper_group_selected",
        PropertyInfo(Variant::INT, "wallpaper_group")
    ));
    ADD_SIGNAL(MethodInfo(
        "tactile_ih_type_selected",
        PropertyInfo(Variant::INT, "ih_type")
    ));

    ClassDB::bind_method(D_METHOD("set_wallpaper_tiling", "tiling"), &WallpaperGroupSelectorControl::set_wallpaper_tiling);
    ClassDB::bind_method(D_METHOD("get_wallpaper_tiling"), &WallpaperGroupSelectorControl::get_wallpaper_tiling);
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "wallpaper_tiling", PROPERTY_HINT_RESOURCE_TYPE, "WallpaperTilingResource"),
        "set_wallpaper_tiling",
        "get_wallpaper_tiling"
    );

    ClassDB::bind_method(D_METHOD("set_lattice_family_filter", "family"), &WallpaperGroupSelectorControl::set_lattice_family_filter);
    ClassDB::bind_method(D_METHOD("get_lattice_family_filter"), &WallpaperGroupSelectorControl::get_lattice_family_filter);

    ClassDB::bind_method(D_METHOD("refresh_ui"), &WallpaperGroupSelectorControl::refresh_ui);

    ClassDB::bind_method(D_METHOD("_on_resource_changed"), &WallpaperGroupSelectorControl::_on_resource_changed);
    ClassDB::bind_method(D_METHOD("_on_authoring_mode_selected", "idx"), &WallpaperGroupSelectorControl::_on_authoring_mode_selected);
    ClassDB::bind_method(D_METHOD("_on_lattice_filter_selected", "idx"), &WallpaperGroupSelectorControl::_on_lattice_filter_selected);
    ClassDB::bind_method(D_METHOD("_on_shape_preset_selected", "idx"), &WallpaperGroupSelectorControl::_on_shape_preset_selected);
    ClassDB::bind_method(D_METHOD("_on_ih_type_selected", "idx"), &WallpaperGroupSelectorControl::_on_ih_type_selected);
    ClassDB::bind_method(D_METHOD("_on_group_button_pressed", "group_idx"), &WallpaperGroupSelectorControl::_on_group_button_pressed);
}

WallpaperGroupSelectorControl::WallpaperGroupSelectorControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    _build_ui();
}

WallpaperGroupSelectorControl::~WallpaperGroupSelectorControl() {
    if (wallpaper_tiling_.is_valid()) {
        const godot::Callable cb(this, "_on_resource_changed");
        if (wallpaper_tiling_->is_connected("changed", cb)) {
            wallpaper_tiling_->disconnect("changed", cb);
        }
    }
}

TilingLatticeFamily WallpaperGroupSelectorControl::_lattice_for_group(WallpaperGroupType p_group) noexcept {
    switch (p_group) {
        case WALLPAPER_P1:
        case WALLPAPER_P2:
            return LATTICE_OBLIQUE;
        case WALLPAPER_PM:
        case WALLPAPER_PG:
        case WALLPAPER_PMM:
        case WALLPAPER_PMG:
        case WALLPAPER_PGG:
            return LATTICE_RECTANGULAR;
        case WALLPAPER_CM:
        case WALLPAPER_CMM:
            return LATTICE_RHOMBIC;
        case WALLPAPER_P4:
        case WALLPAPER_P4M:
        case WALLPAPER_P4G:
            return LATTICE_SQUARE;
        case WALLPAPER_P3:
        case WALLPAPER_P3M1:
        case WALLPAPER_P31M:
        case WALLPAPER_P6:
        case WALLPAPER_P6M:
        default:
            return LATTICE_HEXAGONAL;
    }
}

const char *WallpaperGroupSelectorControl::_group_short_label(WallpaperGroupType p_group) noexcept {
    static constexpr std::array<const char *, 17> LABELS = {
        "p1", "p2", "pm", "pg", "cm", "pmm", "pmg", "pgg", "cmm",
        "p4", "p4m", "p4g", "p3", "p3m1", "p31m", "p6", "p6m"
    };
    const int idx = static_cast<int>(p_group);
    return (idx >= 0 && idx < 17) ? LABELS[static_cast<size_t>(idx)] : "p1";
}

const char *WallpaperGroupSelectorControl::_group_orbifold_label(WallpaperGroupType p_group) noexcept {
    static constexpr std::array<const char *, 17> ORBIFOLDS = {
        "o", "2222", "**", "xx", "*x", "*2222", "22*", "22x", "2*22",
        "442", "*442", "4*2", "333", "*333", "3*3", "632", "*632"
    };
    const int idx = static_cast<int>(p_group);
    return (idx >= 0 && idx < 17) ? ORBIFOLDS[static_cast<size_t>(idx)] : "o";
}

void WallpaperGroupSelectorControl::_build_ui() {
    using namespace godot;

    // Row 1: Authoring Mode + Lattice Family Filter
    HBoxContainer *top_row = memnew(HBoxContainer);
    top_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(top_row);

    Label *mode_lbl = memnew(Label);
    mode_lbl->set_text("Mode:");
    top_row->add_child(mode_lbl);

    authoring_mode_opt_ = memnew(OptionButton);
    authoring_mode_opt_->add_item("17 Wallpaper Groups", TILING_MODE_WALLPAPER_17);
    authoring_mode_opt_->add_item("81 Isohedral Tactile", TILING_MODE_ISOHEDRAL_81);
    authoring_mode_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    authoring_mode_opt_->connect("item_selected", Callable(this, "_on_authoring_mode_selected"));
    top_row->add_child(authoring_mode_opt_);

    Label *lat_lbl = memnew(Label);
    lat_lbl->set_text("Lattice:");
    top_row->add_child(lat_lbl);

    lattice_filter_opt_ = memnew(OptionButton);
    lattice_filter_opt_->add_item("All Families (17)", -1);
    lattice_filter_opt_->add_item("Oblique (p1, p2)", LATTICE_OBLIQUE);
    lattice_filter_opt_->add_item("Rectangular (pm..pgg)", LATTICE_RECTANGULAR);
    lattice_filter_opt_->add_item("Rhombic (cm, cmm)", LATTICE_RHOMBIC);
    lattice_filter_opt_->add_item("Square (p4..p4g)", LATTICE_SQUARE);
    lattice_filter_opt_->add_item("Hexagonal (p3..p6m)", LATTICE_HEXAGONAL);
    lattice_filter_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    lattice_filter_opt_->connect("item_selected", Callable(this, "_on_lattice_filter_selected"));
    top_row->add_child(lattice_filter_opt_);

    // Row 2: 17 Wallpaper Group Button Matrix (6 columns)
    group_grid_ = memnew(GridContainer);
    group_grid_->set_columns(6);
    group_grid_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(group_grid_);

    for (int g = 0; g < 17; ++g) {
        const auto grp = static_cast<WallpaperGroupType>(g);
        Button *btn    = memnew(Button);
        btn->set_toggle_mode(true);
        btn->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        btn->set_text(String(_group_short_label(grp)) + " (" + String(_group_orbifold_label(grp)) + ")");
        btn->connect("pressed", Callable(this, "_on_group_button_pressed").bind(g));
        group_grid_->add_child(btn);
        group_buttons_[static_cast<size_t>(g)] = btn;
    }

    // Row 3: Tactile IH Type Dropdown + Shape Preset Dropdown
    HBoxContainer *sub_row = memnew(HBoxContainer);
    sub_row->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(sub_row);

    Label *ih_lbl = memnew(Label);
    ih_lbl->set_text("Isohedral (IH):");
    sub_row->add_child(ih_lbl);

    ih_type_opt_ = memnew(OptionButton);
    ih_type_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    ih_type_opt_->connect("item_selected", Callable(this, "_on_ih_type_selected"));
    sub_row->add_child(ih_type_opt_);

    Label *preset_lbl = memnew(Label);
    preset_lbl->set_text("Shape Preset:");
    sub_row->add_child(preset_lbl);

    shape_preset_opt_ = memnew(OptionButton);
    shape_preset_opt_->add_item("Tactile Default", SHAPE_PRESET_TACTILE_DEFAULT);
    shape_preset_opt_->add_item("Quad", SHAPE_PRESET_QUAD);
    shape_preset_opt_->add_item("Brick Hex", SHAPE_PRESET_BRICK_HEX);
    shape_preset_opt_->add_item("Right Triangle", SHAPE_PRESET_RIGHT_TRIANGLE);
    shape_preset_opt_->add_item("Equilateral Triangle", SHAPE_PRESET_EQUILATERAL_TRIANGLE);
    shape_preset_opt_->add_item("Flat Triangle", SHAPE_PRESET_FLAT_TRIANGLE);
    shape_preset_opt_->add_item("Tall Triangle", SHAPE_PRESET_TALL_TRIANGLE);
    shape_preset_opt_->add_item("Rhombus 60°", SHAPE_PRESET_RHOMBUS_60);
    shape_preset_opt_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    shape_preset_opt_->connect("item_selected", Callable(this, "_on_shape_preset_selected"));
    sub_row->add_child(shape_preset_opt_);

    // Row 4: Live Crystallographic Telemetry Summary Line
    symmetry_badge_label_ = memnew(Label);
    symmetry_badge_label_->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    add_child(symmetry_badge_label_);

    _rebuild_ih_dropdown();
}

void WallpaperGroupSelectorControl::set_wallpaper_tiling(const godot::Ref<WallpaperTilingResource> &p_tiling) {
    if (wallpaper_tiling_ == p_tiling) return;

    const godot::Callable cb(this, "_on_resource_changed");
    if (wallpaper_tiling_.is_valid() && wallpaper_tiling_->is_connected("changed", cb)) {
        wallpaper_tiling_->disconnect("changed", cb);
    }

    wallpaper_tiling_ = p_tiling;

    if (wallpaper_tiling_.is_valid() && !wallpaper_tiling_->is_connected("changed", cb)) {
        wallpaper_tiling_->connect("changed", cb);
    }

    refresh_ui();
}

void WallpaperGroupSelectorControl::set_lattice_family_filter(int p_family) {
    lattice_family_filter_ = p_family;
    _update_group_button_states();
}

void WallpaperGroupSelectorControl::_rebuild_ih_dropdown() {
    if (!ih_type_opt_) return;
    ih_type_opt_->clear();

    godot::PackedInt32Array ih_list;
    if (wallpaper_tiling_.is_valid() &&
        wallpaper_tiling_->get_authoring_mode() == TILING_MODE_WALLPAPER_17) {
        ih_list = WallpaperTilingResource::get_ih_types_for_group(wallpaper_tiling_->get_wallpaper_group());
    } else {
        ih_list = WallpaperTilingResource::get_all_valid_ih_types();
    }

    const int active_ih = wallpaper_tiling_.is_valid() ? wallpaper_tiling_->get_tactile_ih_type() : 1;
    int selected_idx    = 0;

    for (int i = 0; i < ih_list.size(); ++i) {
        const int ih = ih_list[i];
        const godot::String label = "IH" + godot::String::num_int64(ih).pad_zeros(2);
        ih_type_opt_->add_item(label, ih);
        if (ih == active_ih) {
            selected_idx = i;
        }
    }

    if (ih_type_opt_->get_item_count() > 0) {
        ih_type_opt_->select(selected_idx);
    }
}

void WallpaperGroupSelectorControl::_update_group_button_states() {
    const int active_grp = wallpaper_tiling_.is_valid()
        ? static_cast<int>(wallpaper_tiling_->get_wallpaper_group())
        : 0;

    for (int g = 0; g < 17; ++g) {
        godot::Button *btn = group_buttons_[static_cast<size_t>(g)];
        if (!btn) continue;
        const auto grp         = static_cast<WallpaperGroupType>(g);
        const auto family      = _lattice_for_group(grp);
        const bool matches_flt = (lattice_family_filter_ < 0) || (static_cast<int>(family) == lattice_family_filter_);

        btn->set_visible(matches_flt);
        btn->set_pressed_no_signal(g == active_grp);
    }
}

void WallpaperGroupSelectorControl::refresh_ui() {
    if (is_updating_ui_) return;
    is_updating_ui_ = true;

    if (wallpaper_tiling_.is_valid()) {
        const int mode_idx = authoring_mode_opt_->get_item_index(static_cast<int>(wallpaper_tiling_->get_authoring_mode()));
        if (mode_idx >= 0) authoring_mode_opt_->select(mode_idx);

        const int preset_idx = shape_preset_opt_->get_item_index(static_cast<int>(wallpaper_tiling_->get_shape_preset()));
        if (preset_idx >= 0) shape_preset_opt_->select(preset_idx);

        _rebuild_ih_dropdown();
        _update_group_button_states();

        const godot::String summary =
            "IUCr: " + wallpaper_tiling_->get_iucr_symbol() +
            "  |  Orbifold: " + wallpaper_tiling_->get_orbifold_symbol() +
            "  |  Rot Order: " + godot::String::num_int64(wallpaper_tiling_->get_rotational_order()) + "-fold" +
            "  |  Mirror: " + (wallpaper_tiling_->has_reflection() ? "Yes" : "No") +
            "  |  Glide: " + (wallpaper_tiling_->has_glide_reflection() ? "Yes" : "No");
        symmetry_badge_label_->set_text(summary);
    } else {
        symmetry_badge_label_->set_text("No WallpaperTilingResource bound.");
    }

    is_updating_ui_ = false;
}

void WallpaperGroupSelectorControl::_on_resource_changed() {
    refresh_ui();
}

void WallpaperGroupSelectorControl::_on_authoring_mode_selected(int p_idx) {
    if (is_updating_ui_ || wallpaper_tiling_.is_null()) return;
    const int mode_id = authoring_mode_opt_->get_item_id(p_idx);
    wallpaper_tiling_->set_authoring_mode(static_cast<TilingAuthoringModeType>(mode_id));
    refresh_ui();
}

void WallpaperGroupSelectorControl::_on_lattice_filter_selected(int p_idx) {
    if (is_updating_ui_) return;
    set_lattice_family_filter(lattice_filter_opt_->get_item_id(p_idx));
}

void WallpaperGroupSelectorControl::_on_shape_preset_selected(int p_idx) {
    if (is_updating_ui_ || wallpaper_tiling_.is_null()) return;
    const int preset_id = shape_preset_opt_->get_item_id(p_idx);
    wallpaper_tiling_->commit_set_shape_preset(static_cast<TilingShapePresetType>(preset_id));
    refresh_ui();
}

void WallpaperGroupSelectorControl::_on_ih_type_selected(int p_idx) {
    if (is_updating_ui_ || wallpaper_tiling_.is_null()) return;
    const int ih_type = ih_type_opt_->get_item_id(p_idx);
    wallpaper_tiling_->commit_set_tactile_ih_type(ih_type);
    emit_signal("tactile_ih_type_selected", ih_type);
    refresh_ui();
}

void WallpaperGroupSelectorControl::_on_group_button_pressed(int p_group_idx) {
    if (is_updating_ui_ || wallpaper_tiling_.is_null()) return;
    const auto grp = static_cast<WallpaperGroupType>(p_group_idx);
    wallpaper_tiling_->commit_set_wallpaper_group(grp);
    emit_signal("wallpaper_group_selected", p_group_idx);
    refresh_ui();
}

} // namespace ideam::godot_ext
