#ifndef GEOMAP_H
#define GEOMAP_H
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource.hpp>

/*
       LATITUDE
          ^       
          |
          |
          |
----------+---------> LONGITUDE
          |
          |
          |


1 degree of latitude is always the same distance (around 111.1km).
However, 1 degree of longitude is a different distance depending on the latitude.
*/

/* SPACES:
    - Earth space: Our lovely planet represented by geographical coordinates.
    - World space: 3D Euclidean space of our Godot scene represented by Cartesian coordinates.
    - Grid space: Earth data is partitioned into a grid to allow loading just a part of it.
                    Represented by 2D Cartesian coordinates.
                    The origin corresponds to the origin of World space.
                    The grid must have odd dimensions, e.g. 3x3, 7x5, 1x9...
    NOTE:
    World = Godot scene
    World =/= Earth
*/

struct Longitude {
    double value; // in radians

    static Longitude degrees(double degrees) {
        return Longitude(degrees * Math_PI / 180.0);
    }

    static Longitude radians(double radians) {
        return Longitude(radians);
    }

    static Longitude zero() {
        return Longitude(0.0);
    }

    double get_radians() const {
        return value;
    }

    double get_degrees() const {
        return value * 180.0 / Math_PI;
    }

    Longitude operator+(Longitude rhs) const {
        return Longitude(value + rhs.value); 
    }

    Longitude operator-(Longitude rhs) const {
        return Longitude(value - rhs.value); 
    }

    Longitude operator*(double scalar) const {
        return Longitude(value * scalar);
    }

private:
    explicit Longitude(double rad) : value(rad) { /* ASSERT (value >= 0.0 && value < 2pi)*/}
};

struct Latitude {
    double value; // in radians

    static Latitude degrees(double degrees) {
        return Latitude(degrees * Math_PI / 180.0);
    }
    
    static Latitude radians(double radians) {
        return Latitude(radians);
    }

    static Latitude zero() {
        return Latitude(0.0);
    }

    double get_radians() const {
        return value;
    }

    double get_degrees() const {
        return value * 180.0 / Math_PI;
    }

    Latitude operator+(Latitude rhs) const {
        return Latitude(value + rhs.value); 
    }

    Latitude operator-(Latitude rhs) const {
        return Latitude(value - rhs.value); 
    }

    Latitude operator*(double scalar) const {
        return Latitude(value * scalar);
    }

private:
    explicit Latitude(double rad) : value(rad) { /* ASSERT (value <= pi/2 && value >= -pi/2)*/ }
};

struct GeoCoords {
    Longitude lon;
    Latitude lat;
    GeoCoords(Longitude lon = Longitude::radians(0), Latitude lat = Latitude::radians(0)) : lon(lon), lat(lat) {}

    GeoCoords operator+(GeoCoords rhs) const {
        return GeoCoords(lon + rhs.lon, lat + rhs.lat); 
    }

    GeoCoords operator-(GeoCoords rhs) const {
        return GeoCoords(lon - rhs.lon, lat - rhs.lat); 
    }

    GeoCoords operator*(double scalar) const {
        return GeoCoords(lon * scalar, lat * scalar);
    }

    /**
     * @brief Returns a 2D vector representation of the GeoCoords.
     *        Only use if you really need a Vector2 representation (e.g. for Godot array).
     *        Please avoid storing geographical coordinates in Vector2 as it is less readable.
     */
    godot::Vector2 to_vector2_representation() const {
        return godot::Vector2(lon.get_radians(), lat.get_radians());
    }

    /**
     * @brief Converts a 2D vector representation of the GeoCoords back to GeoCoords.
     */
    static GeoCoords from_vector2_representation(godot::Vector2 vec) {
        return GeoCoords(Longitude::radians(vec.x), Latitude::radians(vec.y));
    }
};

class GeoMap : public godot::Resource {
    GDCLASS(GeoMap, godot::Resource);
    // 1 unit is 1 metre.
    #define UNIT_IN_METRES 1.0
    // 1 degree of latitude is 111.139 km.
    #define LATITUDE_DEGREE_IN_METRES 111139.0
public:
    GeoMap(): geo_origin(GeoCoords()), grid_element_size(godot::Vector2(1000.0, 1000.0)), grid_negative_corner(godot::Vector2(0.0, 0.0)), scale_factor(1.0) {}
    /* Uses bounds in Earth space to calculate the Earth space origin and
       grid space negative corner.*/
    GeoMap (const GeoCoords& min_bounds, const GeoCoords& max_bounds, const godot::Vector2& grid_element_size = godot::Vector2 (1000.0, 1000.0));
    /* Calculates the grid element size (e.g. 1000mx1000m) from grid size (e.g. 3x3 grid). */
    GeoMap (const GeoCoords& min_bounds, const GeoCoords& max_bounds, int64_t grid_size);

    /* Converts World space coordinates into the index of the corresponding
       grid element which can be used for serialisation. */
    unsigned int grid_index (godot::Vector2 world_space);

    /* Converts Earth space coordinates into World space. */
    __declspec(dllexport) virtual godot::Vector3 geo_to_world (GeoCoords);
    __declspec(dllexport) godot::Vector3 geo_to_world (godot::Vector2 vec) {
        return geo_to_world(GeoCoords::from_vector2_representation(vec));
    };

    __declspec(dllexport) virtual godot::Vector3 geo_to_world_up (GeoCoords) {
        return godot::Vector3(0.0, 1.0, 0.0);
    }
    __declspec(dllexport) godot::Vector3 geo_to_world_up (godot::Vector2 vec) {
        return geo_to_world_up(GeoCoords::from_vector2_representation(vec));
    };
    virtual ~GeoMap() {}	

protected:
    static void _bind_methods();
public:
    void set_geo_origin_longitude_degrees(double degrees) {
        geo_origin.lon = Longitude::degrees(degrees);
    }
    double get_geo_origin_longitude_degrees() const {
        return geo_origin.lon.get_degrees();
    }

    void set_geo_origin_latitude_degrees(double degrees) {
        geo_origin.lat = Latitude::degrees(degrees);
    }
    double get_geo_origin_latitude_degrees() const {
        return geo_origin.lat.get_degrees();
    }

    void set_scale_factor(double scale_factor) {
        this->scale_factor = scale_factor;
    }
    double get_scale_factor() const {
        return scale_factor;
    }
    
private:
    /* Converts World space coordinates into Grid space. */
    godot::Vector2 world_to_grid (godot::Vector2 world_space) const
    {
        return world_space / grid_element_size;
    }
    godot::Vector2 grid_size () const {
        return grid_negative_corner.abs() * 2.0;
    }
    /* Geo coordinates of the place on Earth corresponding to (0, 0) in world space. */
    GeoCoords geo_origin;
    /* Size of each grid element, in World space. */
    const godot::Vector2 grid_element_size;
    /*  Most negative corner in Grid space. E.g. a 3x3 grid has a 
        negative corner of (-1.5, -1.5). */
    godot::Vector2 grid_negative_corner;

    /* Scale factor for World space. */
    double scale_factor;
};

class SphereGeoMap : public GeoMap {
    GDCLASS(SphereGeoMap, GeoMap);

public:
    /* Converts Earth space coordinates into World space. */
    __declspec(dllexport) virtual godot::Vector3 geo_to_world (GeoCoords) override;
    __declspec(dllexport) virtual godot::Vector3 geo_to_world_up (GeoCoords) override;

    virtual ~SphereGeoMap() {}

protected:
    static void _bind_methods() {}
};

#endif // GEOMAP_H