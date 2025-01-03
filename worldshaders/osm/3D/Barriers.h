#ifndef WORLDSHADERS_BARRIERS_H
#define WORLDSHADERS_BARRIERS_H
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include "../../../src/import/GeoMap.h"
#include "../../../src/import/osm_parser/OSMModelGD.h"

class Barriers : public godot::Node {
  GDCLASS(Barriers, godot::Node)

 public:
  void import_begin();
  void import_node(OSMNodeGD*, godot::Ref<godot::StreamPeerBuffer> fa);
  void import_way(OSMWayGD*, godot::Ref<godot::StreamPeerBuffer> fa);
  void import_finished();

  void load_tile(godot::Ref<godot::FileAccess> fa, GeoMap* geomap);

  void set_barriers_as_separate_nodes(bool value) {
    barriers_as_separate_nodes = value;
  }
  bool get_barriers_as_separate_nodes() const {
    return barriers_as_separate_nodes;
  }

 protected:
  static void _bind_methods();

 private:
  godot::Dictionary tile_info;
  godot::HashMap<int64_t, GeoCoords> node_pos;
  OSMHeightmap* heightmap;
  bool barriers_as_separate_nodes;
};

#endif  // WORLDSHADERS_BARRIERS_H