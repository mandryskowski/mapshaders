#include "Barriers.h"

#include "../../../src/import/SGImport.h"
#include "../../../src/util/PolyUtil.h"
#include "../../common/3D/RenderUtil3D.h"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/packed_float64_array.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"

using namespace godot;

void Barriers::import_begin() {
  tile_info = Dictionary();
  node_pos = HashMap<int64_t, GeoCoords>();
  heightmap = nullptr;
}

void Barriers::import_node(OSMNodeGD* node,
                           godot::Ref<godot::StreamPeerBuffer> fa) {
  node_pos[node->get_id()] = GeoCoords(Longitude::radians(node->get_lon_rad()),
                                       Latitude::radians(node->get_lat_rad()));
  if (!heightmap) heightmap = node->get_heightmap();
}

void Barriers::import_way(OSMWayGD* way,
                          godot::Ref<godot::StreamPeerBuffer> fa) {
  if (!way->has_tag("barrier")) return;

  PackedFloat64Array path_lon, path_lat, path_elev;
  auto nodes = way->get_nodes();
  for (int i = 0; i < nodes.size(); i++) {
    if (!node_pos.has(nodes[i])) continue;
    auto coords = node_pos[nodes[i]];
    path_lon.push_back(coords.lon.get_radians());
    path_lat.push_back(coords.lat.get_radians());
    if (heightmap) path_elev.push_back(heightmap->getElevation(coords));
  }

  Dictionary d;
  d["path_lon"] = path_lon;
  d["path_lat"] = path_lat;
  d["path_elev"] = path_elev;
  d["height"] = 1.0;

  if (!tile_info.has(fa)) {
    TypedArray<Dictionary> ways;
    tile_info[fa] = ways;
  }
  static_cast<TypedArray<Dictionary>>(tile_info[fa]).append(d);
}

void Barriers::import_finished() {
  for (int i = 0; i < tile_info.size(); i++) {
    godot::Ref<godot::StreamPeerBuffer> fa =
        static_cast<godot::Ref<godot::StreamPeerBuffer>>(tile_info.keys()[i]);
    fa->put_var(tile_info[fa]);
  }
}

void Barriers::load_tile(godot::Ref<godot::FileAccess> fa, GeoMap* geomap) {
  TypedArray<Dictionary> ways =
      static_cast<TypedArray<Dictionary>>(fa->get_var());
  Array arrays = RenderUtil3D::get_array_mesh_arrays(
      {Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL});

  for (int i = 0; i < ways.size(); i++) {
    Dictionary d = ways[i];

    if (!d.has("path_lon")) continue;

    PackedFloat64Array path_lon = d["path_lon"];
    PackedFloat64Array path_lat = d["path_lat"];
    PackedFloat64Array path_elev = d["path_elev"];

    PackedVector3Array path;

    for (int j = 0; j < path_lon.size(); j++) {
      auto coords = GeoCoords(Longitude::radians(path_lon[j]),
                              Latitude::radians(path_lat[j]));
      Vector3 v = geomap->geo_to_world(coords);
      if (!path_elev.is_empty())
        v += geomap->geo_to_world_up(coords) * path_elev[j];
      path.push_back(v);
    }

    for (int j = 0; j < path.size() - 1; j++) {
      Vector3 v_this = path[j];
      Vector3 v_next = path[j + 1];

      Vector3 up = geomap->geo_to_world_up(GeoCoords(
          Longitude::radians(path_lon[j]), Latitude::radians(path_lat[j])));
      Vector3 normal = up.cross(v_next - v_this).normalized();

      double height = d["height"];

      arrays[Mesh::ARRAY_VERTEX].call("push_back", v_this);
      arrays[Mesh::ARRAY_VERTEX].call("push_back", v_next);
      arrays[Mesh::ARRAY_VERTEX].call("push_back", v_next + up * height);

      arrays[Mesh::ARRAY_VERTEX].call("push_back", v_this);
      arrays[Mesh::ARRAY_VERTEX].call("push_back", v_next + up * height);
      arrays[Mesh::ARRAY_VERTEX].call("push_back", v_this + up * height);

      for (int k = 0; k < 6; k++) {
        arrays[Mesh::ARRAY_NORMAL].call("push_back", normal);
      }
    }
  }

  RenderUtil3D::area_poly(this, "Barrier", arrays);
}

void Barriers::_bind_methods() {
  ClassDB::bind_method(D_METHOD("import_begin"), &Barriers::import_begin);
  ClassDB::bind_method(D_METHOD("import_node", "d", "fa"),
                       &Barriers::import_node);
  ClassDB::bind_method(D_METHOD("import_way", "d", "fa"),
                       &Barriers::import_way);

  ClassDB::bind_method(D_METHOD("import_finished"), &Barriers::import_finished);
  ClassDB::bind_method(D_METHOD("load_tile", "fa", "geomap"),
                       &Barriers::load_tile);

  ClassDB::bind_method(D_METHOD("set_barriers_as_separate_nodes", "value"),
                       &Barriers::set_barriers_as_separate_nodes);
  ClassDB::bind_method(D_METHOD("get_barriers_as_separate_nodes"),
                       &Barriers::get_barriers_as_separate_nodes);

  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "barriers_as_separate_nodes"),
               "set_barriers_as_separate_nodes",
               "get_barriers_as_separate_nodes");
}
