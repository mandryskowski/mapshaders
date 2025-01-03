#include "Areas.h"

#include <unordered_map>

#include "../../../src/import/SGImport.h"
#include "../../../src/util/PolyUtil.h"
#include "../../common/3D/RenderUtil3D.h"

using namespace godot;

void Areas::import_begin() {
  tile_info = Dictionary();
  node_pos = HashMap<int64_t, GeoCoords>();
  ways = Dictionary();
  heightmap = nullptr;
}

void Areas::import_node(OSMNodeGD* node,
                        godot::Ref<godot::StreamPeerBuffer> fa) {
  node_pos[node->get_id()] = GeoCoords(Longitude::radians(node->get_lon_rad()),
                                       Latitude::radians(node->get_lat_rad()));
  if (!heightmap) heightmap = node->get_heightmap();
}

void Areas::import_way(OSMWayGD* way, godot::Ref<godot::StreamPeerBuffer> fa) {
  ways[way->get_id()] = way;
}

void Areas::import_relation(OSMRelationGD* relation,
                            godot::Ref<godot::StreamPeerBuffer> fa) {
  std::vector<GeoCoords> outer;
  std::vector<std::vector<GeoCoords>> inners;
  std::unordered_map<uint64_t, bool> seen;  // for checking duplicate nodes

  {
    Array members = relation->get_refs();
    for (int i = 0; i < members.size(); i++) {
      Dictionary member = members[i];
      std::vector<GeoCoords> verts;
      if (!ways.has(member["id"])) return;
      PackedInt64Array way_nodes =
          static_cast<Ref<OSMWayGD>>(ways[member["id"]]).ptr()->get_nodes();

      for (int j = 0; j < way_nodes.size(); j++) {
        if (j != way_nodes.size() - 1 && seen[way_nodes[j]]) {
          std::cout << "Duplicate node in relation " << relation->get_id()
                    << " at i=" << i << "/" << members.size() << ", j=" << j
                    << "/" << way_nodes.size() << std::endl;
          return;
        }
        seen[way_nodes[j]] = true;
        verts.push_back(node_pos[way_nodes[j]]);
      }

      if (member["role"] == "outer")
        outer = verts;
      else if (member["role"] == "inner")
        inners.push_back(verts);
    }
  }

  if (outer.empty()) return;

  Dictionary outd;

  if (EquirectangularGeoMap* geomap = dynamic_cast<EquirectangularGeoMap*>(
          Object::cast_to<SGImport>(get_parent())->get_geo_map().ptr());
      geomap) {
    PolyUtil* pu = memnew(PolyUtil);
    auto rooftris = RenderUtil3D::geo_polygon_to_world(
        pu->triangulate_with_holes(outer, inners), geomap);
    if (heightmap) {
      for (int j = 0; j < rooftris.size(); j++) {
        auto geo = geomap->world_to_geo(rooftris[j]);
        rooftris[j] = rooftris[j] + geomap->geo_to_world_up(geo) *
                                        heightmap->getElevation(geo);
      }
    }
    outd["rooftris"] = rooftris;
  }
  /*outd["outer"] = geo_polygon_to_world(outer, geomap);
  outd["inners"] = Array();
  for (const auto &inner : inners) {
      static_cast<Array>(outd["inners"]).append(geo_polygon_to_world(inner,
  geomap));
  }*/
  outd["name"] = relation->has_tag("name")
                     ? relation->get_tag("name")
                     : String::num_int64(relation->get_id());
  if (relation->has_tag("height"))
    outd["max_height"] = relation->get_tag("height").to_float();
  if (relation->has_tag("min_height"))
    outd["min_height"] = relation->get_tag("min_height").to_float();

  if (!tile_info.has(fa)) {
    TypedArray<Dictionary> rels;
    tile_info[fa] = rels;
  }
  static_cast<TypedArray<Dictionary>>(tile_info[fa]).append(outd);
}

void Areas::import_finished() {
  for (int i = 0; i < tile_info.size(); i++) {
    godot::Ref<godot::StreamPeerBuffer> fa =
        static_cast<godot::Ref<godot::StreamPeerBuffer>>(tile_info.keys()[i]);
    fa->put_var(tile_info[fa]);
  }
}

void Areas::load_tile(godot::Ref<godot::FileAccess> fa, GeoMap* geomap) {
  TypedArray<Dictionary> rels =
      static_cast<TypedArray<Dictionary>>(fa->get_var());

  for (int i = 0; i < rels.size(); i++) {
    Dictionary d = rels[i];
    if (!d.has("rooftris")) continue;

    PackedVector3Array rooftris =
        static_cast<PackedVector3Array>(d["rooftris"]);

    double min_height = d.get("min_height", 0.0);
    double max_height = d.get("max_height", 0.0);

    RenderUtil3D::area_poly(
        this, "Area" + (String)d["name"],
        RenderUtil3D::polygon_triangles(rooftris, max_height, nullptr),
        Color(1, 1, 1), true);

    if (max_height - min_height > 0.0) {
      // walls
    }
  }
}

void Areas::_bind_methods() {
  ClassDB::bind_method(D_METHOD("import_begin"), &Areas::import_begin);
  ClassDB::bind_method(D_METHOD("import_node", "d", "fa"), &Areas::import_node);
  ClassDB::bind_method(D_METHOD("import_way", "d", "fa"), &Areas::import_way);
  ClassDB::bind_method(D_METHOD("import_relation", "d", "fa"),
                       &Areas::import_relation);
  ClassDB::bind_method(D_METHOD("import_finished"), &Areas::import_finished);
  ClassDB::bind_method(D_METHOD("load_tile", "fa", "geomap"),
                       &Areas::load_tile);
}
