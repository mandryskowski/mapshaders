#ifndef SGWAY_H
#define SGWAY_H
#include "core/object/ref_counted.h"
#include "SGNode.h"

class WayParser;

class SGWay : public RefCounted
{
    GDCLASS(SGWay, RefCounted);

public:
    SGWay();
    SGWay(WayParser&);

private:
    Vector<SGNode> nodes;
};

#endif // SGWAY_H