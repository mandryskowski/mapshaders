/* Abstraction to decouple OSM from Elevation */
#ifndef OSM_HEIGHTMAP_H
#define OSM_HEIGHTMAP_H
#include "../GeoMap.h"
#include "../elevation/ElevationParser.h"
#include <godot_cpp/classes/ref.hpp>
#include <memory>

class OSMHeightmap : public godot::RefCounted {
    GDCLASS(OSMHeightmap, godot::RefCounted);
public:
    virtual double getElevation(const GeoCoords&) const = 0;
};

class ElevationHeightmap : public OSMHeightmap {
    GDCLASS(ElevationHeightmap, OSMHeightmap);
public:
    ElevationHeightmap(godot::Ref<ElevationGrid> elevation_grid);
    virtual double getElevation(const GeoCoords&) const override;
private:
    godot::Ref<ElevationGrid> elevation_grid;
};

#endif // OSM_HEIGHTMAP_H