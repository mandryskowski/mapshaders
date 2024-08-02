#include "OSMParser.h"
#include <godot_cpp/classes/xml_parser.hpp>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/classes/file_access.hpp>

using namespace godot;

OSMParser::World OSMParser::import(const String& file_name) {
    ParserInfo pi;
    pi.parser.open(file_name);

    for (int i = 0; i < sg_import.parser_nodes.size(); i++) {
        sg_import.get_node<Node> (sg_import.parser_nodes[i])->call("import_begin");
    }

    for (int i = 0; i < sg_import.parser_nodes.size(); i++) {
        Variant req = sg_import.get_node<Node> (sg_import.parser_nodes[i])->call("get_globals");
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

    for (int i = 0; i < sg_import.parser_nodes.size(); i++) {
        sg_import.get_node<Node> (sg_import.parser_nodes[i])->call("import_finished");
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

        for (int j = 0; j < sg_import.parser_nodes.size(); j++) {
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
        WARN_PRINT("Tile " + String::num_int64(i) + " size is  " + String::num_int64(tile_lens[i]));
        for (int j = 0; j < sg_import.parser_nodes.size(); j++) {
            WARN_PRINT(tile_fas[j]->to_string());
        }
    }
    
    // Overwrite tile offsets at the beginning
    out->seek(0);
    out->store_var (tile_offs);
    out->store_var (tile_lens);
    return pi.world;
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
            sg_import.get_node<Node> (sg_import.parser_nodes[i])->call ("import_node", item, pi.tile_bytes[tile][i]);
        }
        //Error res = sg_import.emit_signal("import_node", item, *pi.tile_bytes[tile]);
    } else if (element_type == "way") {
        
        pi.world.ways[(int64_t)item["id"]] = item;  
        for (int i = 0; i < pi.tile_bytes[tile].size(); i++) {
            sg_import.get_node<Node> (sg_import.parser_nodes[i])->call ("import_way", item, pi.tile_bytes[tile][i]);
        }
    } else if (element_type == "relation") {
        pi.world.relations[(int64_t)item["id"]] = item;
        for (int i = 0; i < pi.tile_bytes[tile].size(); i++) {
            sg_import.get_node<Node> (sg_import.parser_nodes[i])->call ("import_relation", item, pi.tile_bytes[tile][i]);
        }
    }
    
    pi.xml_stack.pop_back();
}

double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

void OSMParser::parse_bounds(ParserInfo & pi) {
    const int grid_size = 5;
    pi.geomap = std::make_unique<GeoMap> (
    GeoCoords(
    Longitude::degrees(pi.parser.get_named_attribute_value("minlon").to_float()),
    Latitude::degrees(pi.parser.get_named_attribute_value("minlat").to_float())),
    
    GeoCoords(
    Longitude::degrees(pi.parser.get_named_attribute_value("maxlon").to_float()),
    Latitude::degrees(pi.parser.get_named_attribute_value("maxlat").to_float())),
    
    grid_size);

    pi.tile_bytes.clear();
    pi.tile_bytes.resize (grid_size * grid_size);

    for (int i = 0; i < grid_size * grid_size; i++) {
        Vector<Ref<FileAccessMemoryResizable>>& tile_fas = const_cast<Vector<Ref<FileAccessMemoryResizable>>&>(pi.tile_bytes[i]);
        tile_fas.resize (sg_import.parser_nodes.size());
        for (int j = 0; j < sg_import.parser_nodes.size(); j++) {
            Ref<FileAccessMemoryResizable>& fa = const_cast<Ref<FileAccessMemoryResizable>&>(tile_fas[j]);
            fa.instantiate();
            //fa->open_resizable (2);
        }
    }

    printf ("bounds neg %.6f %.6f \n", pi.geomap->geo_to_world (    GeoCoords(
    Longitude::degrees(pi.parser.get_named_attribute_value("minlon").to_float()),
    Latitude::degrees(pi.parser.get_named_attribute_value("minlat").to_float()))).x,    pi.geomap->geo_to_world(GeoCoords(
    Longitude::degrees(pi.parser.get_named_attribute_value("minlon").to_float()),
    Latitude::degrees(pi.parser.get_named_attribute_value("minlat").to_float()))).y);

        printf ("bounds pos %.6f %.6f \n", pi.geomap->geo_to_world (    GeoCoords(
    Longitude::degrees(pi.parser.get_named_attribute_value("maxlon").to_float()),
    Latitude::degrees(pi.parser.get_named_attribute_value("maxlat").to_float()))).x,    pi.geomap->geo_to_world(GeoCoords(
    Longitude::degrees(pi.parser.get_named_attribute_value("maxlon").to_float()),
    Latitude::degrees(pi.parser.get_named_attribute_value("maxlat").to_float()))).y);
}

void OSMParser::parse_node(ParserInfo & pi, Dictionary& d) {
    d["pos"] = pi.geomap->geo_to_world(GeoCoords(Longitude::degrees(pi.parser.get_named_attribute_value("lon").to_float()),
                                                 Latitude ::degrees(pi.parser.get_named_attribute_value("lat").to_float())));
}

unsigned int OSMParser::get_element_tile(const String& element_type, ParserInfo& pi, Dictionary& element) {
    if (element_type == "node")
        return pi.geomap->grid_index (element["pos"]);
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