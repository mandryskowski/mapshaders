#ifndef POLYUTIL_H
#define POLYUTIL_H
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/class_db.hpp>

class SkeletonSubtree : public godot::RefCounted {
    GDCLASS(SkeletonSubtree, godot::RefCounted);
public:
    godot::Vector2 get_source() const;
    void set_source(const godot::Vector2& value);
    double get_height() const;
    void set_height(double value);
    godot::PackedVector2Array get_sinks() const;
    void set_sinks(const godot::PackedVector2Array& value);
protected:
    static void _bind_methods();
private:
    godot::Vector2 source;
    double height;
    godot::PackedVector2Array sinks;
};

class PolyUtil : public godot::RefCounted
{
    GDCLASS(PolyUtil, godot::RefCounted);
public:
    __declspec(dllexport) godot::PackedVector2Array triangulate_with_holes(godot::PackedVector2Array outer, godot::Array holes);
    __declspec(dllexport) godot::Array straight_skeleton(godot::PackedVector2Array outer, godot::Array holes);
protected:
    static void _bind_methods();
};

#endif // POLYUTIL_H