#ifndef OSMPARSER_H
#define OSMPARSER_H
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include "../../util/GlobalRequirements.h"
#include "../GeoMap.h"
#include "../Parser.h"
#include "../TileMap.h"
#include "../util/ParserOutputFile.h"
#include "OSMHeightmap.h"

class OSMParser : public Parser {
  GDCLASS(OSMParser, Parser);

 public:
  using Parser::Parser;
  struct World {
    godot::HashMap<int64_t, godot::Dictionary> nodes;
    godot::HashMap<int64_t, godot::Dictionary> ways;
    godot::HashMap<int64_t, godot::Dictionary> relations;
  };

  godot::Ref<GeoMap> import(
      godot::Ref<ParserOutputFileHandle> output_file_handle,
      godot::Ref<GeoMap> geomap = nullptr,
      godot::Ref<OSMHeightmap> heightmap = nullptr);
  bool get_true() { return true; }

  void set_filename(const godot::String& value) { filename = value; }
  godot::String get_filename() const { return filename; }

  ~OSMParser() override = default;

 protected:
  static void _bind_methods();

 private:
  struct ParserInfo {
    godot::Ref<godot::XMLParser> parser;
    godot::TypedArray<godot::Dictionary> xml_stack;
    godot::Ref<GlobalRequirements> reqs;
    godot::Ref<GeoMap> geomap;
    godot::Ref<TileMapBase> tilemap;
    godot::Ref<OSMHeightmap> heightmap;
    godot::Ref<ParserOutputFileHandle> output_file_handle;
    World world;
    ParserInfo(godot::Ref<ParserOutputFileHandle> handle)
        : parser(memnew(godot::XMLParser)),
          geomap(nullptr),
          tilemap(nullptr),
          heightmap(nullptr),
          output_file_handle(handle) {}
  };
  /* Returns true if this is the deepest node (if we have to pop). */
  bool parse_xml_node(ParserInfo&);

  void parse_xml_node_end(ParserInfo&);

  // parse_xml_node helpers
  void parse_bounds(ParserInfo&);
  void parse_node(ParserInfo&, godot::Dictionary& d);

  godot::Vector2i get_element_tile(const godot::String& element_type,
                                   ParserInfo& pi, godot::Dictionary& element);

  // Fields
  godot::String filename;
};

#endif  // OSMPARSER_H