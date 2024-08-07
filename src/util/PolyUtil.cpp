#include "PolyUtil.h"
#define P2T_STATIC_EXPORTS
#include "../poly2tri/poly2tri/poly2tri.h"
#include "../poly2tri/poly2tri/sweep/cdt.h"
#include "../polyskel-cpp-port/polyskel.h"
#include "../polyskel-cpp-port/vec.h"
#include "../polyskel-cpp-port/lavertex.h"

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

Array PolyUtil::straight_skeleton(PackedVector2Array outer, Array holes) {
    std::cout << "Called straight skeleton" << std::endl;
    std::vector<Vec2> polyskel_outer;
    std::vector<std::vector<Vec2>> polyskel_holes;
    for (const Vector2& v : outer) {
        polyskel_outer.push_back(Vec2(v.x, v.y));
    }
    for (size_t i = 0; i < holes.size(); i++) {
        PackedVector2Array hole = holes[i];
        polyskel_holes.push_back(std::vector<Vec2>());
        for (const Vector2& v : hole) {
            polyskel_holes.back().push_back(Vec2(v.x, v.y));
        }
    }

    std::cout << meow();
    std::cout << "Running straight skeleton with " << polyskel_outer.size() << std::endl;

    auto result = skeletonize(polyskel_outer, polyskel_holes);

    std::cout << "Finished straight skeleton" << std::endl;

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
    ClassDB::bind_method(D_METHOD("triangulate_with_holes", "outer", "holes"), &PolyUtil::triangulate_with_holes);
    ClassDB::bind_method(D_METHOD("straight_skeleton", "outer", "holes"), &PolyUtil::straight_skeleton);
}