#ifndef MAPSHADERS_OSMMODEL_H
#define MAPSHADERS_OSMMODEL_H
#include <cstdint>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <string>
#include <unordered_map>

#include "../GeoMap.h"
#include "OSMHeightmap.h"

class OSMNode {
 public:
  OSMNode(uint64_t id, const GeoCoords& coords,
          const std::unordered_map<std::string, std::string>& tags,
          GeoMap* geomap, OSMHeightmap* heightmap)
      : id(id),
        coords(coords),
        tags(tags),
        geomap(geomap),
        heightmap(heightmap) {}
  uint64_t get_id() const { return id; }
  double get_lon_rad() const { return coords.lon.value; }
  double get_lat_rad() const { return coords.lat.value; }
  GeoCoords get_coords() const { return coords; }

  bool has_tag(const godot::String& key) const {
    return tags.find(key.ascii().ptr()) != tags.end();
  }
  godot::String get_tag(const godot::String& key) const {
    std::string key_str = key.ascii().ptr();
    return tags.find(key_str) != tags.end() ? tags.at(key_str).c_str() : "";
  }

  godot::Vector3 get_up() const { return geomap->geo_to_world_up(coords); }
  godot::Vector3 get_pos_flat() const { return geomap->geo_to_world(coords); }
  godot::Vector3 get_pos() const {
    return heightmap
               ? get_pos_flat() + get_up() * heightmap->getElevation(coords)
               : get_pos_flat();
  }

  double get_height() const {
    return heightmap ? heightmap->getElevation(coords) : 0.0;
  }
  OSMHeightmap* get_heightmap() const { return heightmap; }

 private:
  OSMNode()
      : id(std::numeric_limits<uint64_t>::max()),
        geomap(nullptr),
        heightmap(nullptr) {}
  friend class OSMNodeGD;

  uint64_t id;
  GeoCoords coords;
  std::unordered_map<std::string, std::string> tags;
  GeoMap* geomap;
  OSMHeightmap* heightmap;
};

class OSMWay {
 public:
  OSMWay(uint64_t id, std::vector<uint64_t> nodes,
         const std::unordered_map<std::string, std::string>& tags)
      : id(id), nodes(nodes), tags(tags) {}

  uint64_t get_id() const { return id; }

  std::vector<uint64_t> get_nodes() const { return nodes; }

  bool has_tag(const godot::String& key) const {
    return tags.find(key.ascii().ptr()) != tags.end();
  }
  godot::String get_tag(const godot::String& key) const {
    std::string key_str = key.utf8().get_data();
    return tags.find(key_str) != tags.end() ? tags.at(key_str).c_str() : "";
  }

 private:
  OSMWay() : id(std::numeric_limits<uint64_t>::max()) {}
  friend class OSMWayGD;

  uint64_t id;
  std::vector<uint64_t> nodes;
  std::unordered_map<std::string, std::string> tags;
};

class OSMRelation {
 public:
  enum class MemberType { Node, Way, Relation };
  struct Reference {
    uint64_t id;
    std::string role;
    MemberType type;

    Reference(uint64_t id, const std::string& role, MemberType type)
        : id(id), role(role), type(type) {}
  };

  OSMRelation(uint64_t id, std::vector<Reference> refs,
              const std::unordered_map<std::string, std::string>& tags)
      : id(id), refs(refs), tags(tags) {}

  uint64_t get_id() const { return id; }

  std::vector<Reference> get_refs() const { return refs; }
  bool has_tag(const godot::String& key) const {
    return tags.find(key.ascii().ptr()) != tags.end();
  }
  godot::String get_tag(const godot::String& key) const {
    std::string key_str = key.utf8().get_data();
    return tags.find(key_str) != tags.end() ? tags.at(key_str).c_str() : "";
  }

 private:
  OSMRelation() : id(std::numeric_limits<uint64_t>::max()) {}
  friend class OSMRelationGD;

  uint64_t id;
  std::vector<Reference> refs;
  std::unordered_map<std::string, std::string> tags;
};
#endif  // MAPSHADERS_OSMMODEL_H