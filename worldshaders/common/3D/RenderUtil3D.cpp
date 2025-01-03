#include "RenderUtil3D.h"

#include <algorithm>
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <iostream>

#include "godot_cpp/variant/packed_float64_array.hpp"


using namespace godot;

PackedVector3Array RenderUtil3D::geo_polygon_to_world(
    const std::vector<GeoCoords>& coords, GeoMap* geomap) {
  PackedVector3Array out;
  for (const auto& v : coords) {
    out.append(geomap->geo_to_world(v));
  }

  out.reverse();  // TODO: find out why this is needed

  std::cout << "Geo polygon to world: " << out.size() << std::endl;

  return out;
}

godot::PackedVector3Array RenderUtil3D::geo_polygon_to_world_up(
    const std::vector<GeoCoords>& coords, GeoMap* geomap) {
  PackedVector3Array up_world;
  for (int i = 0; i < coords.size(); i++) {
    up_world.append(geomap->geo_to_world_up(coords[i]));
  }

  up_world.reverse();  // TODO: find out why this is needed

  return up_world;
}

Vector3 RenderUtil3D::get_centroid(const godot::PackedVector3Array& verts) {
  Vector3 centroid;
  for (int i = 0; i < verts.size(); i++) {
    centroid += verts[i];
  }

  return centroid / verts.size();
}

bool RenderUtil3D::enforce_winding(godot::PackedVector3Array& verts,
                                   bool clockwise) {
  auto mid = get_centroid(verts);
  double winding_val = 0;
  for (int i = 0; i < verts.size(); i++) {
    auto v1 = verts[i];
    auto v2 = verts[(i + 1) % verts.size()];
    winding_val += (v2.x - v1.x) * (v2.z + v1.z);
  }

  if (winding_val < 0.0 && !clockwise || winding_val > 0.0 && clockwise) {
    verts.reverse();
    return true;
  }
  return false;
}

bool RenderUtil3D::enforce_winding(std::vector<GeoCoords>& coords,
                                   bool clockwise) {
  GeoCoords centroid;
  for (int i = 0; i < coords.size(); i++) {
    centroid = centroid + coords[i] * (1.0 / coords.size());
  }

  double winding_val = 0;
  for (int i = 0; i < coords.size(); i++) {
    auto v1 = coords[i];
    auto v2 = coords[(i + 1) % coords.size()];
    winding_val += (v2.lon - v1.lon).value * (v2.lat + v1.lat).value;
  }

  if ((winding_val < 0.0 && !clockwise) || (winding_val > 0.0 && clockwise)) {
    std::reverse(coords.begin(), coords.end());
    return true;
  }
  return false;
}

Array RenderUtil3D::polygon(const PackedVector3Array& verts, double height) {
  Array arrays = get_array_mesh_arrays(
      {Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL, Mesh::ARRAY_TEX_UV});
  PackedVector2Array nodes_2d;
  for (int i = 0; i < verts.size(); i++) {
    nodes_2d.append(Vector2(verts[i].x, verts[i].z));
  }

  PackedInt32Array triangulated =
      Geometry2D::get_singleton()->triangulate_polygon(nodes_2d);
  if (triangulated.is_empty()) return {};

  for (int i = 0; i < triangulated.size(); i++) {
    Vector3 v = verts[triangulated[i]] + Vector3(0, height, 0);
    arrays[Mesh::ARRAY_VERTEX].call("push_back", v);
    arrays[Mesh::ARRAY_TEX_UV].call(
        "push_back",
        Vector2(verts[triangulated[i]].x, verts[triangulated[i]].z));
    arrays[Mesh::ARRAY_NORMAL].call("push_back", Vector3(0, 1, 0));
  }

  return arrays;
}

void RenderUtil3D::combine_mesh_arrays(godot::Array& dest, godot::Array& src) {
  for (int i = 0; i < dest.size(); i++) {
    if (dest[i].get_type() == Variant::Type::NIL ||
        src[i].get_type() == Variant::Type::NIL)
      continue;
    dest[i].call("append_array", src[i]);
  }
}

Node* RenderUtil3D::achild(Node* parent, Node* child, String name,
                           bool deferred) {
  child->set_name(name);

  if (deferred) {
    parent->call_deferred("add_child", child, true);
    child->call_deferred("set_owner",
                         parent->get_tree()->get_edited_scene_root());
  } else {
    parent->add_child(child);
    child->set_owner(parent->get_tree()->get_edited_scene_root());
  }
  return child;
}

Array RenderUtil3D::wall_poly(godot::PackedVector3Array& verts,
                              double min_height, double max_height) {
  Array arrays = get_array_mesh_arrays(
      {Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL, Mesh::ARRAY_TEX_UV});

  if (verts.size() < 3) return arrays;

  double uv_x_length_so_far = 0.0;
  const Vector3 min_height_vec(0, min_height, 0);
  const Vector3 max_height_vec(0, max_height, 0);

  for (int i = 0; i < verts.size() - 1; i++) {
    Vector3 v_this = verts[i];
    Vector3 v_next = verts[i + 1];

    PackedVector3Array this_verts = {
        // Triangle 1
        v_this + min_height_vec, v_next + min_height_vec,
        v_next + max_height_vec,

        // Triangle 2
        v_this + min_height_vec, v_next + max_height_vec,
        v_this + max_height_vec};

    PackedFloat64Array this_verts_uv_x = {// Triangle 1
                                          0, 1, 1,

                                          // Triangle 2
                                          0, 1, 0};

    arrays[Mesh::ARRAY_VERTEX].call("append_array", this_verts);

    Vector3 normal = (v_next - v_this).cross(Vector3(0, -1, 0));

    // Each vertex has the same normal
    for (int j = 0; j < 6; j++)
      arrays[Mesh::ARRAY_NORMAL].call("append", normal);

    // UV: Keep track of uv x length. Use height coordinate for uv y.
    double edge_len = (v_next - v_this).length();

    for (int j = 0; j < 6; j++)
      arrays[Mesh::ARRAY_TEX_UV].call(
          "append", Vector2(uv_x_length_so_far + this_verts_uv_x[j] * edge_len,
                            this_verts[j].y));

    uv_x_length_so_far += edge_len;
  }

  return arrays;
}

MeshInstance3D* RenderUtil3D::area_poly(Node* parent, String name, Array arrays,
                                        Color color, bool deferred) {
  MeshInstance3D* area = memnew(MeshInstance3D);
  ArrayMesh* mesh = memnew(ArrayMesh);
  if (!arrays.is_empty()) {
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

    StandardMaterial3D* mat = memnew(StandardMaterial3D);
    mat->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
    mat->set_albedo(color);
    mesh->surface_set_material(0, mat);
  }

  area->set_mesh(mesh);
  return Object::cast_to<MeshInstance3D>(achild(parent, area, name, deferred));
}

godot::MeshInstance3D* RenderUtil3D::area_multipoly(
    godot::Node* parent, godot::String name, godot::Array arrays_of_arrays,
    godot::PackedColorArray colors, bool deferred) {
  MeshInstance3D* area = memnew(MeshInstance3D);
  ArrayMesh* mesh = memnew(ArrayMesh);
  for (int i = 0; i < arrays_of_arrays.size(); i++) {
    Array arrays = arrays_of_arrays[i];

    if (!arrays.is_empty()) {
      mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

      StandardMaterial3D* mat = memnew(StandardMaterial3D);
      mat->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
      mat->set_albedo(colors[i]);
      mesh->surface_set_material(i, mat);
    }
  }

  area->set_mesh(mesh);
  return Object::cast_to<MeshInstance3D>(achild(parent, area, name, deferred));
}

Array RenderUtil3D::get_array_mesh_arrays(PackedInt32Array attributes) {
  Array arrays;
  arrays.resize(Mesh::ARRAY_MAX);
  for (int i = 0; i < attributes.size(); i++) {
    switch (attributes[i]) {
      case Mesh::ARRAY_VERTEX:
      case Mesh::ARRAY_NORMAL:
        arrays[attributes[i]] = PackedVector3Array();
        break;
      case Mesh::ARRAY_TANGENT:
        arrays[attributes[i]] = PackedFloat32Array();
        break;
      case Mesh::ARRAY_TEX_UV:
      case Mesh::ARRAY_TEX_UV2:
        arrays[attributes[i]] = PackedVector2Array();
        break;
      case Mesh::ARRAY_INDEX:
        arrays[attributes[i]] = PackedInt32Array();
        break;
    }
  }

  return arrays;
}

Array RenderUtil3D::polygon_triangles(PackedVector3Array triangles,
                                      double height,
                                      PackedVector3Array* normals) {
  Array arrays = get_array_mesh_arrays(
      {Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL, Mesh::ARRAY_TEX_UV});
  for (int i = 0; i < triangles.size(); i++) {
    Vector3 v = triangles[i];
    arrays[Mesh::ARRAY_VERTEX].call("push_back", (v + Vector3(0, height, 0)));
    arrays[Mesh::ARRAY_TEX_UV].call("push_back", (Vector2(v.x, v.z)));
    arrays[Mesh::ARRAY_NORMAL].call(
        "push_back", (normals ? (*normals)[i] : Vector3(0, 1, 0)));
  }

  return arrays;
}