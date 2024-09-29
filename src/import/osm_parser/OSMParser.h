#ifndef OSMPARSER_H
#define OSMPARSER_H
#include "../GeoMap.h"
#include "../FileAccessMemoryResizable.h"
#include "../Parser.h"
#include "../../util/GlobalRequirements.h"
#include "OSMHeightmap.h"
#include "../TileMap.h"

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>


class OSMParser : public Parser {
    GDCLASS(OSMParser, Parser);
public:
    using Parser::Parser;
    struct World {
        godot::HashMap<int64_t, godot::Dictionary> nodes;
        godot::HashMap<int64_t, godot::Dictionary> ways;
        godot::HashMap<int64_t, godot::Dictionary> relations;
    };

    godot::Ref<GeoMap> import(godot::Ref<GeoMap> geomap = nullptr, godot::Ref<OSMHeightmap> heightmap = nullptr);
    godot::Ref<GeoMap> help(godot::Ref<GeoMap> geomap, godot::Ref<OSMHeightmap>) {}
    void load_tile(unsigned int index);
    void load_tiles(bool);

    bool get_true() {
        return true;
    }

    void set_filename(const godot::String& value) {
        filename = value;
    }
    godot::String get_filename() const {
        return filename;
    }

    ~OSMParser() override = default;
protected:
    static void _bind_methods();

private:
    struct ParserInfo {
        godot::XMLParser parser;
        godot::TypedArray<godot::Dictionary> xml_stack;
        GlobalRequirements reqs;
        godot::Ref<GeoMap> geomap;
        godot::Ref<TileMap> tilemap;
        godot::Ref<OSMHeightmap> heightmap;
        godot::Dictionary tile_bytes; // Vector2 -> Array of Ref<FileAccessMemoryResizable>
        World world;
        ParserInfo() : geomap(nullptr) {}
    };
    /* Returns true if this is the deepest node (if we have to pop). */
    bool parse_xml_node(ParserInfo&);

    void parse_xml_node_end(ParserInfo&);

    // parse_xml_node helpers
    void parse_bounds(ParserInfo&);
    void parse_node(ParserInfo&, godot::Dictionary& d);

    godot::Vector2i get_element_tile(const godot::String& element_type, ParserInfo& pi, godot::Dictionary& element);

    // Fields
    godot::String filename;
};

#endif // OSMPARSER_H