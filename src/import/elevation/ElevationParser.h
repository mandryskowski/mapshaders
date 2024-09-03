#ifndef ELEVATION_PARSER_H
#define ELEVATION_PARSER_H
#include "../GeoMap.h"
#include "../Parser.h"

class ElevationGrid : public godot::RefCounted {
    GDCLASS(ElevationGrid, godot::RefCounted);
public:
    void setNcols(int value);
    int getNcols() const;

    void setNrows(int value);
    int getNrows() const;

    void setTopLeftGeo(const GeoCoords& value);
    const GeoCoords& getTopLeftGeo() const;

    void setCellsize(double value);
    double getCellsize() const;

    void setNodataValue(double value);
    double getNodataValue() const;

    void setHeightmap(godot::Array value);
    godot::Array getHeightmap() const;

    __declspec(dllexport) godot::Vector2 getBottomLeftWorld() const;
    __declspec(dllexport) godot::Vector2 getBottomRightWorld() const;
    __declspec(dllexport) godot::Vector2 getTopLeftWorld() const;
    __declspec(dllexport) godot::Vector2 getTopRightWorld() const;

    godot::Ref<GeoMap> get_geo_map() const {
        return geomap;
    }
    void set_geo_map(godot::Ref<GeoMap> value) {
        geomap = value;
    }

    double bilinearInterpolation(const GeoCoords & coords) const;

protected: 
    static void _bind_methods();

private:
    int ncols;
    int nrows;
    GeoCoords topLeftGeo;
    double cellsize;
    double nodata_value;
    godot::Array heightmap;

    godot::Ref<GeoMap> geomap;
};

class ElevationParser : public Parser {
public:
    using Parser::Parser;
    godot::Ref<ElevationGrid> import(const godot::String& filename, godot::Ref<GeoMap> geomap = nullptr);
};

#endif // ELEVATION_PARSER_H