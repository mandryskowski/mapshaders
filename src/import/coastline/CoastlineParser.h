#ifndef COASTLINE_PARSER_H
#define COASTLINE_PARSER_H
#include "../GeoMap.h"
#include "../Parser.h"

class CoastlineParser : public Parser {
public:
    using Parser::Parser;
    void import(const godot::String& shpFilename, const godot::String& shxFilename, godot::Ref<GeoMap> geomap = nullptr);
};

#endif // COASTLINE_PARSER_H