#ifndef SGWAY_H
#define SGWAY_H
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/templates/vector.hpp>
#include "SGNode.h"

class WayParser;

class SGWay : public godot::RefCounted
{
    GDCLASS(SGWay, godot::RefCounted);

public:
    SGWay();
    SGWay(WayParser&);

private:
    godot::Vector<SGNode> nodes;
};

#endif // SGWAY_H