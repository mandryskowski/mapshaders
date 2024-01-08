#include "SGNode.h"
#include "../import/osm_parser/NodeParser.h"

SGNode::SGNode() : id(-1), pos(Vector3(0,0,0)) {

}
SGNode::SGNode(NodeParser& p, const Vector3& pos):
        id(p.id()), pos(pos)
{
    
}   