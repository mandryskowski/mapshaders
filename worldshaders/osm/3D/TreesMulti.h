#ifndef WORLDSHADERS_TREESMULTI_H
#define WORLDSHADERS_TREESMULTI_H
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/templates/hash_map.hpp>


class TreesMulti : public godot::MultiMeshInstance3D {
  GDCLASS(TreesMulti, godot::MultiMeshInstance3D)

 public:
  void import_begin();
  void import_node(godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa);
  void import_finished();

  void load_tile(godot::Ref<godot::FileAccess> fa);

 protected:
  static void _bind_methods();

 private:
  godot::Dictionary tile_info;
  godot::Array transforms;
};

#endif  // WORLDSHADERS_TREESMULTI_H