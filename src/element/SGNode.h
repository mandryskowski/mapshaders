#ifndef SGNODE_H
#define SGNODE_H
#include <godot_cpp/classes/ref_counted.hpp>

//class SGWay;
class NodeParser;

class SGNode : public godot::RefCounted
{
    GDCLASS(SGNode, godot::RefCounted);

public:
    SGNode();
    SGNode(NodeParser&, const godot::Vector3& pos);
private:
    int64_t id;
    godot::Vector3 pos;
 //   Vector<SGWay> of_ways;
};

#endif // SGNODE_H