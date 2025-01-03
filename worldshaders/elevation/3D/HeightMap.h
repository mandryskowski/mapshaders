#ifndef WORLDSHADERS_HEIGHTMAP_H
#define WORLDSHADERS_HEIGHTMAP_H
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/variant/array.hpp>

#include "../../../src/import/GeoMap.h"
#include "../../../src/import/elevation/ElevationParser.h"

class HeightMap : public godot::Node {
  GDCLASS(HeightMap, godot::Node)

 public:
  void import_polygons_geo(godot::Array polygons, GeoMap* geomap);
  void import_grid(ElevationGrid*,
                   godot::Ref<godot::StreamPeerBuffer> tile_bytes);
  void load_tile(godot::Ref<godot::FileAccess> fa, GeoMap* geomap);

  void set_flat_shading(bool flat) { flat_shading = flat; }
  bool get_flat_shading() const { return flat_shading; }
  void set_cliffs(bool cliffs) { this->cliffs = cliffs; }
  bool get_cliffs() const { return cliffs; }

 protected:
  static void _bind_methods();

 private:
  godot::Array coastline_polygons;
  GeoMap* coastline_geomap;
  bool flat_shading;
  bool cliffs;
};

#endif  // WORLDSHADERS_HEIGHTMAP_H