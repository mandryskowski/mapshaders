#ifndef TILEMAP_H
#define TILEMAP_H
#include <godot_cpp/classes/resource.hpp>
#include "GeoMap.h"

class TileMapBase : public godot::Resource {
    GDCLASS(TileMapBase, godot::Resource);

public:
    TileMapBase(bool use_geo = false) : use_geo(use_geo) {}

    /** 
     * Turn a coordinate in geo space into a tile coordinate.
     * @param coord The geo space coordinate to convert.
     * @return The tile coordinate.
     */
    virtual godot::Vector2i get_tile_geo(GeoCoords) = 0;

    /**
     * Turn a coordinate in geo space into a tile coordinate.
     * @param coord The geo space coordinate to convert (in Vector2 representation).
     * @return The tile coordinate.
     */
    godot::Vector2i get_tile_geo(godot::Vector2);

    virtual godot::TypedArray<godot::Vector2i> get_tiles_of_interest(GeoCoords coords, double elevation, godot::Vector3 front_vec) = 0;


    virtual ~TileMapBase() {}

protected:
    static void _bind_methods() {}
private:

    /* Whether to use geo space. If false, uses world space. */
    bool use_geo;
};

class EquirectangularTileMap : public TileMapBase {
    GDCLASS(EquirectangularTileMap, TileMapBase);

public:
    EquirectangularTileMap(bool use_geo = false) : TileMapBase(use_geo), tile_size(1000.0, 1000.0) {}

    virtual godot::Vector2i get_tile_geo(GeoCoords) override;

    GeoCoords get_tile_top_left_geo(const godot::Vector2i& tile) const;

    virtual godot::TypedArray<godot::Vector2i> get_tiles_of_interest(GeoCoords coords, double elevation, godot::Vector3 front_vec) override;

    static void _bind_methods() {}

private:
    /* In Equirectangular world units. */
    godot::Vector2 tile_size;
};

#endif // TILEMAP_H