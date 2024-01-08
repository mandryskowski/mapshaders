#include "SGImport.h"
#include "scene/gui/button.h"
#include "scene/gui/box_container.h"
#include "osm_parser/OSMParser.h"

void SGImport::import_osm(bool) {

    
    WARN_PRINT("I am trying to import osm file" + filename);
    printf("importing osm!\n");
    OSMParser parser(*this);

    OSMParser::World world = parser.import(filename);

    //ArrayMesh mesh = world.ways[0];
    
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
    for (int i = 0; i < parser_nodes.size(); i++) {
        get_node(parser_nodes[i])->call("load_tile", fa);
    }

    fa->close();
}

void SGImport::load_tiles(bool) {
    for (int i = 0; i < 25; i++) {
        load_tile (i);
    }
}

void SGImport::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_origin_longitude"), &SGImport::get_origin_longitude);
	ClassDB::bind_method(D_METHOD("set_origin_longitude", "longitude"), &SGImport::set_origin_longitude);

    ClassDB::bind_method(D_METHOD("get_filename"), &SGImport::get_filename);
    ClassDB::bind_method(D_METHOD("set_filename", "filename"), &SGImport::set_filename);

    ClassDB::bind_method(D_METHOD("import_osm", "plsrefactor"), &SGImport::import_osm);
    ClassDB::bind_method(D_METHOD("get_true"), &SGImport::get_true);

    ClassDB::bind_method(D_METHOD("get_nodes"), &SGImport::get_nodes);
    ClassDB::bind_method(D_METHOD("set_nodes", "nodes_parse"), &SGImport::set_nodes);

    ClassDB::bind_method(D_METHOD("load_tile", "fa"), &SGImport::load_tile);
    ClassDB::bind_method(D_METHOD("load_tiles", "plsrefactor"), &SGImport::load_tiles);


    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "longitude", PROPERTY_HINT_RANGE, "-180,180,0.01"), "set_origin_longitude", "get_origin_longitude");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "filename", PROPERTY_HINT_FILE, "*.osm"), "set_filename", "get_filename");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "import_osm"), "import_osm", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "load_all_tiles"), "load_tiles", "get_true");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "nodes_parse", PROPERTY_HINT_ARRAY_TYPE, "NodePath"), "set_nodes", "get_nodes");
    
   // ADD_SIGNAL()
	//ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "latitude", PROPERTY_HINT_RANGE, "-90,90,0.01"), "set_origin_latitude", "get_origin_latitude");
    
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