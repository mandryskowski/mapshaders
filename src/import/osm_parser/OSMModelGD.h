#ifndef MAPSHADERS_OSMMODELGD_H
#define MAPSHADERS_OSMMODELGD_H
#include <godot_cpp/classes/ref_counted.hpp>

#include "OSMHeightmap.h"
#include "OSMModel.h"
#include "godot_cpp/variant/packed_int64_array.hpp"

class OSMNodeGD : public godot::RefCounted {
  GDCLASS(OSMNodeGD, godot::RefCounted);

 public:
  OSMNodeGD() = default;
  OSMNodeGD(OSMNode&& node) : node(std::move(node)) {}

  uint64_t get_id() const { return node.get_id(); }
  double get_lon_rad() const { return node.get_lon_rad(); }
  double get_lat_rad() const { return node.get_lat_rad(); }
  bool has_tag(const godot::String& key) const { return node.has_tag(key); }
  godot::String get_tag(const godot::String& key) const {
    return node.get_tag(key);
  }
  godot::Vector3 get_up() const { return node.get_up(); }
  godot::Vector3 get_pos_flat() const { return node.get_pos_flat(); }
  godot::Vector3 get_pos() const { return node.get_pos(); }
  double get_height() const { return node.get_height(); }
  OSMHeightmap* get_heightmap() const { return node.get_heightmap(); }

 protected:
  static void _bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_id"), &OSMNodeGD::get_id);
    ClassDB::bind_method(D_METHOD("get_lon_rad"), &OSMNodeGD::get_lon_rad);
    ClassDB::bind_method(D_METHOD("get_lat_rad"), &OSMNodeGD::get_lat_rad);
    ClassDB::bind_method(D_METHOD("get_tag", "key"), &OSMNodeGD::get_tag);
    ClassDB::bind_method(D_METHOD("get_up"), &OSMNodeGD::get_up);
    ClassDB::bind_method(D_METHOD("get_pos_flat"), &OSMNodeGD::get_pos_flat);
    ClassDB::bind_method(D_METHOD("get_pos"), &OSMNodeGD::get_pos);
    ClassDB::bind_method(D_METHOD("get_height"), &OSMNodeGD::get_height);
    ClassDB::bind_method(D_METHOD("get_heightmap"), &OSMNodeGD::get_heightmap);
  }

 private:
  OSMNode node;
};

class OSMWayGD : public godot::RefCounted {
  GDCLASS(OSMWayGD, godot::RefCounted);

 public:
  OSMWayGD() = default;
  OSMWayGD(OSMWay&& way) : way(std::move(way)) {}

  uint64_t get_id() const { return way.get_id(); }
  godot::PackedInt64Array get_nodes() const {
    godot::PackedInt64Array nodes;
    for (const auto& node : way.get_nodes()) {
      nodes.push_back(node);
    }
    return nodes;
  }
  bool has_tag(const godot::String& key) const { return way.has_tag(key); }
  godot::String get_tag(const godot::String& key) const {
    return way.get_tag(key);
  }

 protected:
  static void _bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_id"), &OSMWayGD::get_id);
    ClassDB::bind_method(D_METHOD("get_nodes"), &OSMWayGD::get_nodes);
    ClassDB::bind_method(D_METHOD("get_tag", "key"), &OSMWayGD::get_tag);
  }

 private:
  OSMWay way;
};

class OSMRelationGD : public godot::RefCounted {
  GDCLASS(OSMRelationGD, godot::RefCounted);

 public:
  OSMRelationGD() = default;
  OSMRelationGD(OSMRelation&& relation) : relation(std::move(relation)) {}

  uint64_t get_id() const { return relation.get_id(); }
  godot::Array get_refs() const {
    godot::Array refs;
    for (const auto& ref : relation.get_refs()) {
      godot::Dictionary ref_dict;
      ref_dict["id"] = ref.id;
      ref_dict["role"] = ref.role.c_str();
      ref_dict["type"] = static_cast<int64_t>(ref.type);
      refs.push_back(ref_dict);
    }
    return refs;
  }

  bool has_tag(const godot::String& key) const { return relation.has_tag(key); }
  godot::String get_tag(const godot::String& key) const {
    return relation.get_tag(key);
  }

 protected:
  static void _bind_methods() {
    using namespace godot;
    ClassDB::bind_method(D_METHOD("get_id"), &OSMRelationGD::get_id);
    ClassDB::bind_method(D_METHOD("get_refs"), &OSMRelationGD::get_refs);
    ClassDB::bind_method(D_METHOD("get_tag", "key"), &OSMRelationGD::get_tag);
  }

 private:
  OSMRelation relation;
};

#endif  // MAPSHADERS_OSMMODELGD_H