#ifndef GLOBALREQUIREMENTS_H
#define GLOBALREQUIREMENTS_H
#include "core\variant\variant.h"
#include <core/templates/hash_map.h>
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"

class GlobalRequirements : public RefCounted {
    GDCLASS(GlobalRequirements, RefCounted);
public:
    GlobalRequirements() = default;
    GlobalRequirements(PackedStringArray nodeRequirements, PackedStringArray wayRequirements, PackedStringArray relationRequirements);
    GlobalRequirements(Dictionary nodeRequirements, Dictionary wayRequirements, Dictionary relationRequirements);

    /**
     * Merges the given `GlobalRequirements` object with the current object.
     * 
     * @param other The `GlobalRequirements` object to merge with.
     */
    void merge(Variant);

protected:
    static void _bind_methods();

private:
    // This HashMap serves as a Set.
    Dictionary nodeRequirements, wayRequirements, relationRequirements;
};

/**
 * Converts a PackedStringArray to a Dictionary.
 *
 * This function takes a PackedStringArray as input and converts it into a Dictionary.
 * Each element in the PackedStringArray is added as a key in the Dictionary with a value of `true`.
 *
 * @param arr The PackedStringArray to convert.
 * @return The resulting Dictionary.
 */
Dictionary str_array_to_dict(const PackedStringArray& arr);

#endif // GLOBALREQUIREMENTS_H