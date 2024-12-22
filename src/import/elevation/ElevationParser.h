#ifndef ELEVATION_PARSER_H
#define ELEVATION_PARSER_H
#include <godot_cpp/variant/typed_array.hpp>

#include "../../util/Util.h"
#include "../GeoMap.h"
#include "../Parser.h"
#include "../util/ParserOutputFile.h"

/**
 * @brief The ElevationGrid class represents a grid of elevation data.
 *        When working with an elevation grid, you may wish to use 1-cell
 *        padding around the grid to ensure that your elevation tiles are
 *        seamless. With 1-cell padding, the first row and column and the
 *        last row and column are not rendered, but only used to calculate
 * normals and heights.
 */
class ElevationGrid : public godot::RefCounted {
  GDCLASS(ElevationGrid, godot::RefCounted);

 public:
  ElevationGrid(bool one_cell_padding)
      : ncols(0),
        nrows(0),
        topLeftGeo(GeoCoords()),
        cellsize(0.0),
        nodata_value(0.0),
        has_one_cell_padding(one_cell_padding) {}
  ElevationGrid() : ElevationGrid(false) {}
  void setNcols(int value);
  int getNcols(bool one_cell_padding) const;

  void setNrows(int value);
  int getNrows(bool one_cell_padding) const;

  void setTopLeftGeo(const GeoCoords& value);
  GeoCoords getTopLeftGeo(bool one_cell_padding) const;

  void setCellsize(double value);
  double getCellsize() const;

  void setNodataValue(double value);
  double getNodataValue() const;

  void setHeightmap(const godot::Array& value);

  /**
   * @returns The heightmap as a 2D array of doubles. You need to handle padding
   * yourself.
   */
  godot::Array getHeightmap() const;

  godot::Ref<GeoMap> get_geo_map() const { return geomap; }
  void set_geo_map(godot::Ref<GeoMap> value) { geomap = value; }

  double bilinearInterpolation(const GeoCoords& coords) const;

 protected:
  static void _bind_methods();

 private:
  int ncols;             // Number of columns in the grid, assuming no padding
  int nrows;             // Number of rows in the grid, assuming no padding
  GeoCoords topLeftGeo;  // Top left corner of the grid in geo space, assuming
                         // no padding
  double cellsize;
  double nodata_value;
  godot::TypedArray<godot::PackedFloat64Array>
      heightmap;  // Elevation data, assuming no padding

  bool has_one_cell_padding;

  godot::Ref<GeoMap> geomap;
};

class ElevationParser : public Parser {
  GDCLASS(ElevationParser, Parser);

 public:
  using Parser::Parser;
  godot::Ref<ElevationGrid> import(
      godot::Ref<ParserOutputFileHandle> output_file_handle,
      godot::Ref<GeoMap> geomap = nullptr);

  void set_filename(const godot::String& value) { filename = value; }
  godot::String get_filename() const { return filename; }

  ~ElevationParser() override = default;

 protected:
  static void _bind_methods();

 private:
  godot::String filename;
};

#endif  // ELEVATION_PARSER_H