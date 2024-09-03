#include "OSMHeightmap.h"
#include "../elevation/ElevationParser.h"

ElevationHeightmap::ElevationHeightmap(godot::Ref<ElevationGrid> elevation_grid):
    elevation_grid(elevation_grid){}

double ElevationHeightmap::getElevation(const GeoCoords & coords) const {
    return elevation_grid->bilinearInterpolation(coords);
}