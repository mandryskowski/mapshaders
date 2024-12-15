#include "RenderUtil3D.h"
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <iostream>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/geometry2d.hpp>

using namespace godot;

PackedVector3Array RenderUtil3D::geo_polygon_to_world(const std::vector<GeoCoords>& coords, GeoMap* geomap) {
    PackedVector3Array out;
    for (const auto &v : coords) {
        out.append(geomap->geo_to_world(v));
    }

    std::cout << "Geo polygon to world: " << out.size() << std::endl;

    return out;

}

godot::PackedVector3Array RenderUtil3D::geo_polygon_to_world_up(const std::vector<GeoCoords> &coords, GeoMap *geomap ) {
    PackedVector3Array up_world;
    for (int i = 0; i < coords.size(); i++) {
        up_world.append(geomap->geo_to_world_up(coords[i]));
    }

    return up_world;
}

Vector3 RenderUtil3D::get_centroid( const godot::PackedVector3Array &verts ) {
    Vector3 centroid;
    for (int i = 0; i < verts.size(); i++) {
        centroid += verts[i];
    }

    return centroid / verts.size();
}

bool RenderUtil3D::enforce_winding( godot::PackedVector3Array &verts, bool clockwise) {
    auto mid = get_centroid(verts);
    int winding_val = 0;
    for (int i = 0; i < verts.size(); i++) {
        auto v1 = verts[i];
        auto v2 = verts[(i + 1) % verts.size()];
        winding_val += (v2.x - v1.x) * (v2.z + v1.z);
    }

    if (winding_val < 0 && !clockwise || winding_val > 0 && clockwise) {
        verts.reverse();
        return true;
    }
    return false;
}

Array RenderUtil3D::polygon( const PackedVector3Array &verts, double height ) {
    Array arrays = get_array_mesh_arrays({Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL, Mesh::ARRAY_TEX_UV});
    PackedVector2Array nodes_2d;
    for (int i = 0; i < verts.size(); i++) {
        nodes_2d.append(Vector2(verts[i].x, verts[i].z));
    }

    PackedInt32Array triangulated = Geometry2D::get_singleton()->triangulate_polygon(nodes_2d);
    if (triangulated.is_empty())
        return {};

    for (int i = 0; i < triangulated.size(); i++) {
        Vector3 v = verts[triangulated[i]] + Vector3(0, height, 0);
        arrays[Mesh::ARRAY_VERTEX].call("push_back", v);
        arrays[Mesh::ARRAY_TEX_UV].call("push_back", Vector2(verts[triangulated[i]].x, verts[triangulated[i]].z));
        arrays[Mesh::ARRAY_NORMAL].call("push_back", Vector3(0, 1, 0));
    }

    return arrays;
}

Node* RenderUtil3D::achild(Node* parent, Node* child, String name) {
    child->set_name(name);
    parent->add_child(child);
    child->set_owner(parent->get_tree()->get_edited_scene_root());
    return child;
}

MeshInstance3D* RenderUtil3D::area_poly(Node* parent, String name, Array arrays, Color color) {
    MeshInstance3D* area = Object::cast_to<MeshInstance3D>(achild(parent, memnew(MeshInstance3D), name));
    ArrayMesh *mesh = memnew(ArrayMesh);
    if (!arrays.is_empty()) {
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

        StandardMaterial3D* mat = memnew(StandardMaterial3D);
        mat->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
        mat->set_albedo(color);
        mesh->surface_set_material(0, mat);
    }

    area->set_mesh(mesh);
    return area;
}

Array RenderUtil3D::get_array_mesh_arrays(PackedInt32Array attributes) {
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    for (int i = 0; i < attributes.size(); i++) {
        std::cout << "Attribute: " << attributes[i] << std::endl;
        switch (attributes[i]) {
            case Mesh::ARRAY_VERTEX:
            case Mesh::ARRAY_NORMAL:
                std::cout << "Vertex/normal\n";
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

Array RenderUtil3D::polygon_triangles(PackedVector3Array triangles, double height, PackedVector3Array* normals) {
    Array arrays = get_array_mesh_arrays({Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL, Mesh::ARRAY_TEX_UV});
    for (int i = 0; i < triangles.size(); i++) {
        Vector3 v = triangles[i];
        arrays[Mesh::ARRAY_VERTEX].call("push_back", (v + Vector3(0, height, 0)) );
        arrays[Mesh::ARRAY_TEX_UV].call("push_back", (Vector2(v.x, v.z)));
        arrays[Mesh::ARRAY_NORMAL].call("push_back", (normals ? (*normals)[i] : Vector3(0, 1, 0)));
    }

    return arrays;
}