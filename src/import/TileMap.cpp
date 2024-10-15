#include "TileMap.h"

using namespace godot;

godot::Vector2i TileMapBase::get_tile_geo(godot::Vector2 coords_vec2) {
    return this->get_tile_geo(GeoCoords::from_vector2_representation(coords_vec2));
}


Vector2i EquirectangularTileMap::get_tile_geo(GeoCoords coords) {
    /* Position in global world space (with origin (0,0)). */
    Vector3 global_world_pos = EquirectangularGeoMap().geo_to_world(coords);
    
    return Vector2i(std::round(global_world_pos.x / tile_size.x), std::round(global_world_pos.z / tile_size.y));
}

godot::TypedArray<godot::Vector2i> EquirectangularTileMap::get_tiles_of_interest(GeoCoords coords, double elevation, godot::Vector3 front_vec)
{
    return godot::TypedArray<godot::Vector2i>();
}
