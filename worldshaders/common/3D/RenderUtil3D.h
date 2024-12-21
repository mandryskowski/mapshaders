#ifndef WORLDSHADERS_COMMON_3D_RENDERUTIL3D_H
#define WORLDSHADERS_COMMON_3D_RENDERUTIL3D_H
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <vector>

#include "../../../src/import/GeoMap.h"


class RenderUtil3D {
 public:
  static godot::PackedVector3Array geo_polygon_to_world(
      const std::vector<GeoCoords>& coords, GeoMap* geomap);
  static godot::PackedVector3Array geo_polygon_to_world_up(
      const std::vector<GeoCoords>& coords, GeoMap* geomap);
  static godot::Vector3 get_centroid(const godot::PackedVector3Array& verts);
  static bool enforce_winding(godot::PackedVector3Array& verts,
                              bool clockwise = true);
  static godot::Array polygon(const godot::PackedVector3Array& verts,
                              double height = 0.0);
  static godot::Node* achild(godot::Node* parent, godot::Node* child,
                             godot::String name, bool deferred = false);
  static godot::MeshInstance3D* area_poly(
      godot::Node* parent, godot::String name, godot::Array arrays,
      godot::Color color = godot::Color(1, 1, 1, 1), bool deferred = false);
  static godot::Array get_array_mesh_arrays(godot::PackedInt32Array attributes);
  static godot::Array polygon_triangles(godot::PackedVector3Array triangles,
                                        double height,
                                        godot::PackedVector3Array* normals);
};

#endif  // WORLDSHADERS_COMMON_3D_RENDERUTIL3D_H