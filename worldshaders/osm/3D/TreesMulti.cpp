#include "TreesMulti.h"
#include <godot_cpp/classes/multi_mesh.hpp>

using namespace godot;

void TreesMulti::import_begin() {
    tile_info = Dictionary();
    transforms = Array();
    this->get_multimesh()->set_instance_count(0);
}

void TreesMulti::import_node( godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa ) {
    if (d.get("natural", "") != "tree")
        return;

    if (!tile_info.has(fa)) {
        tile_info[fa] = Array();
    }

    static_cast<Array>(tile_info[fa]).append(d["pos_elevation"]);
}

void TreesMulti::import_finished() {
    for (int i = 0; i < tile_info.size(); i++) {
        godot::Ref<godot::StreamPeerBuffer> fa = static_cast<godot::Ref<godot::StreamPeerBuffer>>(tile_info.keys()[i]);
        fa->put_var(tile_info[fa]);
    }
}

void TreesMulti::load_tile( godot::Ref<godot::FileAccess> fa ) {
    Array positions = fa->get_var();

    
    for (int i = 0; i < positions.size(); i++) {
        transforms.append(Transform3D(Basis().rotated(Vector3(1, 0, 0), 90.0 / 180.0 * Math_PI), Vector3(positions[i])));
    }

    this->get_multimesh()->set_instance_count(transforms.size());
    
    for (int i = 0; i < transforms.size(); i++) {
        this->get_multimesh()->set_instance_transform(i, transforms[i]);
    }
}

void TreesMulti::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import_begin"), &TreesMulti::import_begin);
    ClassDB::bind_method(D_METHOD("import_node", "d", "fa"), &TreesMulti::import_node);
    ClassDB::bind_method(D_METHOD("import_finished"), &TreesMulti::import_finished);
    ClassDB::bind_method(D_METHOD("load_tile", "fa"), &TreesMulti::load_tile);
}
