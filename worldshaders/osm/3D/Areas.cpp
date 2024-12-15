#include "Areas.h"
#include "../../../src/util/PolyUtil.h"
#include "../../../src/import/SGImport.h"
#include "../../common/3D/RenderUtil3D.h"

using namespace godot;

void Areas::import_begin() {
    tile_info = Dictionary();
    node_pos = HashMap<int64_t, GeoCoords>();
    ways = Dictionary();
}

void Areas::import_node( godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa ) {
    node_pos[d["id"]] = GeoCoords(Longitude::radians(d["pos_geo_lon"]), Latitude::radians(d["pos_geo_lat"]));
}

void Areas::import_way( godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa ) {
    if (!tile_info.has(fa)) {
        Dictionary fa_d;
        fa_d["ways"] = Dictionary();
        fa_d["rels"] = Array();
        tile_info[fa] = fa_d;
    }

    static_cast<Dictionary>(static_cast<Dictionary>(tile_info[fa])["ways"])[d["id"]] = d;
    ways[d["id"]] = d;
}


void Areas::import_relation( godot::Dictionary d, godot::Ref<godot::StreamPeerBuffer> fa ) {
    std::vector<GeoCoords> outer;
    std::vector<std::vector<GeoCoords>> inners;

    {
        Array members = d["members"];
        for (int i = 0; i < members.size(); i++) {
            Dictionary member = members[i];
            std::vector<GeoCoords> verts;
            if (!ways.has(member["id"]))
                return;
            Array way = static_cast<Dictionary>(ways[member["id"]])["nodes"];

            for (int j = 0; j < way.size(); j++) {
                verts.push_back(node_pos[way[j]]);
            }

            if (member["role"] == "outer")
                outer = verts;
            else if (member["role"] == "inner")
                inners.push_back(verts);
        }
    }

    if (outer.empty())
        return;

    PolyUtil *pu = memnew(PolyUtil);
    Dictionary outd;

    GeoMap* geomap = Object::cast_to<SGImport>(get_parent())->get_geo_map().ptr();
    outd["rooftris"] = RenderUtil3D::geo_polygon_to_world(pu->triangulate_with_holes(outer, inners), geomap);
    /*outd["outer"] = geo_polygon_to_world(outer, geomap);
    outd["inners"] = Array();
    for (const auto &inner : inners) {
        static_cast<Array>(outd["inners"]).append(geo_polygon_to_world(inner, geomap));
    }*/
    outd["name"] = d.get("name", static_cast<String>(d["id"]));
    if (d.has("height"))
        outd["max_height"] = d["height"];
    if (d.has("min_height"))
        outd["min_height"] = d["min_height"];
    
    static_cast<Array>(static_cast<Dictionary>(tile_info[fa])["rels"]).append(outd);
}

void Areas::import_finished() {
    for (int i = 0; i < tile_info.size(); i++) {
        godot::Ref<godot::StreamPeerBuffer> fa = static_cast<godot::Ref<godot::StreamPeerBuffer>>(tile_info.keys()[i]);
        fa->put_var(tile_info[fa]);
    }

}




void Areas::load_tile( godot::Ref<godot::FileAccess> fa ) {
    Array rels = static_cast<Dictionary>(fa->get_var())["rels"];
    for (int i = 0; i < rels.size(); i++) {
        Dictionary rel = rels[i];
        if (!rel.has("rooftris"))
            continue;
        
        Node* hole_area = RenderUtil3D::achild(this, memnew(Node), "HoleArea" + (String)rel["name"]);
        PackedVector3Array rooftris = static_cast<PackedVector3Array>(rel["rooftris"]);

        double min_height = rel.get("min_height", 0.0);
        double max_height = rel.get("max_height", 0.0);

        RenderUtil3D::area_poly(hole_area, "Roof" + (String)rel["name"], RenderUtil3D::polygon_triangles(rooftris, max_height, nullptr), Color(1,1,1));

        if (max_height - min_height > 0.0) {
            // walls
        }
    }

}

void Areas::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import_begin"), &Areas::import_begin);
    ClassDB::bind_method(D_METHOD("import_node", "d", "fa"), &Areas::import_node);
    ClassDB::bind_method(D_METHOD("import_way", "d", "fa"), &Areas::import_way);
    ClassDB::bind_method(D_METHOD("import_relation", "d", "fa"), &Areas::import_relation);
    ClassDB::bind_method(D_METHOD("import_finished"), &Areas::import_finished);
    ClassDB::bind_method(D_METHOD("load_tile", "fa"), &Areas::load_tile);
}
