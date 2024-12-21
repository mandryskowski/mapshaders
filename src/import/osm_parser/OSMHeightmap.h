/* Abstraction to decouple OSM from Elevation */
#ifndef OSM_HEIGHTMAP_H
#define OSM_HEIGHTMAP_H
#include <godot_cpp/classes/ref.hpp>

#include "../GeoMap.h"
#include "../elevation/ElevationParser.h"


class OSMHeightmap : public godot::RefCounted {
  GDCLASS(OSMHeightmap, godot::RefCounted);

 public:
  virtual double getElevation(const GeoCoords&) const = 0;

  static void _bind_methods() {}
};

class ElevationHeightmap : public OSMHeightmap {
  GDCLASS(ElevationHeightmap, OSMHeightmap);

 public:
  ElevationHeightmap() {}
  ElevationHeightmap(godot::Ref<ElevationGrid> elevation_grid);
  virtual double getElevation(const GeoCoords&) const override;
  double get_elevation_vec(const godot::Vector2& coords) {
    return getElevation(GeoCoords::from_vector2_representation(coords));
  }

  void set_elevation_grid(godot::Ref<ElevationGrid> elevation_grid) {
    this->elevation_grid = elevation_grid;
  }
  godot::Ref<ElevationGrid> get_elevation_grid() const {
    return elevation_grid;
  }

 protected:
  static void _bind_methods();

 private:
  godot::Ref<ElevationGrid> elevation_grid;
};

#endif  // OSM_HEIGHTMAP_H