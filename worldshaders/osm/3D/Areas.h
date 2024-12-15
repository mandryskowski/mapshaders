#ifndef WORLDSHADERS_AREAS_H
#define WORLDSHADERS_AREAS_H
#include "../../../src/import/GeoMap.h"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/classes/file_access.hpp>


class Areas : public godot::Node {
    GDCLASS(Areas, godot::Node)

public:
    void import_begin();
    void import_node(godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa);
    void import_way(godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa);
    void import_relation(godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa);
    void import_finished();

    void load_tile(godot::Ref<godot::FileAccess> fa);

protected:
    static void _bind_methods();

private:
    godot::Dictionary tile_info;
    godot::HashMap<int64_t, GeoCoords> node_pos;
    godot::Dictionary ways;
};

#endif // WORLDSHADERS_AREAS_H