#include "OSMParser.h"
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <thread>

#define MIN_INT 1 << 31
#define MAX_INT ~(1 << 31)

using namespace godot;

godot::Ref<GeoMap> OSMParser::import(Ref<ParserOutputFileHandle> output_file_handle, godot::Ref<GeoMap> geomap, godot::Ref<OSMHeightmap> heightmap) {
    ParserInfo pi(output_file_handle);

    pi.geomap = geomap;
    pi.tilemap = godot::Ref<TileMapBase>(memnew(EquirectangularTileMap));
    pi.heightmap = heightmap;
    pi.parser->open(filename);

    auto shader_nodes = this->get_shader_nodes();

    for (int i = 0; i < shader_nodes.size(); i++) {
        Object::cast_to<Node>(shader_nodes[i])->call("import_begin");
    }

    for (int i = 0; i < shader_nodes.size(); i++) {
        Variant req = Object::cast_to<Node>(shader_nodes[i])->call("get_globals");
        if (req.get_type() != Variant::Type::NIL)
            pi.reqs->merge(req);
    }

    int print_line = 0;
    while (pi.parser->read() == 0) {
        XMLParser::NodeType cur_node_type = pi.parser->get_node_type();
        bool xml_node_finished = false;
        if (print_line + 100000 < pi.parser->get_current_line()) {
            print_line = pi.parser->get_current_line();
            //std::cout << pi.parser->get_current_line() << std::endl;
        }
        if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT)
            xml_node_finished = parse_xml_node(pi);
        else if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT_END)
            xml_node_finished = true;
        
        if (xml_node_finished)
            parse_xml_node_end(pi);
    }

    //_ASSERT(pi.xml_stack.is_empty());
    
    //pi.parser->close();
    WARN_PRINT("Imported " + String::num_int64(pi.world.nodes.size()) + " nodes; " + 
                String::num_int64(pi.world.ways.size()) + " ways; " +
                String::num_int64(pi.world.relations.size()) + " relations.");

    for (int i = 0; i < shader_nodes.size(); i++) {
        Object::cast_to<Node>(shader_nodes[i])->call("import_finished");
    }

    return pi.geomap;
}

bool OSMParser::parse_xml_node(ParserInfo &pi) {
    const bool pop_node = pi.parser->is_empty();
    pi.xml_stack.push_back(Dictionary());
    Dictionary d = static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 1]);
    const String element_type = pi.parser->get_node_name();
    d["element_type"] = element_type;

    if (pi.parser->has_attribute("id"))
    {
        d["id"]  = pi.parser->get_named_attribute_value("id").to_int();
    }
    

    if (element_type == "bounds")
        parse_bounds(pi);
    else if (element_type == "node")
        parse_node(pi, d);
    else if (element_type == "way")
        d["nodes"] = Array();
    else if (element_type == "relation" || element_type == "area")
    {
        d["members"] = Array();
    }
    else if (element_type == "member")
    {
        Dictionary member_d;
        member_d["id"] = pi.parser->get_named_attribute_value("ref").to_int();
        member_d["role"] = pi.parser->get_named_attribute_value("role");
        member_d["type"] = pi.parser->get_named_attribute_value("type");
        static_cast<Array>(static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])["members"]).push_back(member_d);
    }
    else if (element_type == "nd")
    {
        static_cast<Array>(static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])["nodes"]).push_back(pi.parser->get_named_attribute_value("ref").to_int());
    }
    else if (element_type == "tag")
    {
        static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])[pi.parser->get_named_attribute_value("k")] = pi.parser->get_named_attribute_value("v");
    }
    
    
    
    
	return pop_node;
}

void OSMParser::parse_xml_node_end(ParserInfo &pi) {
    Dictionary item = pi.xml_stack.back();
    const String element_type = item["element_type"].stringify();
    auto shader_nodes = this->get_shader_nodes();

    if (element_type != "node" && element_type != "way" && element_type != "relation") {
        pi.xml_stack.pop_back();
        return;
    }

    Vector2i tile = get_element_tile(element_type, pi, item);


    if (element_type == "node") {
        pi.world.nodes[(int64_t)item["id"]] = item;
        for (int i = 0; i < shader_nodes.size(); i++) {
            Object::cast_to<Node>(shader_nodes[i])->call("import_node", item, pi.output_file_handle->get_parser(tile, i));
        }
    } else if (element_type == "way") {
        
        pi.world.ways[(int64_t)item["id"]] = item;  
        for (int i = 0; i < shader_nodes.size(); i++) {
            Object::cast_to<Node>(shader_nodes[i])->call("import_way", item, pi.output_file_handle->get_parser(tile, i));
        }
    } else if (element_type == "relation") {
        pi.world.relations[(int64_t)item["id"]] = item;
        for (int i = 0; i < shader_nodes.size(); i++) {
            Object::cast_to<Node>(shader_nodes[i])->call("import_relation", item, pi.output_file_handle->get_parser(tile, i));
        }
    }
    
    pi.xml_stack.pop_back();
}

double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

void OSMParser::parse_bounds(ParserInfo & pi) {
    auto shader_nodes = this->get_shader_nodes();
    
    if (pi.geomap.is_null()) {
        pi.geomap = godot::Ref<GeoMap>(memnew(EquirectangularGeoMap(
        GeoCoords(
        Longitude::degrees(pi.parser->get_named_attribute_value("minlon").to_float()),
        Latitude::degrees(pi.parser->get_named_attribute_value("minlat").to_float())),
        
        GeoCoords(
        Longitude::degrees(pi.parser->get_named_attribute_value("maxlon").to_float()),
        Latitude::degrees(pi.parser->get_named_attribute_value("maxlat").to_float())))));
    }
}

void OSMParser::parse_node(ParserInfo & pi, Dictionary& d) {
    GeoCoords coords(Longitude::degrees(pi.parser->get_named_attribute_value("lon").to_float()),
                     Latitude::degrees(pi.parser->get_named_attribute_value("lat").to_float()));
    Vector3 pos = pi.geomap->geo_to_world(coords);
    Vector3 up = pi.geomap->geo_to_world_up(coords);

    d["pos"] = pos;
    d["pos_geo"] = coords.to_vector2_representation();
    d["pos_geo_lon"] = coords.lon.value;
    d["pos_geo_lat"] = coords.lat.value;
    d["up"] = up;
    d["pos_elevation"] = pi.heightmap.is_valid() ? pos + up * pi.heightmap->getElevation(coords) : pos;
}

Vector2i OSMParser::get_element_tile(const String& element_type, ParserInfo& pi, Dictionary& element) {
    if (element_type == "node")
        return pi.tilemap->get_tile_geo(GeoCoords::from_vector2_representation(static_cast<Vector2>(element["pos_geo"])));
    else if (element_type == "way") {
        const Array& nodes = static_cast<Array>(element["nodes"]);
        if (nodes.is_empty()) {
            ERR_PRINT_ED("Way " + static_cast<String>(element.get("id", "<invalid_id>")) + " has no nodes.");
            return Vector2(MIN_INT, MIN_INT);
        }

        return get_element_tile("node", pi, pi.world.nodes[nodes[0]]);
    }
    else if (element_type == "relation") {
        const Array& members = static_cast<Array>(element["members"]);
        
        if (members.is_empty()) {
            ERR_PRINT_ED("Relation " + static_cast<String>(element.get("id", "<invalid_id>")) + " has no members.");
            return Vector2(MIN_INT, MIN_INT);
        }

        for (int i = 0; i < members.size(); i++) {
            const String& type = static_cast<Dictionary>(members[i])["type"];
            if (type == "node") { 
                return get_element_tile("node", pi, pi.world.nodes[members[i]]);
            } else if (type == "way" && !pi.world.ways[static_cast<int64_t>(static_cast<Dictionary>(members[i])["id"])].is_empty()) {
                return get_element_tile("way", pi, pi.world.ways[static_cast<int64_t>(static_cast<Dictionary>(members[i])["id"])]);
            }
        }
    }

    ERR_PRINT_ED("Invalid " + element_type + " " + static_cast<String>(element.get("id", "<invalid_id>")) + ".");
    return Vector2(MIN_INT, MIN_INT);
}

void OSMParser::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import", "geomap", "heightmap"), &OSMParser::import);
    ClassDB::bind_method(D_METHOD("set_filename", "value"), &OSMParser::set_filename);
    ClassDB::bind_method(D_METHOD("get_filename"), &OSMParser::get_filename);
    ClassDB::bind_method(D_METHOD("get_true"), &OSMParser::get_true);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "filename", PROPERTY_HINT_FILE, "*.osm"), "set_filename", "get_filename");

}