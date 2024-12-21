#ifndef COASTLINE_PARSER_H
#define COASTLINE_PARSER_H
#include "../GeoMap.h"
#include "../Parser.h"

enum class ShapefileFormat { UNKNOWN, WGS84, MERCATOR };

class CoastlineParser : public Parser {
  GDCLASS(CoastlineParser, Parser);

 public:
  using Parser::Parser;
  /**
   * Imports a coastline shapefile.
   *
   * @param geomap The GeoMap to import the coastline into. If null, the default
   * GeoMap will be used.
   *
   */
  void import(godot::Ref<GeoMap> geomap = nullptr);

  void set_size_in_degrees(double value) { size_in_degrees = value; }
  double get_size_in_degrees() { return size_in_degrees; }

  void set_shp_filename(const godot::String& value) { shpFilename = value; }
  godot::String get_shp_filename() { return shpFilename; }
  void set_shx_filename(const godot::String& value) { shxFilename = value; }
  godot::String get_shx_filename() { return shxFilename; }
  void set_prj_filename(const godot::String& value) { prjFilename = value; }
  godot::String get_prj_filename() { return prjFilename; }

  ~CoastlineParser() override = default;

 protected:
  static void _bind_methods();

 private:
  double size_in_degrees;

  godot::String shpFilename;
  godot::String shxFilename;
  godot::String prjFilename;
};

#endif  // COASTLINE_PARSER_H