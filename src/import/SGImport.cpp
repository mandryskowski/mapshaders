#include "SGImport.h"
#include "osm_parser/OSMParser.h"
#include "elevation/ElevationParser.h"
#include "coastline/CoastlineParser.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/ref.hpp>
using namespace godot;

void SGImport::import_osm(bool) {

    
    WARN_PRINT("Trying to import osm file" + filename);
    OSMParser parser(node_path_array_to_node_array(this->shader_nodes_osm));

    this->set_geo_map(parser.import(filename, this->geomap, this->heightmap)); 
    WARN_PRINT("Geomap origin " + String::num_real(this->geomap->get_geo_origin_latitude_degrees()) + " " + String::num_real(this->geomap->get_geo_origin_longitude_degrees()));
    WARN_PRINT("Example mapping " + String::num_real(this->geomap->geo_to_world(GeoCoords(Longitude::degrees(15.0), Latitude::degrees(10.0))).x) + " " + String::num_real(this->geomap->geo_to_world(GeoCoords(Longitude::degrees(15.0), Latitude::degrees(10.0))).y));
}

void SGImport::import_elevation(bool) {
    WARN_PRINT("Trying to import elevation file " + this->elevation_filename);
    ElevationParser parser(node_path_array_to_node_array(this->shader_nodes_elevation));

    Ref<ElevationGrid> grid = parser.import(this->elevation_filename, this->geomap);
    this->geomap = grid->get_geo_map();
    this->heightmap = godot::Ref<OSMHeightmap>(memnew(ElevationHeightmap(grid)));
}

void SGImport::import_coastline(bool) {
    WARN_PRINT("Trying to import coastline file. Geomap origin " + String::num_real(this->geomap->get_geo_origin_latitude_degrees()));
    CoastlineParser parser(node_path_array_to_node_array(this->shader_nodes_coastline));
    parser.import("maps/water_polygons", this->geomap, this->coastline_tile_size);
}

void SGImport::reset_geo_info(bool) {
    this->geomap.unref();
    this->heightmap.unref();
}

void SGImport::load_tile(unsigned int index) {
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
    for (int i = 0; i < shader_nodes_osm.size(); i++) {
        WARN_PRINT("Tile " + String::num_int64(index) + " offset is " + String::num_int64(fa->get_position()));
        if (fa->get_8() == 1)
            continue;
        get_node<Node>(shader_nodes_osm[i])->call("load_tile", fa);
    }

    fa->close();
}

void SGImport::load_tiles(bool) {
    for (int i = 0; i < 5 * 5; i++) {
        load_tile (i);
    }
}

void SGImport::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_origin_longitude"), &SGImport::get_origin_longitude);
	ClassDB::bind_method(D_METHOD("set_origin_longitude", "longitude"), &SGImport::set_origin_longitude);

    ClassDB::bind_method(D_METHOD("get_filename"), &SGImport::get_filename);
    ClassDB::bind_method(D_METHOD("set_filename", "filename"), &SGImport::set_filename);

    ClassDB::bind_method(D_METHOD("get_elevation_filename"), &SGImport::get_elevation_filename);
    ClassDB::bind_method(D_METHOD("set_elevation_filename", "filename"), &SGImport::set_elevation_filename);

    ClassDB::bind_method(D_METHOD("import_osm", "plsrefactor"), &SGImport::import_osm);
    ClassDB::bind_method(D_METHOD("import_elevation", "plsrefactor"), &SGImport::import_elevation);
    ClassDB::bind_method(D_METHOD("import_coastline", "plsrefactor"), &SGImport::import_coastline);
    ClassDB::bind_method(D_METHOD("get_true"), &SGImport::get_true);

    ClassDB::bind_method(D_METHOD("get_shader_nodes_osm"), &SGImport::get_shader_nodes_osm);
    ClassDB::bind_method(D_METHOD("set_shader_nodes_osm", "shader_nodes"), &SGImport::set_shader_nodes_osm);

    ClassDB::bind_method(D_METHOD("get_shader_nodes_elevation"), &SGImport::get_shader_nodes_elevation);
    ClassDB::bind_method(D_METHOD("set_shader_nodes_elevation", "shader_nodes"), &SGImport::set_shader_nodes_elevation);

    ClassDB::bind_method(D_METHOD("get_shader_nodes_coastline"), &SGImport::get_shader_nodes_coastline);
    ClassDB::bind_method(D_METHOD("set_shader_nodes_coastline", "shader_nodes"), &SGImport::set_shader_nodes_coastline);

    ClassDB::bind_method(D_METHOD("get_geo_map"), &SGImport::get_geo_map);
    ClassDB::bind_method(D_METHOD("set_geo_map", "geomap"), &SGImport::set_geo_map);

    ClassDB::bind_method(D_METHOD("load_tile", "fa"), &SGImport::load_tile);
    ClassDB::bind_method(D_METHOD("load_tiles", "plsrefactor"), &SGImport::load_tiles);

    ClassDB::bind_method(D_METHOD("reset_geo_info"), &SGImport::reset_geo_info);

    ClassDB::bind_method(D_METHOD("set_coastline_tile_size", "coastlineTileSize"), &SGImport::set_coastline_tile_size);
    ClassDB::bind_method(D_METHOD("get_coastline_tile_size"), &SGImport::get_coastline_tile_size);


    ADD_GROUP("OSM", "osm_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "osm_longitude", PROPERTY_HINT_RANGE, "-180,180,0.01"), "set_origin_longitude", "get_origin_longitude");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "osm_filename", PROPERTY_HINT_FILE, "*.osm"), "set_filename", "get_filename");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "osm_import"), "import_osm", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "osm_shader_nodes", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_shader_nodes_osm", "get_shader_nodes_osm");

    ADD_GROUP("Elevation", "elevation_");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "elevation_filename", PROPERTY_HINT_FILE, "*.asc"), "set_elevation_filename", "get_elevation_filename");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "elevation_import"), "import_elevation", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "elevation_nodes_parse", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_shader_nodes_elevation", "get_shader_nodes_elevation");

    ADD_GROUP("Coastline", "coastline_");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "coastline_nodes", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_shader_nodes_coastline", "get_shader_nodes_coastline");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "coastline_import"), "import_coastline", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "coastline_tile_size"), "set_coastline_tile_size", "get_coastline_tile_size");
    
    
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "geo_map", PROPERTY_HINT_RESOURCE_TYPE, "GeoMap,SphereGeoMap"), "set_geo_map", "get_geo_map");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "load_all_tiles"), "load_tiles", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "Reset geo information"), "reset_geo_info", "get_true");
    
   // ADD_SIGNAL()
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude", PROPERTY_HINT_RANGE, "-90,90,0.01"), "set_origin_latitude", "get_origin_latitude");
    
}

godot::Array SGImport::node_path_array_to_node_array(godot::Array node_paths) {
    godot::Array nodes;
    for (int i = 0; i < node_paths.size(); i++) {
        nodes.push_back(get_node<Node>(node_paths[i]));
    }
    return nodes;
}

/*
void SGImportEditor::edit(Object *p_object) {
    SGImport *my_class = Object::cast_to<SGImport>(p_object);

    if (my_class) {
        // Create a button for the editor
        Button *button = memnew(Button);
        button->set_text("My Custom Button");
        // Connect the button to your desired method
        button->connect("pressed", Callable(my_class, "import_osm"));
        
        // Add the button to a VBoxContainer (or other container)
        VBoxContainer *vbox = memnew(VBoxContainer);
        vbox->add_child(button);

        add_control_to_bottom_panel(vbox, "My Custom Class Actions");
    }
}*/