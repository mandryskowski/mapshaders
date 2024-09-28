#ifndef PARSER_H
#define PARSER_H
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/node.hpp>

class Parser : public godot::Resource {
    GDCLASS(Parser, godot::Resource)
public:
    Parser() {}
    Parser(godot::Array shader_nodes) : shader_nodes(shader_nodes) {}

    void set_enabled(bool value) {
        enabled = value;
    }
    bool is_enabled() const {
        return enabled;
    }


    void set_shader_nodes(const godot::TypedArray<godot::Node>& nodes) {
        shader_nodes = nodes;
    }
    godot::TypedArray<godot::Node> get_shader_nodes() const {
        return shader_nodes;
    }

    virtual ~Parser() = default;

protected:
    static void _bind_methods();

private:
    godot::TypedArray<godot::Node> shader_nodes;
    bool enabled;
};

#endif // PARSER_H