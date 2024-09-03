#ifndef PARSER_H
#define PARSER_H
#include <godot_cpp/variant/array.hpp>

class Parser {
public:
    Parser(godot::Array shader_nodes) : shader_nodes(shader_nodes) {}

protected:
    godot::Array get_shader_nodes() const {
        return shader_nodes;
    }

private:
    godot::Array shader_nodes;
};

#endif // PARSER_H