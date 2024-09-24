#include "GeoMap.h"
#include "stdio.h"

using namespace godot;

double latitude_to_flat_distance(Latitude lat) {
    return LATITUDE_DEGREE_IN_METRES * lat.value * 180.0 / Math_PI;
}
double longitude_to_flat_distance(Longitude lon, Latitude avg_latitude) {
    return LATITUDE_DEGREE_IN_METRES * cos(avg_latitude.value) * (lon.value * 180.0 / Math_PI);
}
Vector2 geocoords_to_flat_distance(GeoCoords coords, Latitude avg_latitude) {
    return Vector2(longitude_to_flat_distance(coords.lon, avg_latitude) / UNIT_IN_METRES,
                   latitude_to_flat_distance(coords.lat) / UNIT_IN_METRES);
}


GeoMap::GeoMap(const GeoCoords &min_bounds, const GeoCoords &max_bounds, const Vector2 &grid_element_size):
    geo_origin ((max_bounds + min_bounds) * 0.5),
    grid_negative_corner (Vector2(0.0, 0.0)),
    grid_element_size (grid_element_size) {
    grid_negative_corner = world_to_grid (geo_to_world (min_bounds));
}

GeoMap::GeoMap(const GeoCoords &min_bounds, const GeoCoords &max_bounds, int64_t grid_size):
    geo_origin ((max_bounds + min_bounds) * 0.5),
    grid_negative_corner (Vector2(0.0, 0.0)),
    grid_element_size (geocoords_to_flat_distance (max_bounds - min_bounds, (max_bounds.lat + min_bounds.lat) * 0.5) * (1.0 / static_cast<double>(grid_size))) {
    grid_negative_corner = world_to_grid (geo_to_world (min_bounds) * Vector2(1.0, -1.0)); // Y coordinate is flipped due to Godot's righthandedness
    WARN_PRINT("Grid element size: " + String::num_real(grid_element_size.x) + " " + String::num_real(grid_element_size.y));
    WARN_PRINT("Grid negative corner: " + String::num_real(grid_negative_corner.x) + " " + String::num_real(grid_negative_corner.y));
    WARN_PRINT("Grid min bounds in geo space: " + String::num_real(min_bounds.lon.get_degrees()) + " " + String::num_real(min_bounds.lat.get_degrees()));
    WARN_PRINT("Grid min bounds in world space: " + String::num_real(geo_to_world(min_bounds).x) + " " + String::num_real(geo_to_world(min_bounds).y));
    WARN_PRINT("Grid max bounds in geo space: " + String::num_real(max_bounds.lon.get_degrees()) + " " + String::num_real(max_bounds.lat.get_degrees()));
    WARN_PRINT("Grid max bounds in world space: " + String::num_real(geo_to_world(max_bounds).x) + " " + String::num_real(geo_to_world(max_bounds).y));
}

unsigned int GeoMap::grid_index (Vector2 world_space) {
    Vector2 grid_space = world_to_grid (world_space);
    Vector2 relative_to_corner = grid_space - grid_negative_corner;

    return static_cast<unsigned int>(MAX(floor(relative_to_corner.y - CMP_EPSILON), 0.0) * round(grid_size().x)) + static_cast<unsigned int>(MAX(floor(relative_to_corner.x - CMP_EPSILON), 0.0));
}

Vector2 GeoMap::geo_to_world(GeoCoords coords) {
    // Note: the Y axis is flipped due to godot's righthandedness
	GeoCoords relative_coords(coords.lon - geo_origin.lon, geo_origin.lat - coords.lat);
    return geocoords_to_flat_distance (relative_coords, geo_origin.lat);
}

void GeoMap::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_geo_origin_longitude_degrees", "degrees"), &GeoMap::set_geo_origin_longitude_degrees);
    ClassDB::bind_method(D_METHOD("get_geo_origin_longitude_degrees"), &GeoMap::get_geo_origin_longitude_degrees);

    ClassDB::bind_method(D_METHOD("set_geo_origin_latitude_degrees", "degrees"), &GeoMap::set_geo_origin_latitude_degrees);
    ClassDB::bind_method(D_METHOD("get_geo_origin_latitude_degrees"), &GeoMap::get_geo_origin_latitude_degrees);

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "geo_origin_longitude_degrees"), "set_geo_origin_longitude_degrees", "get_geo_origin_longitude_degrees");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "geo_origin_latitude_degrees"), "set_geo_origin_latitude_degrees", "get_geo_origin_latitude_degrees");
}

/*
godot::Vector3 SphereGeoMap::geo_to_world(GeoCoords coords)
{
    // Geographical coordinates to position on a sphere

    const double EARTH_RADIUS_METERS = 6371000.0;

    // Calculate 3D coordinates on the sphere
    double x = EARTH_RADIUS_METERS * std::cos(coords.lat.get_radians()) * std::cos(coords.lon.get_radians());
    double y = EARTH_RADIUS_METERS * sin(coords.lat.get_radians());  // Y is the vertical axis
    double z = EARTH_RADIUS_METERS * cos(coords.lat.get_radians()) * sin(coords.lon.get_radians());

    return godot::Vector3(x, y, z);
}
*/