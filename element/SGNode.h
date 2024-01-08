#ifndef SGNODE_H
#define SGNODE_H
#include "core/object/ref_counted.h"

//class SGWay;
class NodeParser;

class SGNode : public RefCounted
{
    GDCLASS(SGNode, RefCounted);

public:
    SGNode();
    SGNode(NodeParser&, const Vector3& pos);
private:
    int64_t id;
    Vector3 pos;
 //   Vector<SGWay> of_ways;
};

#endif // SGNODE_H