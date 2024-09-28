#include "ElevationParser.h"
#include <vector>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;

void ElevationGrid::setNcols(int value) { ncols = value; }
int ElevationGrid::getNcols() const { return ncols; }

void ElevationGrid::setNrows(int value) { nrows = value; }
int ElevationGrid::getNrows() const { return nrows; }

void ElevationGrid::setTopLeftGeo(const GeoCoords& value) { topLeftGeo = value; }
const GeoCoords& ElevationGrid::getTopLeftGeo() const { return topLeftGeo; }

void ElevationGrid::setCellsize(double value) { cellsize = value; }
double ElevationGrid::getCellsize() const { return cellsize; }

void ElevationGrid::setNodataValue(double value) { nodata_value = value; }
double ElevationGrid::getNodataValue() const { return nodata_value; }

void ElevationGrid::setHeightmap(const godot::Array& value) { heightmap = value; }
godot::Array ElevationGrid::getHeightmap() const { return heightmap; }

godot::Vector3 ElevationGrid::getBottomLeftWorld() const {
    if (this->geomap == nullptr) {
        ERR_PRINT("GeoMap is not set");
        return godot::Vector3();
    }

    return this->geomap->geo_to_world(getTopLeftGeo() - GeoCoords(Longitude::zero(), Latitude::degrees(getCellsize() * getNrows())));
}

godot::Vector3 ElevationGrid::getBottomRightWorld() const {
    if (this->geomap == nullptr) {
        ERR_PRINT("GeoMap is not set");
        return godot::Vector3();
    }

    return this->geomap->geo_to_world(getTopLeftGeo() + GeoCoords(Longitude::degrees(getCellsize() * getNcols()), Latitude::degrees(-getCellsize() * getNrows())));
}

__declspec(dllexport) godot::Vector3 ElevationGrid::getTopLeftWorld() const {
    if (this->geomap == nullptr) {
        ERR_PRINT("GeoMap is not set");
        return godot::Vector3();
    }

    return this->geomap->geo_to_world(getTopLeftGeo());
}

godot::Vector3 ElevationGrid::getTopRightWorld() const {
    if (this->geomap == nullptr) {
        ERR_PRINT("GeoMap is not set");
        return godot::Vector3();
    }

    return this->geomap->geo_to_world(getTopLeftGeo() + GeoCoords(Longitude::degrees(getCellsize() * getNcols()), Latitude::zero()));
}

double ElevationGrid::bilinearInterpolation(const GeoCoords &point) const {
    // Convert geographic coordinates to pixel coordinates
    double pixelX = (point.lon - getTopLeftGeo().lon).get_degrees() / getCellsize();
    double pixelY = (getTopLeftGeo().lat - point.lat).get_degrees() / getCellsize();

    // Get the integer part of the pixel coordinates (nearest pixel indices)
    int col = static_cast<int>(std::floor(pixelX));
    int row = static_cast<int>(std::floor(pixelY));

    // Ensure indices are within bounds (same as in GDAL code)
    col = std::max(0, std::min(col, getNcols() - 2));
    row = std::max(0, std::min(row, getNrows() - 2));

    // Calculate fractional parts
    double dx = pixelX - col;
    double dy = pixelY - row;

    // Bilinear interpolation
    const auto& heightmap = getHeightmap();
    double Q11 = static_cast<PackedFloat64Array>(heightmap[row])[col];
    double Q21 = static_cast<PackedFloat64Array>(heightmap[row])[col + 1];
    double Q12 = static_cast<PackedFloat64Array>(heightmap[row + 1])[col];
    double Q22 = static_cast<PackedFloat64Array>(heightmap[row + 1])[col + 1];

    double elevation = (1 - dx) * (1 - dy) * Q11 +
                      dx * (1 - dy) * Q21 +
                      (1 - dx) * dy * Q12 +
                      dx * dy * Q22;

    return elevation;
}

void ElevationGrid::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_ncols", "value"), &ElevationGrid::setNcols);
    ClassDB::bind_method(D_METHOD("get_ncols"), &ElevationGrid::getNcols);

    ClassDB::bind_method(D_METHOD("set_nrows", "value"), &ElevationGrid::setNrows);
    ClassDB::bind_method(D_METHOD("get_nrows"), &ElevationGrid::getNrows);

    ClassDB::bind_method(D_METHOD("set_cellsize", "value"), &ElevationGrid::setCellsize);
    ClassDB::bind_method(D_METHOD("get_cellsize"), &ElevationGrid::getCellsize);

    //ClassDB::bind_method(D_METHOD("set_heightmap", "value"), &ElevationGrid::setHeightmap);
    ClassDB::bind_method(D_METHOD("get_heightmap"), &ElevationGrid::getHeightmap);

    ClassDB::bind_method(D_METHOD("set_nodata_value", "value"), &ElevationGrid::setNodataValue);
    ClassDB::bind_method(D_METHOD("get_nodata_value"), &ElevationGrid::getNodataValue);

    ClassDB::bind_method(D_METHOD("get_bottom_left_world"), &ElevationGrid::getBottomLeftWorld);
    ClassDB::bind_method(D_METHOD("get_bottom_right_world"), &ElevationGrid::getBottomRightWorld);
    ClassDB::bind_method(D_METHOD("get_top_left_world"), &ElevationGrid::getTopLeftWorld);
    ClassDB::bind_method(D_METHOD("get_top_right_world"), &ElevationGrid::getTopRightWorld);




    ADD_PROPERTY(PropertyInfo(Variant::INT, "ncols"), "set_ncols", "get_ncols");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "nrows"), "set_nrows", "get_nrows");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cellsize"), "set_cellsize", "get_cellsize");
    //ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "heightmap"), "set_heightmap", "get_heightmap");
    //ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "heightmap", PROPERTY_HINT_ARRAY_TYPE, vformat("%s/%s:%s", Variant::PACKED_FLOAT64_ARRAY)), "set_heightmap", "get_heightmap");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "nodata_value"), "set_nodata_value", "get_nodata_value");
}

// Function to load the ASCII Grid
Ref<ElevationGrid> loadASCIIGrid(const godot::String& filename) {
    Ref<FileAccess> gd_file = FileAccess::open(filename, FileAccess::READ);
    if (!gd_file->is_open()) {
        WARN_PRINT("Could not open file " + filename);
    }

    std::istringstream file(std::string(gd_file->get_as_text().ascii().ptr()));

    gd_file->close();

    Ref<ElevationGrid> grid = memnew(ElevationGrid);
    std::string line;

    {
        std::string key;
        int ncols, nrows;
        double left, bottom, cellsize, nodata_value;
        file >> key >> ncols;
        file >> key >> nrows;
        file >> key >> left;
        file >> key >> bottom;
        file >> key >> cellsize;

        auto pos = file.tellg();
        file >> key;
        if (key == "NODATA_value") {
            file >> nodata_value;
        } else {
            file.seekg(pos);
        }

        grid->setNcols(ncols);
        grid->setNrows(nrows);
        grid->setCellsize(cellsize);
        grid->setTopLeftGeo(GeoCoords(Longitude::degrees(left), Latitude::degrees(bottom + nrows * cellsize)));
    }

    std::getline(file, line);


    // Initialize the heightmap
    std::vector<PackedFloat64Array> heightmap(grid->getNrows(), PackedFloat64Array());
    // Read the elevation data
    for (int i = 0; i < grid->getNrows(); i++) {
        std::getline(file, line);
        std::istringstream iss(line);
        heightmap[i].resize(grid->getNcols());
        for (int j = 0; j < grid->getNcols(); j++) {
            iss >> heightmap[i][j];
        }
    }


    auto gd_heightmap = TypedArray<PackedFloat64Array>();
    gd_heightmap.resize(grid->getNrows());
    for (int i = 0; i < grid->getNrows(); i++) {
        gd_heightmap[i] = heightmap[i];
    }

    grid->setHeightmap(gd_heightmap);

    return grid;
}

// Function to extract a subgrid based on latitude and longitude bounds
std::vector<std::vector<double>> extractSubgrid(const ElevationGrid& grid, const GeoCoords& start, const GeoCoords& end) {
    // Calculate row and column indices (same as in GDAL code)
    int rowStart = static_cast<int>(std::floor((grid.getTopLeftGeo().lat - end.lat).get_degrees() / grid.getCellsize()));
    int rowEnd = static_cast<int>(std::floor((grid.getTopLeftGeo().lat - start.lat).get_degrees() / grid.getCellsize()));
    int colStart = static_cast<int>(std::floor((start.lon - grid.getTopLeftGeo().lon).get_degrees() / grid.getCellsize()));
    int colEnd = static_cast<int>(std::floor((end.lon - grid.getTopLeftGeo().lon).get_degrees() / grid.getCellsize()));

    rowStart = std::max(0, rowStart);
    rowEnd = std::min(grid.getNrows() - 1, rowEnd);
    colStart = std::max(0, colStart);
    colEnd = std::min(grid.getNcols() - 1, colEnd);

    // Extract the subgrid
    std::vector<std::vector<double>> subgrid(std::max(0, rowEnd - rowStart + 1), std::vector<double>(std::max(0, colEnd - colStart + 1)));
    const auto& heightmap = grid.getHeightmap();
    for (int i = rowStart; i <= rowEnd; ++i) {
        for (int j = colStart; j <= colEnd; ++j) {
            subgrid[i - rowStart][j - colStart] = static_cast<PackedFloat64Array>(heightmap[i])[j];
        }
    }

    return subgrid;
}

double bilinearInterpolation(const ElevationGrid& grid, const GeoCoords& point) {
    // Convert geographic coordinates to pixel coordinates
    double pixelX = (point.lon - grid.getTopLeftGeo().lon).get_degrees() / grid.getCellsize();
    double pixelY = (grid.getTopLeftGeo().lat - point.lat).get_degrees() / grid.getCellsize();

    // Get the integer part of the pixel coordinates (nearest pixel indices)
    int col = static_cast<int>(std::floor(pixelX));
    int row = static_cast<int>(std::floor(pixelY));

    // Ensure indices are within bounds (same as in GDAL code)
    col = std::max(0, std::min(col, grid.getNcols() - 2));
    row = std::max(0, std::min(row, grid.getNrows() - 2));

    // Calculate fractional parts
    double dx = pixelX - col;
    double dy = pixelY - row;

    // Bilinear interpolation
    const auto& heightmap = grid.getHeightmap();
    double Q11 = static_cast<PackedFloat64Array>(heightmap[row])[col];
    double Q21 = static_cast<PackedFloat64Array>(heightmap[row])[col + 1];
    double Q12 = static_cast<PackedFloat64Array>(heightmap[row + 1])[col];
    double Q22 = static_cast<PackedFloat64Array>(heightmap[row + 1])[col + 1];

    double elevation = (1 - dx) * (1 - dy) * Q11 +
                      dx * (1 - dy) * Q21 +
                      (1 - dx) * dy * Q12 +
                      dx * dy * Q22;

    return elevation;
}


// Function to print the 2D grid
void printGrid(const std::vector<std::vector<double>>& grid) {
    for (const auto& row : grid) {
        for (const auto& value : row) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

godot::Ref<ElevationGrid> ElevationParser::import(godot::Ref<GeoMap> geomap) {
    auto grid = loadASCIIGrid(filename.ascii().ptr());
    if (geomap.is_null()) {
        geomap = godot::Ref<GeoMap>(memnew(EquirectangularGeoMap(grid->getTopLeftGeo() - GeoCoords(Longitude::zero(), Latitude::degrees(grid->getCellsize() * grid->getNrows())), grid->getTopLeftGeo() + GeoCoords(Longitude::degrees(grid->getCellsize() * grid->getNcols()), Latitude::zero()))));
    }

    grid->set_geo_map(geomap);

    WARN_PRINT("Loaded grid with " + String::num_int64(grid->getNcols()) + " columns and " + String::num_int64(grid->getNrows()) + " rows " + " and cellsize " + String::num_real(grid->getCellsize()));
    WARN_PRINT("Grid top left corner is at " + String::num_real(grid->getTopLeftGeo().lon.get_degrees()) + " " + String::num_real(grid->getTopLeftGeo().lat.get_degrees()));	
    
    auto shader_nodes = this->get_shader_nodes();
    for (int i = 0; i < shader_nodes.size(); i++) {
        Object::cast_to<Node>(shader_nodes[i])->call("import_grid", grid);
    }

    return grid;
}

void ElevationParser::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import", "geomap"), &ElevationParser::import);
    ClassDB::bind_method(D_METHOD("set_filename", "value"), &ElevationParser::set_filename);
    ClassDB::bind_method(D_METHOD("get_filename"), &ElevationParser::get_filename);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "filename", PROPERTY_HINT_FILE, "*.asc"), "set_filename", "get_filename");
}