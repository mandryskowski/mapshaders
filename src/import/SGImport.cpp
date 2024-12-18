#include "SGImport.h"
#include "osm_parser/OSMParser.h"
#include "elevation/ElevationParser.h"
#include "coastline/CoastlineParser.h"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/ref.hpp>
using namespace godot;

void SGImport::import(bool) {
    auto output_file = get_parser_output_file();

    for (int i = 0; i < this->parsers.size(); i++) {
        Ref<ParserOutputFileHandle> output_file_handle = memnew(ParserOutputFileHandle(output_file, i));
        Ref<Parser> parser = this->parsers[i];
        if (Ref<OSMParser> osm_parser = Object::cast_to<OSMParser>(parser.ptr()); osm_parser.is_valid()) {
            auto returned_geomap = osm_parser->import(output_file_handle, this->geomap, this->heightmap);
    
            if (!this->geomap.is_valid()) {
                this->set_geo_map(returned_geomap); 
            }
        }
        else if (Ref<ElevationParser> elevation_parser = Object::cast_to<ElevationParser>(parser.ptr()); elevation_parser.is_valid()) {
            auto returned_grid = elevation_parser->import(this->geomap);
            if (!this->geomap.is_valid()) {
                this->set_geo_map(returned_grid->get_geo_map());
            }
            this->heightmap = godot::Ref<OSMHeightmap>(memnew(ElevationHeightmap(returned_grid)));
        }
        else if (Ref<CoastlineParser> coastline_parser = Object::cast_to<CoastlineParser>(parser.ptr()); coastline_parser.is_valid()) {
            coastline_parser->import(this->geomap);
        }
    }

    output_file->save();
}

void SGImport::import_osm(bool) {
    auto output_file = get_parser_output_file();
    for (int i = 0; i < this->parsers.size(); i++) {
        Ref<Parser> parser = this->parsers[i];
        Ref<OSMParser> osm_parser = Object::cast_to<OSMParser>(parser.ptr());
        if (osm_parser.is_valid()) {
            Ref<ParserOutputFileHandle> output_file_handle = memnew(ParserOutputFileHandle(output_file, i));
            auto returned_geomap = osm_parser->import(output_file_handle, this->geomap, this->heightmap);
    
            if (!this->geomap.is_valid()) {
                this->set_geo_map(returned_geomap); 
            }
        }
    }

    output_file->save();
}

void SGImport::import_elevation(bool) {
    for (int i = 0; i < this->parsers.size(); i++) {
        Ref<Parser> parser = this->parsers[i];
        Ref<ElevationParser> elevation_parser = Object::cast_to<ElevationParser>(*parser);
        if (elevation_parser.is_valid()) {
            auto returned_grid = elevation_parser->import(this->geomap);
            if (!this->geomap.is_valid()) {
                this->set_geo_map(returned_grid->get_geo_map());
            }
            this->heightmap = godot::Ref<OSMHeightmap>(memnew(ElevationHeightmap(returned_grid)));
        }
    }
}

void SGImport::import_coastline(bool) {
    for (int i = 0; i < this->parsers.size(); i++) {
        Ref<Parser> parser = this->parsers[i];
        Ref<CoastlineParser> coastline_parser = Object::cast_to<CoastlineParser>(parser.ptr());
        if (coastline_parser.is_valid()) {
            coastline_parser->import(this->geomap);
        }
    }
}

void SGImport::load_tile( unsigned int index ) {
    auto output_file = get_parser_output_file();
    Ref<FileAccess> fa = output_file->load_tile_fa(index);

    if (!fa.is_valid())
        return;

    for (int i = 0; i < parsers.size(); i++) {
        TypedArray<Node> shader_nodes = Object::cast_to<Parser>(parsers[i])->get_shader_nodes();
        for (int j = 0; j < shader_nodes.size(); j++) {
            WARN_PRINT("Tile " + String::num_int64(index) + " offset is " + String::num_int64(fa->get_position()));
            if (fa->get_8() == 1)
                continue;
            Object::cast_to<Node>(shader_nodes[j])->call("load_tile", fa);
        }
    }

    fa->close();
}

void SGImport::load_tiles( bool use_threading ) {
    auto output_file = get_parser_output_file();
    unsigned int tile_count = output_file->load_tile_count();

    if (!use_threading) {
        for (int i = 0; i < tile_count; i++) {
            load_tile(i);
        }
        return;
    }
    
    std::vector<std::thread*> threads;
    
    for (int i = 0; i < tile_count; i++) {
        threads.push_back(new std::thread(&SGImport::load_tile, this, i));
    }
}

void SGImport::reset_geo_info(bool) {
    this->geomap.unref();
    this->heightmap.unref();
}

void SGImport::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import", "plsrefactor"), &SGImport::import);
    ClassDB::bind_method(D_METHOD("import_osm", "plsrefactor"), &SGImport::import_osm);
    ClassDB::bind_method(D_METHOD("import_elevation", "plsrefactor"), &SGImport::import_elevation);
    ClassDB::bind_method(D_METHOD("import_coastline", "plsrefactor"), &SGImport::import_coastline);

    ClassDB::bind_method(D_METHOD("load_tile", "index"), &SGImport::load_tile);
    ClassDB::bind_method(D_METHOD("load_tiles", "use_threading"), &SGImport::load_tiles);

    ClassDB::bind_method(D_METHOD("get_true"), &SGImport::get_true);

    ClassDB::bind_method(D_METHOD("set_output_filename", "filename"), &SGImport::set_output_filename);
    ClassDB::bind_method(D_METHOD("get_output_filename"), &SGImport::get_output_filename);

    ClassDB::bind_method(D_METHOD("get_geo_map"), &SGImport::get_geo_map);
    ClassDB::bind_method(D_METHOD("set_geo_map", "geomap"), &SGImport::set_geo_map);

    ClassDB::bind_method(D_METHOD("reset_geo_info"), &SGImport::reset_geo_info);

    ClassDB::bind_method(D_METHOD("set_parsers", "parsers"), &SGImport::set_parsers);
    ClassDB::bind_method(D_METHOD("get_parsers"), &SGImport::get_parsers);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "geo_map", PROPERTY_HINT_RESOURCE_TYPE, "EquirectangularGeoMap,SphereGeoMap"), "set_geo_map", "get_geo_map");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "Reset geo information"), "reset_geo_info", "get_true");

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "output_filename", PROPERTY_HINT_FILE, "*.sgdmap"), "set_output_filename", "get_output_filename");

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "import"), "import", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "import_osm"), "import_osm", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "import_elevation"), "import_elevation", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "import_coastline"), "import_coastline", "get_true");

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "load_tiles"), "load_tiles", "get_true");


    
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "parsers", PROPERTY_HINT_TYPE_STRING, vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "Parser"), PROPERTY_HINT_ARRAY_TYPE), "set_parsers", "get_parsers");
   // ADD_SIGNAL()
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude", PROPERTY_HINT_RANGE, "-90,90,0.01"), "set_origin_latitude", "get_origin_latitude");
    
}

Ref<ParserOutputFile> SGImport::get_parser_output_file() {
    std::vector<unsigned int> parsers_in_slot;
    for (int i = 0; i < this->parsers.size(); i++) {
        parsers_in_slot.push_back(Object::cast_to<Parser>(this->parsers[i])->get_shader_nodes().size());
    }
    return memnew(ParserOutputFile(output_filename, parsers_in_slot));
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