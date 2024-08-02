#ifndef POLYUTIL_H
#define POLYUTIL_H
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/class_db.hpp>

class PolyUtil : public godot::RefCounted
{
    GDCLASS(PolyUtil, godot::RefCounted);
public:
    __declspec(dllexport) godot::PackedVector2Array triangulate_with_holes(godot::PackedVector2Array outer, godot::Array holes);

protected:
    static void _bind_methods();
};

#endif // POLYUTIL_H