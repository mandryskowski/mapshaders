#ifndef WORLDSHADERS_COASTLINE_3D_COASTLINE_H
#define WORLDSHADERS_COASTLINE_3D_COASTLINE_H
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/material.hpp>
#include "../../../src/import/GeoMap.h"

class Coastline : public godot::Node {
    GDCLASS(Coastline, godot::Node)

public:
    void import_polygons_geo(godot::Array polygons, godot::Ref<GeoMap> geomap);

    void set_material(godot::Ref<godot::Material> mat) {
        material = mat;
    }
    godot::Ref<godot::Material> get_material() const {
        return material;
    }

    void set_add_seabed(bool add) {
        add_seabed = add;
    }
    bool get_add_seabed() const {
        return add_seabed;
    }
protected:
    static void _bind_methods();
private:
    godot::Ref<godot::Material> material;
    bool add_seabed;
};
#endif // WORLDSHADERS_COASTLINE_3D_COASTLINE_H