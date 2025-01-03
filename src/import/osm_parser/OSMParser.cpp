#include "OSMParser.h"

#include <osmpbf/osmpbf.h>
#include <zlib.h>

#include <cstddef>
#include <cstdint>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <limits>
#include <thread>

#include "../util/PortableByteSwap.h"
#include "OSMModel.h"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/ref_counted.hpp"
#include "godot_cpp/core/error_macros.hpp"

#define MIN_INT 1 << 31
#define MAX_INT ~(1 << 31)

using namespace godot;

#include <fstream>
#include <iostream>
#include <vector>

#include "OSMModelGD.h"
#include "OSMPBFReader.h"
#include "OSMXMLReader.h"

template <typename Visitor>
class OSMPBFParserVisitor {
 public:
  OSMPBFParserVisitor(Visitor& visitor, Ref<GeoMap> geomap,
                      OSMHeightmap* heightmap)
      : visitor(visitor), geomap(geomap), heightmap(heightmap) {}
  void node_callback(uint64_t id, double lon, double lat,
                     osmpbfreader::Tags tags) {
    visitor.node_callback(
        OSMNode(id, GeoCoords(Longitude::degrees(lon), Latitude::degrees(lat)),
                tags, geomap.ptr(), heightmap));
  }
  void way_callback(uint64_t id, osmpbfreader::Tags tags,
                    std::vector<uint64_t> refs) {
    visitor.way_callback(OSMWay(id, refs, tags));
  }
  void relation_callback(uint64_t id, osmpbfreader::Tags tags,
                         osmpbfreader::References refs) {
    std::vector<OSMRelation::Reference> references;
    for (const osmpbfreader::Reference& ref : refs) {
      OSMRelation::MemberType type;
      switch (ref.member_type) {
        case OSMPBF::Relation_MemberType_NODE:
          type = OSMRelation::MemberType::Node;
          break;
        case OSMPBF::Relation_MemberType_WAY:
          type = OSMRelation::MemberType::Way;
          break;
        case OSMPBF::Relation_MemberType_RELATION:
          type = OSMRelation::MemberType::Relation;
          break;
      }
      references.push_back(
          OSMRelation::Reference(ref.member_id, ref.role, type));
    }
    visitor.relation_callback(OSMRelation(id, references, tags));
  }
  void bounds_callback(int64_t minlon_nano, int64_t maxlon_nano,
                       int64_t minlat_nano, int64_t maxlat_nano) {
    if (!geomap.is_valid())
      geomap = Ref<GeoMap>(memnew(EquirectangularGeoMap(
          GeoCoords(Longitude::nanodegrees(minlon_nano),
                    Latitude::nanodegrees(minlat_nano)),
          GeoCoords(Longitude::nanodegrees(maxlon_nano),
                    Latitude::nanodegrees(maxlat_nano)))));
  }

  Ref<GeoMap> get_geomap() { return geomap; }

 private:
  Visitor& visitor;
  Ref<GeoMap> geomap;
  OSMHeightmap* heightmap;
};

template <typename Visitor>
class OSMXMLParserVisitor {
 public:
  OSMXMLParserVisitor(Visitor& visitor, Ref<GeoMap> geomap,
                      OSMHeightmap* heightmap)
      : visitor(visitor), geomap(geomap), heightmap(heightmap) {}
  void node_callback(const Dictionary& node) {
    int64_t id;
    GeoCoords coords;
    std::unordered_map<std::string, std::string> tags;
    for (int i = 0; i < node.size(); i++) {
      auto key = node.keys()[i];
      auto value = node[key];
      if (key == "id") {
        id = value;
      } else if (key == "pos_geo_lon") {
        coords.lon = Longitude::degrees(value);
      } else if (key == "pos_geo_lat") {
        coords.lat = Latitude::degrees(value);
      } else {
        std::string key_str = static_cast<String>(key).ascii().ptr();
        std::string value_str = static_cast<String>(value).ascii().ptr();
        tags[key_str] = value_str;
      }
    }
    visitor.node_callback(OSMNode(id, coords, tags, geomap.ptr(), heightmap));
  }
  void way_callback(const Dictionary& way) {
    int64_t id;
    std::vector<uint64_t> nodes;
    std::unordered_map<std::string, std::string> tags;

    for (int i = 0; i < way.size(); i++) {
      auto key = way.keys()[i];
      auto value = way[key];
      if (key == "id") {
        id = value;
      } else if (key == "nodes") {
        Array value_arr = static_cast<Array>(value);
        for (int j = 0; j < value_arr.size(); j++) {
          nodes.push_back(value_arr[j]);
        }
      } else {
        std::string key_str = static_cast<String>(key).ascii().ptr();
        std::string value_str = static_cast<String>(value).ascii().ptr();
        tags[key_str] = value_str;
      }
    }
    visitor.way_callback(OSMWay(id, nodes, tags));
  }
  void relation_callback(const Dictionary& relation) {
    int64_t id;
    std::vector<OSMRelation::Reference> refs;
    std::unordered_map<std::string, std::string> tags;
    for (int i = 0; i < relation.size(); i++) {
      auto key = relation.keys()[i];
      auto value = relation[key];
      if (key == "id") {
        id = value;
      } else if (key == "members") {
        Array value_refs = static_cast<Array>(value);
        for (int j = 0; j < value_refs.size(); j++) {
          Dictionary ref = static_cast<Dictionary>(value_refs[j]);
          OSMRelation::MemberType type;
          if (ref["type"] == "node") {
            type = OSMRelation::MemberType::Node;
          } else if (ref["type"] == "way") {
            type = OSMRelation::MemberType::Way;
          } else if (ref["type"] == "relation") {
            type = OSMRelation::MemberType::Relation;
          } else {
            WARN_PRINT(String("Unknown member type: ") + String(ref["type"]));
          }
          refs.push_back(OSMRelation::Reference(
              static_cast<int64_t>(ref["id"]),
              static_cast<String>(ref["role"]).ascii().ptr(), type));
        }

      } else {
        std::string key_str = static_cast<String>(key).ascii().ptr();
        std::string value_str = static_cast<String>(value).utf8().ptr();
        tags[key_str] = value_str;
      }
    }
    visitor.relation_callback(OSMRelation(id, refs, tags));
  }
  void bounds_callback(double minlon, double maxlon, double minlat,
                       double maxlat) {
    if (!geomap.is_valid())
      geomap = Ref<GeoMap>(memnew(EquirectangularGeoMap(
          GeoCoords(Longitude::degrees(minlon), Latitude::degrees(minlat)),
          GeoCoords(Longitude::degrees(maxlon), Latitude::degrees(maxlat)))));
  }

  Ref<GeoMap> get_geomap() { return geomap; }

 private:
  Visitor& visitor;
  Ref<GeoMap> geomap;
  OSMHeightmap* heightmap;
};

class OSMCaller {
 public:
  OSMCaller(godot::TypedArray<godot::Node> shader_nodes,
            EquirectangularTileMap& tilemap,
            ParserOutputFileHandle& output_file_handle)
      : shader_nodes(shader_nodes),
        tilemap(tilemap),
        output_file_handle(output_file_handle) {}

 protected:
  std::unordered_map<uint64_t, Vector2i> nodes_tiles;
  std::unordered_map<uint64_t, Vector2i> ways_tiles;

  Vector2i put_node(uint64_t id, const GeoCoords& coords) {
    const auto tile = tilemap.get().get_tile_geo(coords);
    nodes_tiles[id] = tile;
    return tile;
  }
  Vector2i put_way(uint64_t id, const std::vector<uint64_t>& nodes) {
    if (nodes.empty())
      return Vector2i(std::numeric_limits<uint64_t>::max(),
                      std::numeric_limits<uint64_t>::max());
    const auto tile = nodes_tiles[nodes[0]];
    ways_tiles[id] = tile;
    return tile;
  }
  Vector2i put_relation(uint64_t,
                        const std::vector<OSMRelation::Reference>& members) {
    if (members.empty())
      return Vector2i(std::numeric_limits<uint64_t>::max(),
                      std::numeric_limits<uint64_t>::max());
    for (const OSMRelation::Reference& ref : members) {
      if (ref.type == OSMRelation::MemberType::Node) {
        return nodes_tiles[ref.id];
      } else if (ref.type == OSMRelation::MemberType::Way &&
                 ways_tiles.find(ref.id) != ways_tiles.end()) {
        return ways_tiles[ref.id];
      }
    }

    return Vector2i(std::numeric_limits<uint64_t>::max(),
                    std::numeric_limits<uint64_t>::max());
  }

  godot::TypedArray<godot::Node> shader_nodes;
  std::reference_wrapper<EquirectangularTileMap> tilemap;
  std::reference_wrapper<ParserOutputFileHandle> output_file_handle;
};

class OSMGDScriptCaller : public OSMCaller {
 public:
  using OSMCaller::OSMCaller;
  void node_callback(OSMNode&& node) {
    const auto coords = node.get_coords();
    Ref<OSMNodeGD> node_gd = memnew(OSMNodeGD(std::move(node)));
    for (int i = 0; i < shader_nodes.size(); i++) {
      Object::cast_to<Node>(shader_nodes[i])
          ->call("import_node", node_gd,
                 output_file_handle.get().get_parser(
                     put_node(node_gd->get_id(), coords), i));
    }
  }
  void way_callback(OSMWay&& way) {
    const auto nodes = way.get_nodes();
    Ref<OSMWayGD> way_gd = memnew(OSMWayGD(std::move(way)));
    for (int i = 0; i < shader_nodes.size(); i++) {
      Object::cast_to<Node>(shader_nodes[i])
          ->call("import_way", way_gd,
                 output_file_handle.get().get_parser(
                     put_way(way_gd->get_id(), nodes), i));
    }
  }
  void relation_callback(OSMRelation&& relation) {
    const auto refs = relation.get_refs();
    Ref<OSMRelationGD> relation_gd = memnew(OSMRelationGD(std::move(relation)));
    for (int i = 0; i < shader_nodes.size(); i++) {
      Object::cast_to<Node>(shader_nodes[i])
          ->call("import_relation", relation_gd,
                 output_file_handle.get().get_parser(
                     put_relation(relation_gd->get_id(), refs), i));
    }
  }
};

godot::Ref<GeoMap> OSMParser::import(
    Ref<ParserOutputFileHandle> output_file_handle, godot::Ref<GeoMap> geomap,
    godot::Ref<OSMHeightmap> heightmap) {
  auto shader_nodes = this->get_shader_nodes();
  for (int i = 0; i < shader_nodes.size(); i++) {
    Object::cast_to<Node>(shader_nodes[i])->call("import_begin");
  }

  // for (int i = 0; i < shader_nodes.size(); i++) {
  //     Variant req =
  //     Object::cast_to<Node>(shader_nodes[i])->call("get_globals"); if
  //     (req) reqs->merge(req);
  //   }
  godot::Ref<EquirectangularTileMap> tilemap =
      godot::Ref<EquirectangularTileMap>(memnew(EquirectangularTileMap));
  OSMGDScriptCaller caller(shader_nodes, *tilemap.ptr(),
                           *output_file_handle.ptr());

  if (filename.ends_with(".osm")) {
    OSMXMLParserVisitor visitor(caller, geomap, heightmap.ptr());
    osmxmlreader::read_osm_xml(filename, visitor);
    geomap = visitor.get_geomap();
  } else if (filename.ends_with(".osm.pbf")) {
    OSMPBFParserVisitor visitor(caller, geomap, heightmap.ptr());
    osmpbfreader::read_osm_pbf(ProjectSettings::get_singleton()
                                   ->globalize_path(filename)
                                   .ascii()
                                   .ptr(),
                               visitor);
    geomap = visitor.get_geomap();
  }
  for (int i = 0; i < shader_nodes.size(); i++) {
    Object::cast_to<Node>(shader_nodes[i])->call("import_finished");
  }

  return geomap;
  ParserInfo pi(output_file_handle);

  pi.geomap = geomap;
  pi.tilemap = godot::Ref<TileMapBase>(memnew(EquirectangularTileMap));
  pi.heightmap = heightmap;
  pi.parser->open(filename);

  for (int i = 0; i < shader_nodes.size(); i++) {
    Object::cast_to<Node>(shader_nodes[i])->call("import_begin");
  }

  for (int i = 0; i < shader_nodes.size(); i++) {
    Variant req = Object::cast_to<Node>(shader_nodes[i])->call("get_globals");
    if (req.get_type() != Variant::Type::NIL) pi.reqs->merge(req);
  }

  int print_line = 0;
  while (pi.parser->read() == 0) {
    XMLParser::NodeType cur_node_type = pi.parser->get_node_type();
    bool xml_node_finished = false;
    if (print_line + 100000 < pi.parser->get_current_line()) {
      print_line = pi.parser->get_current_line();
      // std::cout << pi.parser->get_current_line() << std::endl;
    }
    if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT)
      xml_node_finished = parse_xml_node(pi);
    else if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT_END)
      xml_node_finished = true;

    if (xml_node_finished) parse_xml_node_end(pi);
  }

  //_ASSERT(pi.xml_stack.is_empty());

  // pi.parser->close();
  WARN_PRINT("Imported " + String::num_int64(pi.world.nodes.size()) +
             " nodes; " + String::num_int64(pi.world.ways.size()) + " ways; " +
             String::num_int64(pi.world.relations.size()) + " relations.");

  for (int i = 0; i < shader_nodes.size(); i++) {
    Object::cast_to<Node>(shader_nodes[i])->call("import_finished");
  }

  return pi.geomap;
}

bool OSMParser::parse_xml_node(ParserInfo& pi) {
  const bool pop_node = pi.parser->is_empty();
  pi.xml_stack.push_back(Dictionary());
  Dictionary d = static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 1]);
  const String element_type = pi.parser->get_node_name();
  d["element_type"] = element_type;

  if (pi.parser->has_attribute("id")) {
    d["id"] = pi.parser->get_named_attribute_value("id").to_int();
  }

  if (element_type == "bounds")
    parse_bounds(pi);
  else if (element_type == "node")
    parse_node(pi, d);
  else if (element_type == "way")
    d["nodes"] = Array();
  else if (element_type == "relation" || element_type == "area") {
    d["members"] = Array();
  } else if (element_type == "member") {
    Dictionary member_d;
    member_d["id"] = pi.parser->get_named_attribute_value("ref").to_int();
    member_d["role"] = pi.parser->get_named_attribute_value("role");
    member_d["type"] = pi.parser->get_named_attribute_value("type");
    static_cast<Array>(static_cast<Dictionary>(
                           pi.xml_stack[pi.xml_stack.size() - 2])["members"])
        .push_back(member_d);
  } else if (element_type == "nd") {
    static_cast<Array>(
        static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])["nodes"])
        .push_back(pi.parser->get_named_attribute_value("ref").to_int());
  } else if (element_type == "tag") {
    static_cast<Dictionary>(
        pi.xml_stack[pi.xml_stack.size() -
                     2])[pi.parser->get_named_attribute_value("k")] =
        pi.parser->get_named_attribute_value("v");
  }

  return pop_node;
}

void OSMParser::parse_xml_node_end(ParserInfo& pi) {
  Dictionary item = pi.xml_stack.back();
  const String element_type = item["element_type"].stringify();
  auto shader_nodes = this->get_shader_nodes();

  if (element_type != "node" && element_type != "way" &&
      element_type != "relation") {
    pi.xml_stack.pop_back();
    return;
  }

  Vector2i tile = get_element_tile(element_type, pi, item);

  if (element_type == "node") {
    pi.world.nodes[(int64_t)item["id"]] = item;
    for (int i = 0; i < shader_nodes.size(); i++) {
      Object::cast_to<Node>(shader_nodes[i])
          ->call("import_node", item,
                 pi.output_file_handle->get_parser(tile, i));
    }
  } else if (element_type == "way") {
    pi.world.ways[(int64_t)item["id"]] = item;
    for (int i = 0; i < shader_nodes.size(); i++) {
      Object::cast_to<Node>(shader_nodes[i])
          ->call("import_way", item,
                 pi.output_file_handle->get_parser(tile, i));
    }
  } else if (element_type == "relation") {
    pi.world.relations[(int64_t)item["id"]] = item;
    for (int i = 0; i < shader_nodes.size(); i++) {
      Object::cast_to<Node>(shader_nodes[i])
          ->call("import_relation", item,
                 pi.output_file_handle->get_parser(tile, i));
    }
  }

  pi.xml_stack.pop_back();
}

void OSMParser::parse_bounds(ParserInfo& pi) {
  auto shader_nodes = this->get_shader_nodes();

  if (pi.geomap.is_null()) {
    pi.geomap = godot::Ref<GeoMap>(memnew(EquirectangularGeoMap(
        GeoCoords(
            Longitude::degrees(
                pi.parser->get_named_attribute_value("minlon").to_float()),
            Latitude::degrees(
                pi.parser->get_named_attribute_value("minlat").to_float())),

        GeoCoords(
            Longitude::degrees(
                pi.parser->get_named_attribute_value("maxlon").to_float()),
            Latitude::degrees(
                pi.parser->get_named_attribute_value("maxlat").to_float())))));
  }
}

void OSMParser::parse_node(ParserInfo& pi, Dictionary& d) {
  GeoCoords coords(Longitude::degrees(
                       pi.parser->get_named_attribute_value("lon").to_float()),
                   Latitude::degrees(
                       pi.parser->get_named_attribute_value("lat").to_float()));
  Vector3 pos = pi.geomap->geo_to_world(coords);
  Vector3 up = pi.geomap->geo_to_world_up(coords);

  d["pos"] = pos;
  d["pos_elevation"] = pi.heightmap.is_valid()
                           ? pos + up * pi.heightmap->getElevation(coords)
                           : pos;
  d["pos_geo_lon"] = coords.lon.value;
  d["pos_geo_lat"] = coords.lat.value;
  d["up"] = up;
}

Vector2i OSMParser::get_element_tile(const String& element_type, ParserInfo& pi,
                                     Dictionary& element) {
  if (element_type == "node")
    return pi.tilemap->get_tile_geo(GeoCoords::from_vector2_representation(
        static_cast<Vector2>(element["pos_geo"])));
  else if (element_type == "way") {
    const Array& nodes = static_cast<Array>(element["nodes"]);
    if (nodes.is_empty()) {
      ERR_PRINT_ED("Way " +
                   static_cast<String>(element.get("id", "<invalid_id>")) +
                   " has no nodes.");
      return Vector2(MIN_INT, MIN_INT);
    }

    return get_element_tile("node", pi, pi.world.nodes[nodes[0]]);
  } else if (element_type == "relation") {
    const Array& members = static_cast<Array>(element["members"]);

    if (members.is_empty()) {
      ERR_PRINT_ED("Relation " +
                   static_cast<String>(element.get("id", "<invalid_id>")) +
                   " has no members.");
      return Vector2(MIN_INT, MIN_INT);
    }

    for (int i = 0; i < members.size(); i++) {
      const String& type = static_cast<Dictionary>(members[i])["type"];
      if (type == "node") {
        return get_element_tile("node", pi, pi.world.nodes[members[i]]);
      } else if (type == "way" &&
                 !pi.world
                      .ways[static_cast<int64_t>(
                          static_cast<Dictionary>(members[i])["id"])]
                      .is_empty()) {
        return get_element_tile(
            "way", pi,
            pi.world.ways[static_cast<int64_t>(
                static_cast<Dictionary>(members[i])["id"])]);
      }
    }
  }

  ERR_PRINT_ED("Invalid " + element_type + " " +
               static_cast<String>(element.get("id", "<invalid_id>")) + ".");
  return Vector2(MIN_INT, MIN_INT);
}

void OSMParser::_bind_methods() {
  ClassDB::bind_method(
      D_METHOD("import", "output_file_handle", "geomap", "heightmap"),
      &OSMParser::import);
  ClassDB::bind_method(D_METHOD("set_filename", "value"),
                       &OSMParser::set_filename);
  ClassDB::bind_method(D_METHOD("get_filename"), &OSMParser::get_filename);
  ClassDB::bind_method(D_METHOD("get_true"), &OSMParser::get_true);

  ADD_PROPERTY(PropertyInfo(Variant::STRING, "filename", PROPERTY_HINT_FILE,
                            "*.osm,*.osm.pbf"),
               "set_filename", "get_filename");
}