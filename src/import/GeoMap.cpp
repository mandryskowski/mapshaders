#include "GeoMap.h"

#include "stdio.h"


using namespace godot;

/* Base */
void GeoMap::_bind_methods() {
  ClassDB::bind_method(D_METHOD("geo_to_world", "coords"),
                       (Vector3(GeoMap::*)(Vector2))(&GeoMap::geo_to_world));
  ClassDB::bind_method(D_METHOD("geo_to_world_up", "coords"),
                       (Vector3(GeoMap::*)(Vector2))(&GeoMap::geo_to_world_up));

  ClassDB::bind_method(D_METHOD("set_scale_factor", "scale"),
                       &GeoMap::set_scale_factor);
  ClassDB::bind_method(D_METHOD("get_scale_factor"), &GeoMap::get_scale_factor);

  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "scale"), "set_scale_factor",
               "get_scale_factor");
}

/* Origin based */

OriginBasedGeoMap::OriginBasedGeoMap(const GeoCoords &min_bounds,
                                     const GeoCoords &max_bounds)
    : geo_origin((max_bounds + min_bounds) * 0.5) {}

void OriginBasedGeoMap::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_geo_origin_longitude_degrees", "degrees"),
                       &OriginBasedGeoMap::set_geo_origin_longitude_degrees);
  ClassDB::bind_method(D_METHOD("get_geo_origin_longitude_degrees"),
                       &OriginBasedGeoMap::get_geo_origin_longitude_degrees);

  ClassDB::bind_method(D_METHOD("set_geo_origin_latitude_degrees", "degrees"),
                       &OriginBasedGeoMap::set_geo_origin_latitude_degrees);
  ClassDB::bind_method(D_METHOD("get_geo_origin_latitude_degrees"),
                       &OriginBasedGeoMap::get_geo_origin_latitude_degrees);

  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "geo_origin_longitude_degrees"),
               "set_geo_origin_longitude_degrees",
               "get_geo_origin_longitude_degrees");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "geo_origin_latitude_degrees"),
               "set_geo_origin_latitude_degrees",
               "get_geo_origin_latitude_degrees");
}

/* Equirectangular */

Latitude flat_distance_to_latitude(double distance) {
  return Latitude::degrees(distance / LATITUDE_DEGREE_IN_METRES);
}

double latitude_to_flat_distance(Latitude lat) {
  return LATITUDE_DEGREE_IN_METRES * lat.value * 180.0 / Math_PI;
}

Longitude flat_distance_to_longitude(double distance, Latitude avg_latitude) {
  return Longitude::degrees(
      distance / (LATITUDE_DEGREE_IN_METRES * cos(avg_latitude.value)));
}

double longitude_to_flat_distance(Longitude lon, Latitude avg_latitude) {
  return LATITUDE_DEGREE_IN_METRES * cos(avg_latitude.value) *
         (lon.value * 180.0 / Math_PI);
}
Vector2 geocoords_to_flat_distance(GeoCoords coords, Latitude avg_latitude) {
  return Vector2(
      static_cast<real_t>(longitude_to_flat_distance(coords.lon, avg_latitude) /
                          UNIT_IN_METRES),
      static_cast<real_t>(latitude_to_flat_distance(coords.lat) /
                          UNIT_IN_METRES));
}

MAPSHADERS_DLL_SYMBOL GeoCoords
EquirectangularGeoMap::world_to_geo(const godot::Vector3 &world) {
  return geo_origin +
         GeoCoords(flat_distance_to_longitude(world.x, geo_origin.lat),
                   flat_distance_to_latitude(-world.z));
}

Vector3 EquirectangularGeoMap::geo_to_world_impl(GeoCoords coords) {
  // Note: the Y axis is flipped due to godot's righthandedness
  GeoCoords relative_coords(coords.lon - geo_origin.lon,
                            geo_origin.lat - coords.lat);
  Vector2 world_coords =
      geocoords_to_flat_distance(relative_coords, geo_origin.lat);

  return Vector3(world_coords.x, 0.0, world_coords.y);
}

void EquirectangularGeoMap::_bind_methods() {}

/* Sphere */

godot::Vector3 SphereGeoMap::geo_to_world_impl(GeoCoords coords) {
  // Geographical coordinates to position on a sphere

  const double EARTH_RADIUS_METERS = 6371000.0;

  // Calculate 3D coordinates on the sphere
  // Flip the longitude as otherwise it is "inside out"
  double x = EARTH_RADIUS_METERS * std::cos(coords.lat.get_radians()) *
             std::cos(-coords.lon.get_radians());
  double y = EARTH_RADIUS_METERS *
             std::sin(coords.lat.get_radians());  // Y is the vertical axis
  double z = EARTH_RADIUS_METERS * std::cos(coords.lat.get_radians()) *
             std::sin(-coords.lon.get_radians());

  return godot::Vector3(x, y, z);
}

godot::Vector3 SphereGeoMap::geo_to_world_up(GeoCoords coords) {
  return geo_to_world(coords).normalized();
}

void SphereGeoMap::_bind_methods() {}