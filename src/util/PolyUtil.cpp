#include "PolyUtil.h"
#define P2T_STATIC_EXPORTS
#include "../../extern/poly2tri/poly2tri/poly2tri.h"
#include "../../extern/poly2tri/poly2tri/sweep/cdt.h"
#include "../../extern/poly2tri/poly2tri/common/utils.h"
#include "../../extern/polyskel-cpp-port/polyskel.h"
#include "../../extern/polyskel-cpp-port/vec.h"
#include "../../extern/polyskel-cpp-port/lavertex.h"

using namespace godot;

Vector2 SkeletonSubtree::get_source() const {
    return source;
}
void SkeletonSubtree::set_source(const Vector2& value) {
    source = value;
}
double SkeletonSubtree::get_height() const {
    return height;
}
void SkeletonSubtree::set_height(double value) {
    height = value;
}
PackedVector2Array SkeletonSubtree::get_sinks() const {
    return sinks;
}
void SkeletonSubtree::set_sinks(const PackedVector2Array& value) {
    sinks = value;
}

void SkeletonSubtree::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_source"), &SkeletonSubtree::get_source);
    ClassDB::bind_method(D_METHOD("set_source", "value"), &SkeletonSubtree::set_source);
    ClassDB::bind_method(D_METHOD("get_height"), &SkeletonSubtree::get_height);
    ClassDB::bind_method(D_METHOD("set_height", "value"), &SkeletonSubtree::set_height);
    ClassDB::bind_method(D_METHOD("get_sinks"), &SkeletonSubtree::get_sinks);
    ClassDB::bind_method(D_METHOD("set_sinks", "value"), &SkeletonSubtree::set_sinks);

    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "source"), "set_source", "get_source");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height"), "set_height", "get_height");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_VECTOR2_ARRAY, "sinks"), "set_sinks", "get_sinks");
}

bool polygon_invalid_edges_check(const std::vector<p2t::Point*>& polygon) {
    const double epsilon = 1e-16;
    if (polygon.size() < 3) {
        ERR_PRINT("Polygon has less than 3 points");
        return false;
    }

    for (int i = 0; i < polygon.size() - 1; i++) {
        int j = i + 1;
        if (std::abs(polygon[i]->x - polygon[j]->x) < epsilon && std::abs(polygon[i]->y - polygon[j]->y) < epsilon) {
            ERR_PRINT("Duplicate point in polygon");
            return false;
        }
    }
    return true;
}

PackedVector2Array PolyUtil::triangulate_with_holes(PackedVector2Array outer, Array holes) {
    //WARN_PRINT("Running triangulate w holes w outer size: " + String::num_int64(outer.size()) + " hole count: " + String::num_int64(holes.size()));
    // Create polyline and CDT
    std::vector<p2t::Point*> polyline;
    polyline.reserve(outer.size());
    for (const auto& v : outer)
        polyline.push_back(new p2t::Point(v.x, v.y));

    if (!polygon_invalid_edges_check(polyline))
        return PackedVector2Array();

    polyline.pop_back();
    p2t::CDT* cdt = new p2t::CDT(polyline);

    // Add holes
    for (int i = 0; i < holes.size(); i++) {
        const PackedVector2Array& hole = holes[i];
        std::vector<p2t::Point*> polyline;
        for (const auto& v : hole) {
            polyline.push_back(new p2t::Point(v.x, v.y));
        }
        
        if (!polygon_invalid_edges_check(polyline))
            return PackedVector2Array();

        polyline.pop_back();

        cdt->AddHole(polyline);

        for (p2t::Point* v : polyline) {
           ;// delete v;
        }
    }


    
    cdt->Triangulate();

    // Output
    PackedVector2Array triangulated;

    auto triangles = cdt->GetTriangles();
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; i++) {
            p2t::Point* p = triangle->GetPoint(i);
            triangulated.append(Vector2(p->x, p->y));
        }
    }

    // Free
    for (p2t::Point* v : polyline)
        ;//delete v;

    return triangulated;
}

MAPSHADERS_DLL_SYMBOL std::vector<GeoCoords> PolyUtil::triangulate_with_holes(
    std::vector<GeoCoords> outer, std::vector<std::vector<GeoCoords>> holes )
{
    std::vector<p2t::Point*> polyline;
    polyline.reserve(outer.size());
    for (const auto& v : outer)
        polyline.push_back(new p2t::Point(v.lon.value, v.lat.value));

    if (!polygon_invalid_edges_check(polyline))
        return std::vector<GeoCoords>();

    polyline.pop_back();
    p2t::CDT* cdt = new p2t::CDT(polyline);

    // Add holes
    for (int i = 0; i < holes.size(); i++) {
        const std::vector<GeoCoords>& hole = holes[i];
        std::vector<p2t::Point*> polyline;
        for (const auto& v : hole) {
            polyline.push_back(new p2t::Point(v.lon.value, v.lat.value));
        }
        
        if (!polygon_invalid_edges_check(polyline))
            return std::vector<GeoCoords>();

        polyline.pop_back();

        cdt->AddHole(polyline);

        for (p2t::Point* v : polyline) {
           ;// delete v;
        }
    }


    
    cdt->Triangulate();

    // Output
    std::vector<GeoCoords> triangulated;

    auto triangles = cdt->GetTriangles();
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; i++) {
            p2t::Point* p = triangle->GetPoint(i);
            triangulated.push_back(GeoCoords(Longitude::radians(p->x), Latitude::radians(p->y)));
        }
    }

    // Free
    for (p2t::Point* v : polyline)
        ;//delete v;

    return triangulated;
}

MAPSHADERS_DLL_SYMBOL std::vector<std::pair<double, double>> PolyUtil::triangulate_with_holes(
    std::vector<std::pair<double, double>> outer, std::vector<std::vector<std::pair<double, double>>> holes )
{
    std::vector<p2t::Point*> polyline;
    polyline.reserve(outer.size());
    for (const auto& v : outer)
        polyline.push_back(new p2t::Point(v.first, v.second));

    if (!polygon_invalid_edges_check(polyline))
        return std::vector<std::pair<double, double>>();

    polyline.pop_back();
    p2t::CDT* cdt = new p2t::CDT(polyline);

    // Add holes
    for (int i = 0; i < holes.size(); i++) {
        const std::vector<std::pair<double, double>>& hole = holes[i];
        std::vector<p2t::Point*> polyline;
        for (const auto& v : hole) {
            polyline.push_back(new p2t::Point(v.first, v.second));
        }
        
        if (!polygon_invalid_edges_check(polyline))
            return std::vector<std::pair<double, double>>();

        polyline.pop_back();

        cdt->AddHole(polyline);

        for (p2t::Point* v : polyline) {
           ;// delete v;
        }
    }


    
    cdt->Triangulate();

    // Output
    std::vector<std::pair<double, double>> triangulated;

    auto triangles = cdt->GetTriangles();
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; i++) {
            p2t::Point* p = triangle->GetPoint(i);
            triangulated.push_back(std::make_pair(p->x, p->y));
        }
    }

    // Free
    for (p2t::Point* v : polyline)
        ;//delete v;

    return triangulated;
}

Array PolyUtil::straight_skeleton(PackedVector2Array outer, Array holes) {
    //std::cout << "Running straight skeleton with outer size: " << outer.size() << " hole count: " << holes.size() << std::endl;
    std::vector<polyskel::Vec2> polyskel_outer;
    std::vector<std::vector<polyskel::Vec2>> polyskel_holes;
    for (const Vector2& v : outer) {
        polyskel_outer.push_back(polyskel::Vec2(v.x, v.y));
    }
    for (size_t i = 0; i < holes.size(); i++) {
        PackedVector2Array hole = holes[i];
        polyskel_holes.push_back(std::vector<polyskel::Vec2>());
        for (const Vector2& v : hole) {
            polyskel_holes.back().push_back(polyskel::Vec2(v.x, v.y));
        }
    }

    std::vector<std::shared_ptr<polyskel::Subtree>> result = polyskel::skeletonize(polyskel_outer, polyskel_holes);

    Array out;

    for (const auto& subtree : result) {
        SkeletonSubtree* ss = memnew(SkeletonSubtree);
        ss->set_source(Vector2(subtree->source.x, subtree->source.y));
        ss->set_height(subtree->height);
        PackedVector2Array sinks;

        for (const auto& sink : subtree->sinks) {
            sinks.append(Vector2(sink.x, sink.y));
        }

        ss->set_sinks(sinks);
        out.push_back(ss);
    }

    return out;
}

void PolyUtil::_bind_methods() {
    ClassDB::bind_method(D_METHOD("triangulate_with_holes", "outer", "holes"), (PackedVector2Array(PolyUtil::*)(PackedVector2Array, Array))&PolyUtil::triangulate_with_holes);
    ClassDB::bind_method(D_METHOD("straight_skeleton", "outer", "holes"), &PolyUtil::straight_skeleton);
}
