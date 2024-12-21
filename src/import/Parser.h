#ifndef PARSER_H
#define PARSER_H
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/variant/array.hpp>


class Parser : public godot::Resource {
  GDCLASS(Parser, godot::Resource)
 public:
  Parser() : shader_nodes_reference(nullptr) {}
  Parser(godot::Array shader_nodes)
      : shader_nodes(shader_nodes), shader_nodes_reference(nullptr) {}

  void set_enabled(bool value) { enabled = value; }
  bool is_enabled() const { return enabled; }

  void set_shader_nodes_paths(godot::TypedArray<godot::NodePath> nodes);
  godot::TypedArray<godot::NodePath> get_shader_nodes_paths() const;
  godot::TypedArray<godot::Node> get_shader_nodes() const {
    return shader_nodes;
  }

  void set_shader_nodes_reference(godot::Node* node) {
    shader_nodes_reference = node;
    if (shader_nodes_reference) {
      set_shader_nodes_paths(shader_nodes_stored_paths);
    }
  }

  void clear_nodes(bool);
  bool get_true() { return true; }

  virtual ~Parser() = default;

 protected:
  static void _bind_methods();

 private:
  godot::TypedArray<godot::Node> shader_nodes;

  godot::TypedArray<godot::NodePath> shader_nodes_stored_paths;
  godot::Node* shader_nodes_reference;
  bool enabled;
};

#endif  // PARSER_H