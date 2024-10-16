#include "GlobalRequirementsBuilder.h"

using namespace godot;

void GlobalRequirementsBuilder::_bind_methods() {
    ClassDB::bind_method(D_METHOD("withNodeRequirement", "one_req"), &GlobalRequirementsBuilder::withNodeRequirement);
    ClassDB::bind_method(D_METHOD("withWayRequirement", "one_req"), &GlobalRequirementsBuilder::withWayRequirement);
    ClassDB::bind_method(D_METHOD("withRelationRequirement", "one_req"), &GlobalRequirementsBuilder::withRelationRequirement);

    ClassDB::bind_method(D_METHOD("withNodeRequirements", "req"), &GlobalRequirementsBuilder::withNodeRequirements);
    ClassDB::bind_method(D_METHOD("withWayRequirements", "req"), &GlobalRequirementsBuilder::withWayRequirements);
    ClassDB::bind_method(D_METHOD("withRelationRequirements", "req"), &GlobalRequirementsBuilder::withRelationRequirements);

    ClassDB::bind_method(D_METHOD("build"), &GlobalRequirementsBuilder::build);
}   