#ifndef SGIMPORT_H
#define SGIMPORT_H
#include "scene/main/node.h"
#include "editor/editor_plugin.h"
#include "../element/SGNode.h"

class SGImport : public Node
{
    GDCLASS(SGImport, Node);

    void set_origin_longitude(double lon) {

    }
    double get_origin_longitude() {
        return 0.0;
    }

    void set_filename(String _filename) {
        this->filename = _filename;
        WARN_PRINT("Filename is " + filename);
    }
    String get_filename() {
        return filename;
    }
    void set_nodes(TypedArray<NodePath> nodes) {
        parser_nodes = nodes;
    }
    TypedArray<NodePath> get_nodes() {
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
    String filename;
    TypedArray<NodePath> parser_nodes;
};
/*
class SGImportEditor : public EditorPlugin {
    GDCLASS(SGImport, EditorPlugin);

public:
    void edit(Object *p_object);

}; */

#endif // SGIMPORT_H