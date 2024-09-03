#ifndef SGIMPORT_H
#define SGIMPORT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include "GeoMap.h"
#include "osm_parser/OSMHeightmap.h"

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
    }
    godot::String get_filename() {
        return filename;
    }

    void set_elevation_filename(godot::String _filename) {
        this->elevation_filename = _filename;
    }
    godot::String get_elevation_filename() {
        return elevation_filename;
    }

    void set_shader_nodes_osm(godot::Array nodes) {
        this->shader_nodes_osm = nodes;
    }
    godot::Array get_shader_nodes_osm() {
        return shader_nodes_osm;
    }

    void set_shader_nodes_elevation(godot::Array nodes) {
        this->shader_nodes_elevation = nodes;
    }
    godot::Array get_shader_nodes_elevation() {
        return shader_nodes_elevation;
    }

    void set_shader_nodes_coastline(godot::Array nodes) {
        this->shader_nodes_coastline = nodes;
    }
    godot::Array get_shader_nodes_coastline() {
        return shader_nodes_coastline;
    }

    
    void import_osm(bool);
    void import_elevation(bool);
    void import_coastline(bool);

    void reset_geo_info(bool);

    void load_tile(unsigned int);
    void load_tiles(bool);
    bool get_true() {
        return true;
    }

public:
//    SGImport();
protected:
    static void _bind_methods();

    friend class OSMParser;
    friend class ElevationParser;
private:
    godot::Array node_path_array_to_node_array(godot::Array node_path_array);

    double longitude;

    godot::String filename;
    godot::String elevation_filename;
    godot::Array shader_nodes_osm, shader_nodes_elevation, shader_nodes_coastline;
    godot::Ref<GeoMap> geomap;
    godot::Ref<OSMHeightmap> heightmap;
};

#endif // SGIMPORT_H
