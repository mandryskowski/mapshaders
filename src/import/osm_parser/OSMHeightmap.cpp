#include "OSMHeightmap.h"

#include <godot_cpp/core/class_db.hpp>

#include "../elevation/ElevationParser.h"

using namespace godot;

ElevationHeightmap::ElevationHeightmap(godot::Ref<ElevationGrid> elevation_grid)
    : elevation_grid(elevation_grid) {}

double ElevationHeightmap::getElevation(const GeoCoords& coords) const {
  return elevation_grid->bilinearInterpolation(coords);
}

void ElevationHeightmap::_bind_methods() {
  godot::ClassDB::bind_method(D_METHOD("get_elevation_vec", "coords"),
                              &ElevationHeightmap::get_elevation_vec);
  godot::ClassDB::bind_method(D_METHOD("set_elevation_grid", "elevation_grid"),
                              &ElevationHeightmap::set_elevation_grid);
  godot::ClassDB::bind_method(D_METHOD("get_elevation_grid"),
                              &ElevationHeightmap::get_elevation_grid);

  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "elevation_grid"),
               "set_elevation_grid", "get_elevation_grid");
}