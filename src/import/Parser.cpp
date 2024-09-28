#include "Parser.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>


using namespace godot;

void Parser::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_enabled", "value"), &Parser::set_enabled);
    ClassDB::bind_method(D_METHOD("is_enabled"), &Parser::is_enabled);

    ClassDB::bind_method(D_METHOD("set_shader_nodes", "nodes"), &Parser::set_shader_nodes);
    ClassDB::bind_method(D_METHOD("get_shader_nodes"), &Parser::get_shader_nodes);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "is_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "shader_nodes", PROPERTY_HINT_ARRAY_TYPE, vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_NODE_TYPE, "Node")), "set_shader_nodes", "get_shader_nodes");
}