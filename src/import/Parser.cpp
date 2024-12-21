#include "Parser.h"

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void Parser::set_shader_nodes_paths(
    godot::TypedArray<godot::NodePath> nodes_paths) {
  TypedArray<Node> nodes;

  shader_nodes_stored_paths = nodes_paths;

  shader_nodes.clear();

  if (!shader_nodes_reference) {
    return;
  }

  for (int i = 0; i < nodes_paths.size(); i++) {
    shader_nodes.push_back(
        shader_nodes_reference->has_node(nodes_paths[i])
            ? shader_nodes_reference->get_node<Node>(nodes_paths[i])
            : nullptr);
  }
}

godot::TypedArray<godot::NodePath> Parser::get_shader_nodes_paths() const {
  return shader_nodes_stored_paths;
}

void Parser::clear_nodes(bool) {
  for (int i = 0; i < shader_nodes.size(); i++) {
    auto node = Object::cast_to<Node>(shader_nodes[i]);
    auto children = node->get_children();
    for (int j = 0; j < children.size(); j++) {
      node->remove_child(Object::cast_to<Node>(children[j]));
    }
  }
}

void Parser::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_enabled", "value"), &Parser::set_enabled);
  ClassDB::bind_method(D_METHOD("is_enabled"), &Parser::is_enabled);

  ClassDB::bind_method(D_METHOD("set_shader_nodes_paths", "nodes"),
                       &Parser::set_shader_nodes_paths);
  ClassDB::bind_method(D_METHOD("get_shader_nodes_paths"),
                       &Parser::get_shader_nodes_paths);
  ClassDB::bind_method(D_METHOD("get_shader_nodes"), &Parser::get_shader_nodes);

  ClassDB::bind_method(D_METHOD("clear_nodes", "plsrefactor"),
                       &Parser::clear_nodes);
  ClassDB::bind_method(D_METHOD("get_true"), &Parser::get_true);

  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled",
               "is_enabled");
  // ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "scene_tree", PROPERTY_HINT_T,
  // "SceneTree"), "set_scene_tree", "get_scene_tree");
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "clear_nodes"), "clear_nodes",
               "get_true");
  ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "shader_nodes",
                            PROPERTY_HINT_ARRAY_TYPE, "NodePath"),
               "set_shader_nodes_paths", "get_shader_nodes_paths");
}