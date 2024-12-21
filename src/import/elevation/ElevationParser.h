#ifndef ELEVATION_PARSER_H
#define ELEVATION_PARSER_H
#include "../GeoMap.h"
#include "../Parser.h"
#include "../../util/Util.h"
#include "../util/ParserOutputFile.h"
#include <godot_cpp/variant/typed_array.hpp>

class ElevationGrid : public godot::RefCounted {
    GDCLASS(ElevationGrid, godot::RefCounted);
public:
    void setNcols(int value);
    int getNcols() const;

    void setNrows(int value);
    int getNrows() const;

    void setTopLeftGeo(const GeoCoords& value);
    const GeoCoords& getTopLeftGeo() const;

    void setTopLeftGeoVec(const godot::Vector2& value) {
        topLeftGeo = GeoCoords::from_vector2_representation(value);
    }
    godot::Vector2 getTopLeftGeoVec() const {
        return topLeftGeo.to_vector2_representation();
    }

    void setCellsize(double value);
    double getCellsize() const;

    void setNodataValue(double value);
    double getNodataValue() const;

    void setHeightmap(const godot::Array& value);
    godot::Array getHeightmap() const;

    MAPSHADERS_DLL_SYMBOL godot::Vector3 getBottomLeftWorld() const;
    MAPSHADERS_DLL_SYMBOL godot::Vector3 getBottomRightWorld() const;
    MAPSHADERS_DLL_SYMBOL godot::Vector3 getTopLeftWorld() const;
    MAPSHADERS_DLL_SYMBOL godot::Vector3 getTopRightWorld() const;

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
    godot::TypedArray<godot::PackedFloat64Array> heightmap;

    godot::Ref<GeoMap> geomap;
};

class ElevationParser : public Parser {
    GDCLASS(ElevationParser, Parser);
public:
    using Parser::Parser;
    godot::Ref<ElevationGrid> import(godot::Ref<ParserOutputFileHandle> output_file_handle, godot::Ref<GeoMap> geomap = nullptr);

    void set_filename(const godot::String& value) {
        filename = value;
    }
    godot::String get_filename() const {
        return filename;
    }

    ~ElevationParser() override = default;

protected:
    static void _bind_methods();

private:
    godot::String filename;
};

#endif // ELEVATION_PARSER_H