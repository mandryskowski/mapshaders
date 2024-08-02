#ifndef GLOBALREQUIREMENTS_H
#define GLOBALREQUIREMENTS_H
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/ref_counted.hpp>

class GlobalRequirements : public godot::RefCounted {
    GDCLASS(GlobalRequirements, godot::RefCounted);
public:
    GlobalRequirements() = default;
    GlobalRequirements(godot::PackedStringArray nodeRequirements, godot::PackedStringArray wayRequirements, godot::PackedStringArray relationRequirements);
    GlobalRequirements(godot::Dictionary nodeRequirements, godot::Dictionary wayRequirements, godot::Dictionary relationRequirements);

    /**
     * Merges the given `GlobalRequirements` object with the current object.
     * 
     * @param other The `GlobalRequirements` object to merge with.
     */
    void merge(godot::Variant);

protected:
    static void _bind_methods();

private:
    // This HashMap serves as a Set.
    godot::Dictionary nodeRequirements, wayRequirements, relationRequirements;
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
godot::Dictionary str_array_to_dict(const godot::PackedStringArray& arr);

#endif // GLOBALREQUIREMENTS_H