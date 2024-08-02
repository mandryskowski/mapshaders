#include "GlobalRequirements.h"

using namespace godot;

GlobalRequirements::GlobalRequirements(PackedStringArray nodeRequirements, PackedStringArray wayRequirements, PackedStringArray relationRequirements)
{
    this->nodeRequirements = str_array_to_dict(nodeRequirements);
    this->wayRequirements = str_array_to_dict(wayRequirements);
    this->relationRequirements = str_array_to_dict(relationRequirements);
}

GlobalRequirements::GlobalRequirements(Dictionary nodeRequirements, Dictionary wayRequirements, Dictionary relationRequirements) {
    this->nodeRequirements = nodeRequirements;
    this->wayRequirements = wayRequirements;
    this->relationRequirements = relationRequirements;
}

void GlobalRequirements::merge(Variant _rhs) {
    GlobalRequirements& rhs = *Object::cast_to<GlobalRequirements>(_rhs);
    nodeRequirements.merge(rhs.nodeRequirements);
    wayRequirements.merge(rhs.wayRequirements);
    relationRequirements.merge(rhs.relationRequirements);
}

void GlobalRequirements::_bind_methods() {
    ClassDB::bind_method(D_METHOD("merge", "rhs"), &GlobalRequirements::merge);
}

Dictionary str_array_to_dict(const PackedStringArray& arr) {
    Dictionary h;
    for (const auto& str : arr) {
        h[str] = Variant();
    }

    return h;
}