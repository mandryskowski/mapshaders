#ifndef POLYUTIL_H
#define POLYUTIL_H
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#include "core/object/ref_counted.h"
#include "core/variant/array.h"
#include "core/object/class_db.h"

class PolyUtil : public RefCounted
{
    GDCLASS(PolyUtil, RefCounted);
public:
    __declspec(dllexport) PackedVector2Array triangulate_with_holes(PackedVector2Array outer, Array holes);

protected:
    static void _bind_methods();
};

#endif // POLYUTIL_H