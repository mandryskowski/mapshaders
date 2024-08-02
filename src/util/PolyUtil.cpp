#include "PolyUtil.h"
#define P2T_STATIC_EXPORTS
#include "../poly2tri/poly2tri/poly2tri.h"
#include "../poly2tri/poly2tri/sweep/cdt.h"

using namespace godot;

PackedVector2Array PolyUtil::triangulate_with_holes(PackedVector2Array outer, Array holes) {
    WARN_PRINT("Running triangulate w holes");
    // Create polyline and CDT
    std::vector<p2t::Point*> polyline;
    polyline.reserve(outer.size());
    for (const auto& v : outer)
        polyline.push_back(new p2t::Point(v.x, v.y));

    polyline.pop_back();
    p2t::CDT* cdt = new p2t::CDT(polyline);

    // Add holes
    for (int i = 0; i < holes.size(); i++) {
        const PackedVector2Array& hole = holes[i];
        std::vector<p2t::Point*> polyline;
        for (const auto& v : hole) {
            polyline.push_back(new p2t::Point(v.x, v.y));
        }
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

void PolyUtil::_bind_methods() {
    ClassDB::bind_method(D_METHOD("triangulate_with_holes", "outer", "holes"), &PolyUtil::triangulate_with_holes);
}