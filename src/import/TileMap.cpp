#include "TileMap.h"

using namespace godot;

godot::Vector2i TileMapBase::get_tile_geo(godot::Vector2 coords_vec2) {
  return this->get_tile_geo(
      GeoCoords::from_vector2_representation(coords_vec2));
}

Vector2i EquirectangularTileMap::get_tile_geo(GeoCoords coords) {
  /* Position in global world space (with origin (0,0)). */
  Ref<EquirectangularGeoMap> originMap = memnew(EquirectangularGeoMap());
  Vector3 global_world_pos = originMap->geo_to_world(coords);

  return Vector2i(std::round(global_world_pos.x / tile_size.x),
                  std::round(global_world_pos.z / tile_size.y));
}

GeoCoords EquirectangularTileMap::get_tile_top_left_geo(
    const Vector2i &tile) const {
  // Top left corner of the (0, 0) tile in world space.
  auto top_left_0_0_world = Vector3(-tile_size.x / 2.0, 0, tile_size.y / 2.0);
  auto top_left_world = top_left_0_0_world +
                        Vector3(tile.x * tile_size.x, 0, tile.y * tile_size.y);

  Ref<EquirectangularGeoMap> originMap = memnew(EquirectangularGeoMap());
  return originMap->world_to_geo(top_left_world);
}

godot::TypedArray<godot::Vector2i>
EquirectangularTileMap::get_tiles_of_interest(GeoCoords coords,
                                              double elevation,
                                              godot::Vector3 front_vec) {
  return godot::TypedArray<godot::Vector2i>();
}
