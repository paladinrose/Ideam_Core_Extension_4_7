#pragma once

/**
 * container_3d.h
 * 3D layout containers (`Container3D`, `RadialContainer3D`) and `SemanticStructure3D`
 * (`Surface -> Wall/Floor/Seat -> Chair`, `Aperture -> Door/Window`) in `src/godot/design/`.
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include "style_3d_resource.h"

namespace ideam::godot_ext {

class Container3D : public godot::Node3D {
    GDCLASS(Container3D, godot::Node3D)

private:
    godot::Vector3 cell_spacing_ = godot::Vector3(1.0f, 1.0f, 1.0f);
    bool auto_refit_children_    = true;

protected:
    static void _bind_methods();

public:
    Container3D() = default;
    ~Container3D() override = default;

    void set_cell_spacing(const godot::Vector3 &p_spacing);
    [[nodiscard]] godot::Vector3 get_cell_spacing() const { return cell_spacing_; }

    void set_auto_refit_children(bool p_auto);
    [[nodiscard]] bool get_auto_refit_children() const { return auto_refit_children_; }

    virtual void refit_children_3d();
};

class RadialContainer3D : public Container3D {
    GDCLASS(RadialContainer3D, Container3D)

private:
    real_t radius_ = static_cast<real_t>(3.0);

protected:
    static void _bind_methods();

public:
    RadialContainer3D() = default;
    ~RadialContainer3D() override = default;

    void set_radius(real_t p_radius);
    [[nodiscard]] real_t get_radius() const { return radius_; }

    void refit_children_3d() override;
};

class SemanticStructure3D : public godot::Node3D {
    GDCLASS(SemanticStructure3D, godot::Node3D)

private:
    uint32_t semantic_role_id_ = core::SEMANTIC_ROLE_SURFACE_WALL;
    godot::Ref<Theme3DResource> active_theme_3d_;

protected:
    static void _bind_methods();

public:
    SemanticStructure3D() = default;
    ~SemanticStructure3D() override = default;

    void set_semantic_role_id(int p_role_id);
    [[nodiscard]] int get_semantic_role_id() const { return static_cast<int>(semantic_role_id_); }

    void set_active_theme_3d(const godot::Ref<Theme3DResource> &p_theme);
    [[nodiscard]] godot::Ref<Theme3DResource> get_active_theme_3d() const { return active_theme_3d_; }
};

} // namespace ideam::godot_ext
