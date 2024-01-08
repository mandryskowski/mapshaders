#ifndef WAYPARSER_H
#define NODEPARSER_H

#include "core/object/ref_counted.h"

class NodeParser : public RefCounted {
    GDCLASS(NodeParser, RefCounted);

public:
    NodeParser(Dictionary&);

    int64_t id() {
        return d.get("id", -1);
    }


    
private:
    Dictionary& d;
};

#endif // NODEPARSER_H