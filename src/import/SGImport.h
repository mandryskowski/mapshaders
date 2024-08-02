#ifndef SGIMPORT_H
#define SGIMPORT_H

#include "../element/SGNode.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>

class SGImport : public godot::Node
{
    GDCLASS(SGImport, godot::Node);

    void set_origin_longitude(double lon) {

    }
    
    double get_origin_longitude() {
        return 0.0;
    }

    void set_filename(godot::String _filename) {
        this->filename = _filename;
        WARN_PRINT("Filename is " + filename);
    }
    godot::String get_filename() {
        return filename;
    }
    void set_nodes(godot::Array nodes) {
        this->parser_nodes = nodes;
    }
    godot::Array get_nodes() {
        return parser_nodes;
    }

    
    void import_osm(bool);
    void load_tile(unsigned int);
    void load_tiles(bool);
    bool get_true() {
        return true;
    }

public:
//    SGImport();
protected:
    static void _bind_methods();
    //friend class SGImportEditor;
    friend class OSMParser;
private:
    double longitude;
    godot::String filename;
    godot::Array parser_nodes;
};

#endif // SGIMPORT_H
