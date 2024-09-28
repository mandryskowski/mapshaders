#include "OSMParser.h"
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

godot::Ref<GeoMap> OSMParser::import(godot::Ref<GeoMap> geomap, godot::Ref<OSMHeightmap> heightmap) {
    ParserInfo pi;
    pi.geomap = geomap;
    pi.heightmap = heightmap;
    pi.parser.open(filename);

    auto shader_nodes = this->get_shader_nodes();

    for (int i = 0; i < shader_nodes.size(); i++) {
        Object::cast_to<Node>(shader_nodes[i])->call("import_begin");
    }

    for (int i = 0; i < shader_nodes.size(); i++) {
        Variant req = Object::cast_to<Node>(shader_nodes[i])->call("get_globals");
        if (req.get_type() != Variant::Type::NIL)
            pi.reqs.merge(req);
    }

    while (pi.parser.read() == 0) {
        XMLParser::NodeType cur_node_type = pi.parser.get_node_type();
        bool xml_node_finished = false;
        if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT)
            xml_node_finished = parse_xml_node(pi);
        else if (cur_node_type == XMLParser::NodeType::NODE_ELEMENT_END)
            xml_node_finished = true;
        
        if (xml_node_finished)
            parse_xml_node_end(pi);
    }

    //_ASSERT(pi.xml_stack.is_empty());
    
    //pi.parser.close();
    WARN_PRINT("Imported " + String::num_int64(pi.world.nodes.size()) + " nodes; " + 
                String::num_int64(pi.world.ways.size()) + " ways; " +
                String::num_int64(pi.world.relations.size()) + " relations.");

    for (int i = 0; i < shader_nodes.size(); i++) {
        Object::cast_to<Node>(shader_nodes[i])->call("import_finished");
    }

    Ref<FileAccess> out = FileAccess::open("res://parsed.sgdmap", FileAccess::WRITE);
    PackedInt64Array tile_offs, tile_lens;
    // We will store tile offsets at the beginning
    tile_offs.resize (5 * 5);
    tile_lens.resize (5 * 5);
    out->store_var (tile_offs);
    out->store_var (tile_lens);

    for (int i = 0; i < 5 * 5; i++) {
        const_cast<int64_t&>(tile_offs[i]) = out->get_position();
        Vector<Ref<FileAccessMemoryResizable>>& tile_fas = const_cast<Vector<Ref<FileAccessMemoryResizable>>&>(pi.tile_bytes[i]);

        for (int j = 0; j < shader_nodes.size(); j++) {
            Ref<FileAccessMemoryResizable>& fa = const_cast<Ref<FileAccessMemoryResizable>&>(tile_fas[j]);

            size_t len = fa->get_position();
            fa->seek(0);
            PackedByteArray arr = fa->get_data_array();

            if (arr.size() != len)
                WARN_PRINT("Bug in OSMParser::import: buffer length mismatch.");

            // Empty flag
            out->store_8 (len == 0 ? 1 : 0);
            // Contents
            out->store_buffer (arr);
        }
        const_cast<int64_t&>(tile_lens[i]) = out->get_position() - tile_offs[i];
    }
    
    // Overwrite tile offsets at the beginning
    out->seek(0);
    out->store_var (tile_offs);
    out->store_var (tile_lens);
    return pi.geomap;
}

bool OSMParser::parse_xml_node(ParserInfo &pi) {
    XMLParser& parser = pi.parser;
    const bool pop_node = parser.is_empty();
    pi.xml_stack.push_back(Dictionary());
    Dictionary d = static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 1]);
    const String element_type = parser.get_node_name();
    d["element_type"] = element_type;

    if (parser.has_attribute("id"))
    {
        d["id"]  = parser.get_named_attribute_value("id").to_int();
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
        member_d["id"] = parser.get_named_attribute_value("ref").to_int();
        member_d["role"] = parser.get_named_attribute_value("role");
        member_d["type"] = parser.get_named_attribute_value("type");
        static_cast<Array>(static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])["members"]).push_back(member_d);
    }
    else if (element_type == "nd")
    {
        static_cast<Array>(static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])["nodes"]).push_back(parser.get_named_attribute_value("ref").to_int());
    }
    else if (element_type == "tag")
    {
        static_cast<Dictionary>(pi.xml_stack[pi.xml_stack.size() - 2])[parser.get_named_attribute_value("k")] = parser.get_named_attribute_value("v");
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

    unsigned int tile = get_element_tile(element_type, pi, item);

    //tile = 0;
    if (tile >= 5 * 5) {
        pi.xml_stack.pop_back();
        return;
    }


    if (element_type == "node") {
        pi.world.nodes[(int64_t)item["id"]] = item;
        for (int i = 0; i < pi.tile_bytes[tile].size(); i++) {
            Object::cast_to<Node>(shader_nodes[i])->call("import_node", item, pi.tile_bytes[tile][i]);

        }
        //Error res = sg_import.emit_signal("import_node", item, *pi.tile_bytes[tile]);
    } else if (element_type == "way") {
        
        pi.world.ways[(int64_t)item["id"]] = item;  
        for (int i = 0; i < pi.tile_bytes[tile].size(); i++) {
            Object::cast_to<Node>(shader_nodes[i])->call("import_way", item, pi.tile_bytes[tile][i]);
        }
    } else if (element_type == "relation") {
        pi.world.relations[(int64_t)item["id"]] = item;
        for (int i = 0; i < pi.tile_bytes[tile].size(); i++) {
            Object::cast_to<Node>(shader_nodes[i])->call("import_relation", item, pi.tile_bytes[tile][i]);
        }
    }
    
    pi.xml_stack.pop_back();
}

double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

void OSMParser::parse_bounds(ParserInfo & pi) {
    const int grid_size = 5;
    auto shader_nodes = this->get_shader_nodes();
    
    if (pi.geomap.is_null()) {
        pi.geomap = godot::Ref<GeoMap>(memnew(EquirectangularGeoMap(
        GeoCoords(
        Longitude::degrees(pi.parser.get_named_attribute_value("minlon").to_float()),
        Latitude::degrees(pi.parser.get_named_attribute_value("minlat").to_float())),
        
        GeoCoords(
        Longitude::degrees(pi.parser.get_named_attribute_value("maxlon").to_float()),
        Latitude::degrees(pi.parser.get_named_attribute_value("maxlat").to_float())))));
    }

    pi.tile_bytes.clear();
    pi.tile_bytes.resize (grid_size * grid_size);

    for (int i = 0; i < grid_size * grid_size; i++) {
        Vector<Ref<FileAccessMemoryResizable>>& tile_fas = const_cast<Vector<Ref<FileAccessMemoryResizable>>&>(pi.tile_bytes[i]);
        tile_fas.resize (shader_nodes.size());
        for (int j = 0; j < shader_nodes.size(); j++) {
            Ref<FileAccessMemoryResizable>& fa = const_cast<Ref<FileAccessMemoryResizable>&>(tile_fas[j]);
            fa.instantiate();
            //fa->open_resizable (2);
        }
    }
}

void OSMParser::parse_node(ParserInfo & pi, Dictionary& d) {
    GeoCoords coords(Longitude::degrees(pi.parser.get_named_attribute_value("lon").to_float()),
                     Latitude::degrees(pi.parser.get_named_attribute_value("lat").to_float()));
    Vector3 pos = pi.geomap->geo_to_world(coords);
    Vector3 up = pi.geomap->geo_to_world_up(coords);

    d["pos"] = pos;
    d["pos_geo"] = coords.to_vector2_representation();
    d["up"] = up;
    d["pos_elevation"] = pi.heightmap.is_valid() ? pos + up * pi.heightmap->getElevation(coords) : pos;
}

unsigned int OSMParser::get_element_tile(const String& element_type, ParserInfo& pi, Dictionary& element) {
    if (element_type == "node")
        return 0;//return pi.geomap->grid_index (element["pos"]);
    else if (element_type == "way") {
        const Array& nodes = static_cast<Array>(element["nodes"]);
        if (nodes.is_empty()) {
            ERR_PRINT_ED("Way " + static_cast<String>(element.get("id", "<invalid_id>")) + " has no nodes.");
            return 0;
        }

        return get_element_tile("node", pi, pi.world.nodes[nodes[0]]);
    }
    else if (element_type == "relation") {
        const Array& members = static_cast<Array>(element["members"]);
        
        if (members.is_empty()) {
            ERR_PRINT_ED("Relation " + static_cast<String>(element.get("id", "<invalid_id>")) + " has no members.");
            return 0;
        }

        for (int i = 0; i < members.size(); i++) {
            const String& type = static_cast<Dictionary>(members[i])["type"];
            if (type == "node") { 
                return get_element_tile("node", pi, pi.world.nodes[members[i]]);
            } else if (type == "way" && !pi.world.ways[members[i]].is_empty()) {
                return get_element_tile("way", pi, pi.world.ways[members[i]]);
            }
        }
    }

    ERR_PRINT_ED("Invalid " + element_type + " " + static_cast<String>(element.get("id", "<invalid_id>")) + ".");
    return 0;
}


void OSMParser::load_tile(unsigned int index) {
    Ref<FileAccess> fa = FileAccess::open("res://parsed.sgdmap", FileAccess::READ);
    PackedInt64Array tile_offs, tile_lens;

    tile_offs = fa->get_var();
    tile_lens = fa->get_var();

    if (tile_offs.size() != tile_lens.size())
        WARN_PRINT ("Tile offsets count different from tile lengths count");
    
    if (tile_lens[index] == 0) {
        fa->close();
        return;
    }

    fa->seek (tile_offs[index]);
    auto shader_nodes_osm = this->get_shader_nodes();
    for (int i = 0; i < shader_nodes_osm.size(); i++) {
        WARN_PRINT("Tile " + String::num_int64(index) + " offset is " + String::num_int64(fa->get_position()));
        if (fa->get_8() == 1)
            continue;
        Object::cast_to<Node>(shader_nodes_osm[i])->call("load_tile", fa);
    }

    fa->close();
}

void OSMParser::load_tiles(bool) {
    for (int i = 0; i < 5 * 5; i++) {
        load_tile (i);
    }
}

void OSMParser::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import", "geomap", "heightmap"), &OSMParser::import);
    ClassDB::bind_method(D_METHOD("set_filename", "value"), &OSMParser::set_filename);
    ClassDB::bind_method(D_METHOD("get_filename"), &OSMParser::get_filename);
    ClassDB::bind_method(D_METHOD("load_tile", "index"), &OSMParser::load_tile);
    ClassDB::bind_method(D_METHOD("load_tiles", "plsrefactor"), &OSMParser::load_tiles);
    ClassDB::bind_method(D_METHOD("get_true"), &OSMParser::get_true);

    ClassDB::bind_method(D_METHOD("help", "plsrefactor", "halo"), &OSMParser::help);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "filename", PROPERTY_HINT_FILE, "*.osm"), "set_filename", "get_filename");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "load_all_tiles"), "load_tiles", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "help"), "help", "get_true");
}