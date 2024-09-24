#ifndef COASTLINE_PARSER_H
#define COASTLINE_PARSER_H
#include "../GeoMap.h"
#include "../Parser.h"

enum class ShapefileFormat {
    UNKNOWN,
    WGS84,
    MERCATOR
};

class CoastlineParser : public Parser {
public:
    using Parser::Parser;
    /**
     * Imports a coastline shapefile.
     * Deduces filenames for the .shp, .shx, and .prj files.
     * 
     * @param filenameWithoutExtension The name of the shapefile without the extension.
     * @param geomap The GeoMap to import the coastline into. If null, the default GeoMap will be used.
     * 
     */
    void import(const godot::String& filenameWithoutExtension, godot::Ref<GeoMap> geomap = nullptr, double xd = 0.0);

    /**
     * Imports a coastline shapefile.
     * 
     * @param shpFilename The name of the .shp file.
     * @param shxFilename The name of the .shx file.
     * @param prjFilename The name of the .prj file.
     * @param geomap The GeoMap to import the coastline into. If null, the default GeoMap will be used.
     * 
     */
    void import(const godot::String& shpFilename, const godot::String& shxFilename, const godot::String& prjFilename, godot::Ref<GeoMap> geomap = nullptr, double xd = 0.0);
};

#endif // COASTLINE_PARSER_H