#ifndef SGIMPORT_H
#define SGIMPORT_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include "GeoMap.h"
#include "Parser.h"
#include "osm_parser/OSMHeightmap.h"

class SGImport : public godot::Node
{
    GDCLASS(SGImport, godot::Node);

    void set_geo_map(godot::Ref<GeoMap> _geomap) {
        this->geomap = _geomap;
    }
    godot::Ref<GeoMap> get_geo_map() {
        return geomap;
    }

    godot::Array get_parsers() {
        return parsers;
    }
    void set_parsers(godot::Array _parsers) {
        parsers = _parsers;
    }

    
    void import_osm(bool);
    void import_elevation(bool);
    void import_coastline(bool);

    void reset_geo_info(bool);

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

    godot::Ref<GeoMap> geomap;
    godot::Ref<OSMHeightmap> heightmap;

    godot::TypedArray<Parser> parsers;
};

#endif // SGIMPORT_H
