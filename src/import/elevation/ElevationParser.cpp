#include "ElevationParser.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "../../../extern/libtiff/libtiff/tiffio.h"
#include "../TileMap.h"
#include "../util/ThreadPool.h"

using namespace godot;

void ElevationGrid::setNcols(int value) { ncols = value; }
int ElevationGrid::getNcols(bool one_cell_padding) const {
  return one_cell_padding && has_one_cell_padding ? ncols - 2 : ncols;
}

void ElevationGrid::setNrows(int value) { nrows = value; }
int ElevationGrid::getNrows(bool one_cell_padding) const {
  return one_cell_padding && has_one_cell_padding ? nrows - 2 : nrows;
}

void ElevationGrid::setTopLeftGeo(const GeoCoords& value) {
  topLeftGeo = value;
}
GeoCoords ElevationGrid::getTopLeftGeo(bool one_cell_padding) const {
  return one_cell_padding && has_one_cell_padding
             ? topLeftGeo + GeoCoords(Longitude::degrees(cellsize),
                                      Latitude::degrees(-cellsize))
             : topLeftGeo;
}

void ElevationGrid::setCellsize(double value) { cellsize = value; }
double ElevationGrid::getCellsize() const { return cellsize; }

void ElevationGrid::setNodataValue(double value) { nodata_value = value; }
double ElevationGrid::getNodataValue() const { return nodata_value; }

void ElevationGrid::setHeightmap(const godot::Array& value) {
  heightmap = value;
}
godot::Array ElevationGrid::getHeightmap() const { return heightmap; }

double ElevationGrid::bilinearInterpolation(const GeoCoords& point) const {
  // Convert geographic coordinates to pixel coordinates
  double pixelX =
      (point.lon - getTopLeftGeo(false).lon).get_degrees() / getCellsize();
  double pixelY =
      (getTopLeftGeo(false).lat - point.lat).get_degrees() / getCellsize();

  // Get the integer part of the pixel coordinates (nearest pixel indices)
  int col = static_cast<int>(std::floor(pixelX));
  int row = static_cast<int>(std::floor(pixelY));

  // Ensure indices are within bounds (same as in GDAL code)
  col = std::max(0, std::min(col, getNcols(false) - 2));
  row = std::max(0, std::min(row, getNrows(false) - 2));

  // Calculate fractional parts
  double dx = pixelX - col;
  double dy = pixelY - row;

  // Bilinear interpolation
  const auto& heightmap = getHeightmap();
  double Q11 = static_cast<PackedFloat64Array>(heightmap[row])[col];
  double Q21 = static_cast<PackedFloat64Array>(heightmap[row])[col + 1];
  double Q12 = static_cast<PackedFloat64Array>(heightmap[row + 1])[col];
  double Q22 = static_cast<PackedFloat64Array>(heightmap[row + 1])[col + 1];

  double elevation = (1 - dx) * (1 - dy) * Q11 + dx * (1 - dy) * Q21 +
                     (1 - dx) * dy * Q12 + dx * dy * Q22;

  return elevation;
}

void ElevationGrid::_bind_methods() {
  ClassDB::bind_method(D_METHOD("set_ncols", "value"),
                       &ElevationGrid::setNcols);
  ClassDB::bind_method(D_METHOD("get_ncols"), &ElevationGrid::getNcols);

  ClassDB::bind_method(D_METHOD("set_nrows", "value"),
                       &ElevationGrid::setNrows);
  ClassDB::bind_method(D_METHOD("get_nrows"), &ElevationGrid::getNrows);

  ClassDB::bind_method(D_METHOD("set_cellsize", "value"),
                       &ElevationGrid::setCellsize);
  ClassDB::bind_method(D_METHOD("get_cellsize"), &ElevationGrid::getCellsize);

  ClassDB::bind_method(D_METHOD("set_heightmap", "value"),
                       &ElevationGrid::setHeightmap);
  ClassDB::bind_method(D_METHOD("get_heightmap"), &ElevationGrid::getHeightmap);

  ClassDB::bind_method(D_METHOD("set_nodata_value", "value"),
                       &ElevationGrid::setNodataValue);
  ClassDB::bind_method(D_METHOD("get_nodata_value"),
                       &ElevationGrid::getNodataValue);

  ADD_PROPERTY(PropertyInfo(Variant::INT, "ncols"), "set_ncols", "get_ncols");
  ADD_PROPERTY(PropertyInfo(Variant::INT, "nrows"), "set_nrows", "get_nrows");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "cellsize"), "set_cellsize",
               "get_cellsize");
  ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "heightmap"), "set_heightmap",
               "get_heightmap");
  // ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "heightmap",
  // PROPERTY_HINT_ARRAY_TYPE, vformat("%s/%s:%s",
  // Variant::PACKED_FLOAT64_ARRAY)), "set_heightmap", "get_heightmap");
  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "nodata_value"), "set_nodata_value",
               "get_nodata_value");
}

#define TIFFTAG_GEOPIXELSCALE 33550
#define TIFFTAG_GEOTIEPOINTS 33922
#define TIFFTAG_GDAL_NODATA 42113

Ref<ElevationGrid> readGeoTIFFInfo(const std::string& filename) {
  auto grid = memnew(ElevationGrid(false));
  TIFF* tiff = TIFFOpen(filename.c_str(), "r");
  if (!tiff) {
    WARN_PRINT("Failed to open GeoTIFF file.");
    return {};
  }

  // 1. Read number of columns and rows
  uint32_t ncols, nrows;
  TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &ncols);
  TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &nrows);

  grid->setNcols(ncols);
  grid->setNrows(nrows);

  // 2. Read georeferencing tags
  double* tiePoints;  // Tiepoint structure: [i, j, k, x, y, z]
  double* scale;      // Pixel scale: [scaleX, scaleY, scaleZ]
  uint16_t count;
  if (TIFFGetField(tiff, TIFFTAG_GEOTIEPOINTS, &count, &tiePoints)) {
    grid->setTopLeftGeo(GeoCoords(Longitude::degrees(tiePoints[3]),
                                  Latitude::degrees(tiePoints[4])));
  } else {
    WARN_PRINT("Failed to retrieve GeoTIFF tiepoints.");
    return {};
  }

  if (TIFFGetField(tiff, TIFFTAG_GEOPIXELSCALE, &count, &scale)) {
    grid->setCellsize(scale[0]);
  } else {
    WARN_PRINT("Failed to retrieve GeoTIFF pixel scale.");
    return {};
  }

  // 3. Read NODATA value (if present)
  char* nodataValue = nullptr;
  if (TIFFGetField(tiff, TIFFTAG_GDAL_NODATA, &count, &nodataValue)) {
    grid->setNodataValue(std::atoi(nodataValue));
  } else {
    grid->setNodataValue(-9999);
  }

  TIFFClose(tiff);
  return grid;
}

void handler(const char* module, const char* fmt, va_list ap) {
  std::cout << "TIFF error: " << fmt << std::endl;
}

Ref<ElevationGrid> readGeoTIFF(const std::string& filename) {
  TIFFSetErrorHandler(handler);
  std::cout << "Loading " << filename << std::endl;
  auto grid = readGeoTIFFInfo(filename);
  TIFF* tiff = TIFFOpen(filename.c_str(), "r");
  if (!tiff) {
    std::cout << "Failed to open TIFF file." << std::endl;
    return nullptr;
  }

  uint16_t bitsPerSample, samplesPerPixel;
  TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);

  std::vector<PackedFloat64Array> heightmap(grid->getNrows(false),
                                            PackedFloat64Array());

  // Loading stripes
  if (!TIFFIsTiled(tiff)) {
    tsize_t scanlineSize = TIFFScanlineSize(tiff);
    for (int row = 0; row < grid->getNrows(false); row++) {
      std::vector<uint8_t> scanline(scanlineSize);
      if (TIFFReadScanline(tiff, scanline.data(), row) < 0) {
        std::cout << "Error reading scanline " << row << std::endl;
        TIFFClose(tiff);
        return nullptr;
      }

      heightmap[row].resize(grid->getNcols(false));
      for (int i = 0; i < grid->getNcols(false); i++) {
        if (bitsPerSample == 8) {
          heightmap[row][i] = static_cast<double>(scanline[i]);
        } else if (bitsPerSample == 16) {
          uint16_t* data = reinterpret_cast<uint16_t*>(scanline.data());
          heightmap[row][i] = static_cast<double>(data[i]);
        } else if (bitsPerSample == 32) {
          float* data = reinterpret_cast<float*>(scanline.data());
          heightmap[row][i] = static_cast<double>(data[i]);
        } else {
          std::cerr << "Unsupported bits per sample: " << bitsPerSample
                    << std::endl;
          TIFFClose(tiff);
          return nullptr;
        }
      }
    }
  } else {
    tsize_t tileBufSize = TIFFTileSize(tiff);  // Size of a tile in bytes
    uint32_t tileWidth, tileLength;

    // Get tile dimensions
    TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tileWidth);
    TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tileLength);

    for (int row = 0; row < grid->getNrows(false); row++)
      heightmap[row].resize(grid->getNcols(false));

    // Iterate over the grid in tile-sized chunks
    for (uint32_t row = 0; row < grid->getNrows(false); row += tileLength) {
      for (uint32_t col = 0; col < grid->getNcols(false); col += tileWidth) {
        std::vector<uint8_t> tile(tileBufSize);
        // Read the tile at the current position
        if (TIFFReadTile(tiff, tile.data(), col / tileWidth, row / tileLength,
                         0, 0) < 0) {
          std::cerr << "Error reading tile at (" << col << ", " << row << ")"
                    << std::endl;
          continue;
        }

        // Process the tile data
        for (uint32_t y = 0;
             y < tileLength && (row + y) < grid->getNrows(false); ++y) {
          for (uint32_t x = 0;
               x < tileWidth && (col + x) < grid->getNcols(false); ++x) {
            uint32_t index = (y * tileWidth + x) * (bitsPerSample / 8);

            if (bitsPerSample == 8) {
              heightmap[row + y][col + x] = static_cast<double>(tile[index]);
            } else if (bitsPerSample == 16) {
              uint16_t* data = reinterpret_cast<uint16_t*>(tile.data());
              heightmap[row + y][col + x] = static_cast<double>(data[index]);
            } else if (bitsPerSample == 32) {
              float* data = reinterpret_cast<float*>(tile.data());
              heightmap[row + y][col + x] = static_cast<double>(data[index]);
            } else {
              std::cerr << "Unsupported bits per sample: " << bitsPerSample
                        << std::endl;
              return {};
            }
          }
        }
      }
    }
  }

  TIFFClose(tiff);

  auto gd_heightmap = TypedArray<PackedFloat64Array>();
  gd_heightmap.resize(grid->getNrows(false));
  for (int i = 0; i < grid->getNrows(false); i++) {
    gd_heightmap[i] = heightmap[i];
  }

  grid->setHeightmap(gd_heightmap);

  std::cout << "Successfully loaded raster data: " << grid->getNcols(false)
            << "x" << grid->getNrows(false) << std::endl;

  std::cout << "ncols: " << grid->getNcols(false) << "\n";
  std::cout << "nrows: " << grid->getNrows(false) << "\n";
  std::cout << "xllcorner: " << grid->getTopLeftGeo(false).lon.get_degrees()
            << "\n";
  std::cout << "yllcorner: " << grid->getTopLeftGeo(false).lat.get_degrees()
            << "\n";
  std::cout << "cellsize: " << grid->getCellsize() << "\n";
  std::cout << "NODATA_value: " << grid->getNodataValue() << "\n";

  return grid;
}

// Function to load the ASCII Grid
Ref<ElevationGrid> readASCIIGrid(const godot::String& filename) {
  Ref<FileAccess> gd_file = FileAccess::open(filename, FileAccess::READ);
  if (!gd_file->is_open()) {
    WARN_PRINT("Could not open file " + filename);
  }

  std::istringstream file(std::string(gd_file->get_as_text().ascii().ptr()));

  gd_file->close();

  Ref<ElevationGrid> grid = memnew(ElevationGrid(false));
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
    grid->setTopLeftGeo(
        GeoCoords(Longitude::degrees(left),
                  Latitude::degrees(bottom + nrows * cellsize)));
  }

  std::getline(file, line);

  // Initialize the heightmap
  std::vector<PackedFloat64Array> heightmap(grid->getNrows(false),
                                            PackedFloat64Array());
  // Read the elevation data
  for (int i = 0; i < grid->getNrows(false); i++) {
    std::getline(file, line);
    std::istringstream iss(line);
    heightmap[i].resize(grid->getNcols(false));
    for (int j = 0; j < grid->getNcols(false); j++) {
      iss >> heightmap[i][j];
    }
  }

  auto gd_heightmap = TypedArray<PackedFloat64Array>();
  gd_heightmap.resize(grid->getNrows(false));
  for (int i = 0; i < grid->getNrows(false); i++) {
    gd_heightmap[i] = heightmap[i];
  }

  grid->setHeightmap(gd_heightmap);

  return grid;
}

// Function to extract a subgrid based on latitude and longitude bounds
Ref<ElevationGrid> extractSubgrid(const ElevationGrid& grid,
                                  const GeoCoords& start,
                                  const GeoCoords& end) {
  // Calculate row and column indices (same as in GDAL code)
  int rowStart = static_cast<int>(
      std::floor((grid.getTopLeftGeo(false).lat - end.lat).get_degrees() /
                 grid.getCellsize()));
  int rowEnd = static_cast<int>(
      std::floor((grid.getTopLeftGeo(false).lat - start.lat).get_degrees() /
                 grid.getCellsize()));
  int colStart = static_cast<int>(
      std::floor((start.lon - grid.getTopLeftGeo(false).lon).get_degrees() /
                 grid.getCellsize()));
  int colEnd = static_cast<int>(
      std::floor((end.lon - grid.getTopLeftGeo(false).lon).get_degrees() /
                 grid.getCellsize()));

  // One cell padding (to ensure seamless tiles)
  rowStart--;
  rowEnd++;
  colStart--;
  colEnd++;

  rowStart = std::max(0, rowStart);
  rowEnd = std::min(grid.getNrows(false) - 1, rowEnd);
  colStart = std::max(0, colStart);
  colEnd = std::min(grid.getNcols(false) - 1, colEnd);

  // Extract the subgrid heightmap
  std::vector<PackedFloat64Array> sub_heightmap;
  const auto& heightmap = grid.getHeightmap();
  for (int i = rowStart; i <= rowEnd; ++i) {
    sub_heightmap.push_back(static_cast<PackedFloat64Array>(heightmap[i])
                                .slice(colStart, colEnd + 1));
  }

  godot::TypedArray<godot::PackedFloat64Array> gd_heightmap;
  gd_heightmap.resize(sub_heightmap.size());
  for (int i = 0; i < static_cast<int>(sub_heightmap.size()); i++) {
    gd_heightmap[i] = sub_heightmap[i];
  }

  Ref<ElevationGrid> subgrid = memnew(ElevationGrid(true));
  subgrid->setNcols(colEnd - colStart + 1);
  subgrid->setNrows(rowEnd - rowStart + 1);
  subgrid->setCellsize(grid.getCellsize());
  subgrid->setTopLeftGeo(
      grid.getTopLeftGeo(false) +
      GeoCoords(Longitude::degrees(grid.getCellsize() * colStart),
                Latitude::degrees(-grid.getCellsize() * rowStart)));
  subgrid->setHeightmap(gd_heightmap);
  subgrid->set_geo_map(grid.get_geo_map());

  return subgrid;
}

godot::Ref<ElevationGrid> ElevationParser::import(
    godot::Ref<ParserOutputFileHandle> output_file_handle,
    godot::Ref<GeoMap> geomap) {
  Ref<ElevationGrid> grid = nullptr;

  if (filename.ends_with(".asc")) {
    grid = readASCIIGrid(filename.ascii().ptr());
  } else if (filename.ends_with(".tif")) {
    grid = readGeoTIFF(ProjectSettings::get_singleton()
                           ->globalize_path(filename)
                           .ascii()
                           .ptr());
  } else {
    WARN_PRINT("Unsupported file format.");
    return {};
  }

  if (geomap.is_null()) {
    geomap = godot::Ref<GeoMap>(memnew(EquirectangularGeoMap(
        grid->getTopLeftGeo(false) -
            GeoCoords(
                Longitude::zero(),
                Latitude::degrees(grid->getCellsize() * grid->getNrows(false))),
        grid->getTopLeftGeo(false) +
            GeoCoords(
                Longitude::degrees(grid->getCellsize() * grid->getNcols(false)),
                Latitude::zero()))));
  }

  grid->set_geo_map(geomap);

  WARN_PRINT("Loaded grid with " + String::num_int64(grid->getNcols(false)) +
             " columns and " + String::num_int64(grid->getNrows(false)) +
             " rows " + " and cellsize " +
             String::num_real(grid->getCellsize()));
  WARN_PRINT("Grid top left corner is at " +
             String::num_real(grid->getTopLeftGeo(false).lon.get_degrees()) +
             " " +
             String::num_real(grid->getTopLeftGeo(false).lat.get_degrees()));

  auto shader_nodes = this->get_shader_nodes();
  auto tilemap = (memnew(EquirectangularTileMap));

  // Note that we cannot accept partial tiles as they would produce an
  // incomplete heightmap. So we move the corners by 1 diagonally to the inside
  // to only consider full tiles.
  auto ul_tile =
      tilemap->get_tile_geo(grid->getTopLeftGeo(false)) + Vector2i(1, 1);
  auto lr_tile = tilemap->get_tile_geo(
                     grid->getTopLeftGeo(false) +
                     GeoCoords(Longitude::degrees(grid->getCellsize() *
                                                  grid->getNcols(false)),
                               Latitude::degrees(-grid->getCellsize() *
                                                 grid->getNrows(false)))) -
                 Vector2i(1, 1);
  std::cout << "UL tile: " << ul_tile.x << " " << ul_tile.y << std::endl;
  std::cout << "LR tile: " << lr_tile.x << " " << lr_tile.y << std::endl;
  std::atomic<int32_t> total_tiles = (lr_tile - ul_tile + Vector2i(1, 1)).x *
                                     (lr_tile - ul_tile + Vector2i(1, 1)).y;

  ThreadPool tp(8);
  std::mutex mutex;
  for (int y = ul_tile.y; y <= lr_tile.y; y++) {
    for (int x = ul_tile.x; x <= lr_tile.x; x++) {
      tp.enqueue([&total_tiles, tilemap, grid, x, y, output_file_handle,
                  shader_nodes, &mutex]() {
        if (total_tiles % 1000 == 0)
          std::cout << "Tiles left " << total_tiles << std::endl;
        auto tile = Vector2i(x, y);
        auto tile_ul = tilemap->get_tile_top_left_geo(tile);
        auto tile_lr = tilemap->get_tile_top_left_geo(tile + Vector2i(1, -1));
        // auto tile_bytes = output_file_handle->get_parser(tile);
        auto subgrid = extractSubgrid(*grid.ptr(), tile_ul, tile_lr);
        for (int i = 0; i < shader_nodes.size(); i++) {
          godot::StreamPeerBuffer* parser;
          {
            std::scoped_lock lock(mutex);
            parser = output_file_handle->get_parser(tile, i);
          }
          Object::cast_to<Node>(shader_nodes[i])
              ->call("import_grid", subgrid, parser);
        }
        total_tiles--;
      });
    }
  }

  return grid;
}

void ElevationParser::_bind_methods() {
  ClassDB::bind_method(D_METHOD("import", "output_file_handle", "geomap"),
                       &ElevationParser::import);
  ClassDB::bind_method(D_METHOD("set_filename", "value"),
                       &ElevationParser::set_filename);
  ClassDB::bind_method(D_METHOD("get_filename"),
                       &ElevationParser::get_filename);

  ADD_PROPERTY(PropertyInfo(Variant::STRING, "filename", PROPERTY_HINT_FILE,
                            "*.asc,*.tif"),
               "set_filename", "get_filename");
}
