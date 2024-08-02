#ifndef NODEPARSER_H
#define NODEPARSER_H

#include <godot_cpp/classes/ref_counted.hpp>

class NodeParser : public godot::RefCounted {
    GDCLASS(NodeParser, godot::RefCounted);

public:
    NodeParser(godot::Dictionary&);

    int64_t id() {
        return d.get("id", -1);
    }


    
private:
    godot::Dictionary& d;
};

#endif // NODEPARSER_H