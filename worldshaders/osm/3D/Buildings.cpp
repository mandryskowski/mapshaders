#include "Buildings.h"

#include "../../../src/import/SGImport.h"
#include "../../../src/util/PolyUtil.h"
#include "../../common/3D/RenderUtil3D.h"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/variant/color.hpp"
#include "godot_cpp/variant/packed_float64_array.hpp"
#include "godot_cpp/variant/packed_vector3_array.hpp"

using namespace godot;

enum class RoofType { Flat, Pyramidal, Skillion, Hipped, Gabled };

Color osm_to_gd_color(const String& code) {
  return Color(code.replace("grey", "gray"));
}

RoofType osm_to_roof_type(const String& roof_shape) {
  if (roof_shape == "flat") return RoofType::Flat;
  if (roof_shape == "Pyramidal") return RoofType::Pyramidal;
  if (roof_shape == "Skillion") return RoofType::Skillion;
  if (roof_shape == "Hipped") return RoofType::Hipped;
  if (roof_shape == "Gabled") return RoofType::Gabled;

  return RoofType::Flat;
}

Dictionary building_info(OSMWayGD& way) {
  Dictionary d;
  d["name"] = way.has_tag("name") ? way.get_tag("name")
                                  : String::num_int64(way.get_id());
  d["max_height"] =
      way.has_tag("height") ? way.get_tag("height").to_float() : 6.0;
  d["min_height"] =
      way.has_tag("min_height") ? way.get_tag("min_height").to_float() : 0.0;
  d["color"] =
      osm_to_gd_color(way.has_tag("color") ? way.get_tag("color") : "white");
  d["roof_type"] = static_cast<uint8_t>(osm_to_roof_type(d["roof:shape"]));
  if (way.has_tag("roof:colour"))
    d["roof_color"] = osm_to_gd_color(way.get_tag("roof:colour"));
  if (way.has_tag("roof:height")) {
    d["max_height"] = static_cast<double>(d["max_height"]) -
                      way.get_tag("roof:height").to_float();
    d["roof_height"] = way.get_tag("roof:height").to_float();
  }
  if (way.has_tag("roof:direction"))
    d["roof_dir"] = way.get_tag("roof:direction").to_float();
  return d;
}

Array get_roof_arrays(PackedVector3Array& path, double min_height,
                      double max_height, double roof_height,
                      RoofType roof_type) {
  switch (roof_type) {
    case RoofType::Flat:
      return RenderUtil3D::polygon(path, max_height);
  }
  return {};
}

void Buildings::import_begin() {
  tile_info = Dictionary();
  node_pos = HashMap<int64_t, GeoCoords>();
  heightmap = nullptr;
}

void Buildings::import_node(OSMNodeGD* node,
                            godot::Ref<godot::StreamPeerBuffer> fa) {
  node_pos[node->get_id()] = GeoCoords(Longitude::radians(node->get_lon_rad()),
                                       Latitude::radians(node->get_lat_rad()));
  if (!heightmap) heightmap = node->get_heightmap();
}

void Buildings::import_way(OSMWayGD* way,
                           godot::Ref<godot::StreamPeerBuffer> fa) {
  if (!way->has_tag("building") && !way->has_tag("building:part")) return;

  if (!tile_info.has(fa)) {
    TypedArray<Dictionary> ways;
    tile_info[fa] = ways;
  }

  PackedFloat64Array path_lon, path_lat, path_elev;
  PackedInt64Array nodes = way->get_nodes();
  for (int i = 0; i < nodes.size(); i++) {
    GeoCoords coords = node_pos[nodes[i]];
    path_lon.append(coords.lon.value);
    path_lat.append(coords.lat.value);
    if (heightmap) path_elev.append(heightmap->getElevation(coords));
  }

  Dictionary d = building_info(*way);
  d["path_lon"] = path_lon;
  d["path_lat"] = path_lat;
  d["path_elev"] = path_elev;
  static_cast<TypedArray<Dictionary>>(tile_info[fa]).append(d);
}

void Buildings::import_finished() {
  for (int i = 0; i < tile_info.size(); i++) {
    godot::Ref<godot::StreamPeerBuffer> fa =
        static_cast<godot::Ref<godot::StreamPeerBuffer>>(tile_info.keys()[i]);
    fa->put_var(tile_info[fa]);
  }
}

void Buildings::load_tile(godot::Ref<godot::FileAccess> fa, GeoMap* geomap) {
  TypedArray<Dictionary> ways =
      static_cast<TypedArray<Dictionary>>(fa->get_var());

  HashMap<Color, Array> material_arrays;
  auto add_to_material_arrays = [&material_arrays](const Color& color,
                                                   const String& material,
                                                   Array arrays) {
    Color key = color;  // + String(";") + material;
    if (!material_arrays.has(key)) {
      material_arrays[key] = RenderUtil3D::get_array_mesh_arrays(
          {Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL, Mesh::ARRAY_TEX_UV});
    }

    RenderUtil3D::combine_mesh_arrays(material_arrays[key], arrays);
  };

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

    double max_height = d["max_height"];
    double min_height = d["min_height"];
    double roof_height =
        d.has("roof_height") ? static_cast<double>(d["roof_height"]) : 0.0;
    Color color = d["color"];
    Color roof_color =
        d.has("roof_color") ? static_cast<Color>(d["roof_color"]) : color;

    auto walls_arrays = RenderUtil3D::wall_poly(path, min_height, max_height);
    auto roof_arrays = get_roof_arrays(
        path, min_height, max_height, roof_height,
        static_cast<RoofType>(static_cast<uint8_t>(d["roof_type"])));

    if (buildings_as_separate_nodes) {
      Array arrays_of_arrays;
      arrays_of_arrays.push_back(walls_arrays);
      arrays_of_arrays.push_back(roof_arrays);
      RenderUtil3D::area_multipoly(this, d["name"], arrays_of_arrays,
                                   {color, roof_color}, true);
    } else {
      add_to_material_arrays(color, "", walls_arrays);
      if (!roof_arrays.is_empty())
        add_to_material_arrays(roof_color, "", roof_arrays);
    }
  }

  if (!buildings_as_separate_nodes) {
    for (auto it = material_arrays.begin(); it != material_arrays.end(); ++it) {
      Color key = it->key;
      Array arrays = it->value;
      auto building =
          RenderUtil3D::area_poly(this, "Building" + key, arrays, key, true);
    }
  }
}

void Buildings::_bind_methods() {
  ClassDB::bind_method(D_METHOD("import_begin"), &Buildings::import_begin);
  ClassDB::bind_method(D_METHOD("import_node", "d", "fa"),
                       &Buildings::import_node);
  ClassDB::bind_method(D_METHOD("import_way", "d", "fa"),
                       &Buildings::import_way);

  ClassDB::bind_method(D_METHOD("import_finished"),
                       &Buildings::import_finished);
  ClassDB::bind_method(D_METHOD("load_tile", "fa", "geomap"),
                       &Buildings::load_tile);

  ClassDB::bind_method(D_METHOD("set_buildings_as_separate_nodes", "value"),
                       &Buildings::set_buildings_as_separate_nodes);
  ClassDB::bind_method(D_METHOD("get_buildings_as_separate_nodes"),
                       &Buildings::get_buildings_as_separate_nodes);

  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "buildings_as_separate_nodes"),
               "set_buildings_as_separate_nodes",
               "get_buildings_as_separate_nodes");
}
