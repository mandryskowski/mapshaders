#ifndef GLOBAL_REQUIREMENTS_BUILDER_H
#define GLOBAL_REQUIREMENTS_BUILDER_H
#include <godot_cpp/variant/variant.hpp>
#include "GlobalRequirements.h"


/**
 * @class GlobalRequirementsBuilder
 * 
 * @brief The GlobalRequirementsBuilder class is responsible for building GlobalRequirements objects.
 * 
 * The GlobalRequirementsBuilder class provides a fluent interface for constructing GlobalRequirements objects.
 * It allows you to specify node, way, and relation requirements individually or in bulk, and then build the
 * final GlobalRequirements object using the `build()` method.
 */
/**
 * @brief The GlobalRequirementsBuilder class is responsible for building GlobalRequirements objects.
 * 
 * This class provides methods for adding node, way, and relation requirements to the builder,
 * and for building the final GlobalRequirements object.
 */
class GlobalRequirementsBuilder : public godot::RefCounted {
    GDCLASS(GlobalRequirementsBuilder, godot::RefCounted);

public:
    /**
     * @brief Default constructor for GlobalRequirementsBuilder.
     */
    GlobalRequirementsBuilder() = default;

    /**
     * @brief Adds a single node requirement to the builder.
     * 
     * @param one_req The node requirement to add.
     * @return A reference to the builder object.
     */
    __declspec(dllexport) godot::Variant withNodeRequirement(godot::String one_req) {
        ndr.append(one_req);
        return this;
    }

    /**
     * @brief Adds a single way requirement to the builder.
     * 
     * @param one_req The way requirement to add.
     * @return A reference to the builder object.
     */
    __declspec(dllexport) godot::Variant withWayRequirement(godot::String one_req) {
        wr.append(one_req);
        return this;
    }

    /**
     * @brief Adds a single relation requirement to the builder.
     * 
     * @param one_req The relation requirement to add.
     * @return A reference to the builder object.
     */
    __declspec(dllexport) godot::Variant withRelationRequirement(godot::String one_req) {
        rr.append(one_req);
        return this;
    }

    /**
     * @brief Adds multiple node requirements to the builder.
     * 
     * @param req The array of node requirements to add.
     * @return A reference to the builder object.
     */
    __declspec(dllexport) godot::Variant withNodeRequirements(godot::PackedStringArray req) {
        ndr = req;
        return this;
    }

    /**
     * @brief Adds multiple way requirements to the builder.
     * 
     * @param req The array of way requirements to add.
     * @return A reference to the builder object.
     */
    __declspec(dllexport) godot::Variant withWayRequirements(godot::PackedStringArray req) {
        wr = req;
        return this;
    }

    /**
     * @brief Adds multiple relation requirements to the builder.
     * 
     * @param req The array of relation requirements to add.
     * @return A reference to the builder object.
     */
    __declspec(dllexport) godot::Variant withRelationRequirements(godot::PackedStringArray req) {
        rr = req;
        return this;
    }

    /**
     * @brief Builds and returns the GlobalRequirements object.
     * 
     * @return The built GlobalRequirements object.
     */
    __declspec(dllexport) godot::Variant build() {
        // Return the built GlobalRequirements object
        return memnew(GlobalRequirements(ndr, wr, rr));
    }

protected:
    static void _bind_methods();

private:
    godot::PackedStringArray ndr, wr, rr;
};

#endif // GLOBAL_REQUIREMENTS_BUILDER_H