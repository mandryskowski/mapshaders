#ifndef OSMPARSER_H
#define OSMPARSER_H
#include "../GeoMap.h"
#include "../FileAccessMemoryResizable.h"
#include "../SGImport.h"
#include "../../util/GlobalRequirements.h"

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

class OSMParser {
public:
    struct World {
        godot::HashMap<int64_t, godot::Dictionary> nodes;
        godot::HashMap<int64_t, godot::Dictionary> ways;
        godot::HashMap<int64_t, godot::Dictionary> relations;
    };

    OSMParser(SGImport& sg_import): sg_import(sg_import) {}

    World import(const godot::String& file_name);

private:
    SGImport& sg_import;
    struct ParserInfo {
        godot::XMLParser parser;
        godot::Array xml_stack; // Array of Dictionaries.
        GlobalRequirements reqs;
        std::unique_ptr<GeoMap> geomap;
        godot::Vector<godot::Vector<godot::Ref<FileAccessMemoryResizable>>> tile_bytes;
        World world;
        ParserInfo() : geomap(nullptr) {}
    };
    /* Returns true if this is the deepest node (if we have to pop). */
    bool parse_xml_node(ParserInfo&);

    void parse_xml_node_end(ParserInfo&);

    // parse_xml_node helpers
    void parse_bounds(ParserInfo&);
    void parse_node(ParserInfo&, godot::Dictionary& d);

    unsigned int get_element_tile(const godot::String& element_type, ParserInfo& pi, godot::Dictionary& element);

    // parse_xml_node_end helpers

};

#endif // OSMPARSER_H