#include "HeightMap.h"

#include <algorithm>
#include <functional>
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <iomanip>
#include <queue>

#include "../../../../src/import/osm_parser/OSMHeightmap.h"
#include "../../../src/util/PolyUtil.h"
#include "../../common/3D/RenderUtil3D.h"


using namespace godot;

struct CellEdges {
  std::vector<std::vector<std::pair<double, double>>> edges;
  std::vector<std::vector<std::pair<double, double>>> fully_contained_edges;
};

template <typename T, typename U>
struct std::hash<std::pair<T, U>> {
 public:
  size_t operator()(std::pair<int, int> x) const throw() {
    return hash<T>()(x.first) ^ hash<U>()(x.second);
  }
};

enum class CoastTile {
  NotDetermined,
  Sea,
  Coast,
  Land,

  MaxValue = Land
};

void generate_full_rectangle(const std::pair<int, int>& col_row,
                             const GeoCoords& cell_pos, double cellrad,
                             ElevationGrid* grid, bool flat, Array& arrays,
                             GeoMap* coastline_geomap);
void generate_partial_rectangle(
    const std::pair<int, int>& col_row,
    std::vector<std::vector<std::pair<double, double>>>& arrays,
    std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>&);
bool rasterizeLine(std::vector<std::vector<CoastTile>>& grid, double x1,
                   double y1, double x2, double y2,
                   std::unordered_map<std::pair<int, int>,
                                      std::unique_ptr<CellEdges>>& cell_edges,
                   std::vector<std::pair<double, double>>*& this_edge,
                   std::pair<int, int>& initial_cell,
                   std::vector<std::pair<double, double>>&);
void bfs(std::vector<std::vector<CoastTile>>& grid, int x, int y,
         CoastTile value);

void HeightMap::import_polygons_geo(godot::Array polygons, GeoMap* geomap) {
  coastline_polygons = polygons;
  coastline_geomap = geomap;
}

int pnpoly(int nvert, std::vector<std::pair<double, double>>& verts,
           double testx, double testy) {
  int i, j, c = 0;
  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    if (((verts[i].second > testy) != (verts[j].second > testy)) &&
        (testx < (verts[j].first - verts[i].first) * (testy - verts[i].second) /
                         (verts[j].second - verts[i].second) +
                     verts[i].first))
      c = !c;
  }
  return c;
}

bool approximately_equal(double a, double b) { return std::abs(a - b) < 1e-12; }

GeoCoords grid_to_geo(ElevationGrid& grid,
                      const std::pair<double, double>& gridPos) {
  auto cellrad = grid.getCellsize() / 180.0 * Math_PI;
  return grid.getTopLeftGeo(true) +
         GeoCoords(Longitude::radians(cellrad * gridPos.first),
                   Latitude::radians(-cellrad * gridPos.second));
}

void generate_coastline(std::vector<std::pair<double, double>>& poly,
                        Array& arrays, GeoMap& coastline_geomap,
                        ElevationGrid* grid, bool connect_last_to_first) {
  PackedVector3Array poly3d, normals;
  Ref<ElevationHeightmap> hmap = memnew(ElevationHeightmap);
  hmap->set_elevation_grid(grid);

  for (int k = 0;
       k < static_cast<int>(poly.size()) - (connect_last_to_first ? 0 : 1);
       k++) {
    auto v1_geo = grid_to_geo(*grid, poly[k]);
    auto v2_geo = grid_to_geo(*grid, poly[(k + 1) % poly.size()]);

    auto v1_world = coastline_geomap.geo_to_world(v1_geo);
    auto v2_world = coastline_geomap.geo_to_world(v2_geo);

    // Rectangle that goes to 0
    poly3d.push_back(v1_world + Vector3(0, hmap->getElevation(v1_geo), 0));
    poly3d.push_back(v2_world + Vector3(0, hmap->getElevation(v2_geo), 0));
    poly3d.push_back(v2_world);

    poly3d.push_back(v2_world);
    poly3d.push_back(v1_world);
    poly3d.push_back(v1_world + Vector3(0, hmap->getElevation(v1_geo), 0));

    auto normal = (v2_world - v1_world).cross(Vector3(0, 1, 0)).normalized();
    for (int l = 0; l < 6; l++) {
      normals.push_back(normal);
    }
  }
  arrays[Mesh::ARRAY_VERTEX].call("append_array", poly3d);
  arrays[Mesh::ARRAY_NORMAL].call("append_array", normals);
}

void HeightMap::import_grid(ElevationGrid* grid,
                            godot::Ref<godot::StreamPeerBuffer> tile_bytes) {
  // auto children = get_children();
  // for (int j = 0; j < children.size(); j++) {
  //     remove_child(Object::cast_to<Node>(children[j]));
  // }
  if (!(std::abs(grid->getTopLeftGeo(true).lon.get_degrees() - 24.451) <
            0.005 &&
        std::abs(grid->getTopLeftGeo(true).lat.get_degrees() - 36.697) <
            0.005)) {
    // return;
  }
  std::cout << std::fixed;
  std::cout << std::setprecision(4);
  auto cell_pos = grid->getTopLeftGeo(true);
  auto left = cell_pos.lon;
  std::vector<std::vector<CoastTile>> land_cells(
      grid->getNrows(true),
      std::vector<CoastTile>(grid->getNcols(true), CoastTile::NotDetermined));
  std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>
      coastline_cell_edges;
  std::vector<std::pair<std::vector<std::pair<double, double>>, bool>>
      coastline_polylines;
  std::vector<std::vector<std::pair<double, double>>> partial_polygons;

  // Grid rectangle
  auto cellrad = grid->getCellsize() / 180.0 * Math_PI;

  for (int i = 0; i < coastline_polygons.size(); i++) {
    Array poly = static_cast<Array>(coastline_polygons[i]);
    for (int j = 0; j < poly.size(); j++) {
      Array part = static_cast<Array>(poly[j]);
      PackedFloat64Array part_x = part[0], part_y = part[1];

      DEV_ASSERT(part_x.size() == part_y.size());
      DEV_ASSERT(!part_x.is_empty());
      std::vector<std::pair<double, double>>* this_edge = nullptr;

      // Initial edge will be incomplete, so we need to store it until we reach
      // the last edge.
      std::vector<std::pair<double, double>> initial_edge;
      std::pair<int, int> initial_cell = {-1, -1};

      // Coastline polyline
      std::vector<std::pair<double, double>> coastline_polyline;
      bool broken_coastline = false;
      this_edge = &initial_edge;
      for (int k = 0; k < part_x.size(); k++) {
        std::pair<double, double> v1(part_x[k], part_y[k]);
        std::pair<double, double> v2(part_x[(k + 1) % part_x.size()],
                                     part_y[(k + 1) % part_y.size()]);

        double v1_tile_x =
            (v1.first - grid->getTopLeftGeo(true).lon.get_radians()) / cellrad;
        double v1_tile_y =
            -(v1.second - grid->getTopLeftGeo(true).lat.get_radians()) /
            cellrad;
        double v2_tile_x =
            (v2.first - grid->getTopLeftGeo(true).lon.get_radians()) / cellrad;
        double v2_tile_y =
            -(v2.second - grid->getTopLeftGeo(true).lat.get_radians()) /
            cellrad;

        if (!rasterizeLine(land_cells, v1_tile_x, v1_tile_y, v2_tile_x,
                           v2_tile_y, coastline_cell_edges, this_edge,
                           initial_cell, coastline_polyline) &&
            !coastline_polyline.empty() && cliffs) {
          coastline_polylines.push_back({std::move(coastline_polyline), false});
          broken_coastline = true;
        }
      }

      if (this_edge == &initial_edge) {  // Fully contained edges
        if (!coastline_cell_edges[initial_cell]) {
          coastline_cell_edges[initial_cell] = std::make_unique<CellEdges>();
        }

        coastline_cell_edges[initial_cell]->fully_contained_edges.push_back(
            initial_edge);
      } else if (coastline_cell_edges[initial_cell] &&
                 this_edge == &coastline_cell_edges[initial_cell]
                                   ->edges.back()) {  // Assuming the coastline
                                                      // has made a full loop
        if (!approximately_equal(initial_edge[0].first,
                                 this_edge->back().first) ||
            !approximately_equal(initial_edge[0].second,
                                 this_edge->back().second)) {
          ERR_PRINT_ED("Coastline loop is not closed");
          ERR_PRINT_ED(String::num(initial_edge[0].first) + " " +
                       String::num(initial_edge[0].second) + " " +
                       String::num(this_edge->back().first) + " " +
                       String::num(this_edge->back().second));
          ERR_PRINT_ED(String::num(initial_edge[1].first) + " " +
                       String::num(initial_edge[1].second));
        } else {
          for (int k = 1; k < static_cast<int>(initial_edge.size());
               k++) {  // skip the first point, as it's the same as the last one
            this_edge->push_back(initial_edge[k]);
          }
        }
      } else {  // The coastline hasn't made a full loop, but started from the
                // grid boundary as it was clipped.
        // std::cout << "We just lost " << initial_edge.size() << " points ig?"
        // << std::endl;
        DEV_ASSERT(!initial_edge.empty());

        if (!(approximately_equal(initial_edge[0].first, 0.0) ||
              approximately_equal(initial_edge[0].first, 1.0) ||
              approximately_equal(initial_edge[0].second, 0.0) ||
              approximately_equal(initial_edge[0].second, 1.0)))
          ERR_PRINT(
              "Coastline hasn't made a full loop, is not fully contained, but "
              "hasn't started from the grid boundary.");

        // TODO check if (initial_cell.x == 0 || initial_cell.x = max_grid_x ||
        // initial_cell.y == 0 || initial_cell == max_grid_y)
        if (!coastline_cell_edges[initial_cell]) {
          coastline_cell_edges[initial_cell] = std::make_unique<CellEdges>();
        }
        coastline_cell_edges[initial_cell]->edges.push_back(
            std::move(initial_edge));
      }

      if (cliffs)
        coastline_polylines.push_back(
            {std::move(coastline_polyline), !broken_coastline});
    }
  }

  // UtilityFunctions::print("Total rows ", grid->getNrows());

  for (int row = 0; row < grid->getNrows(true); row++) {
    // if (row % 20 == 0)
    //     UtilityFunctions::print("Row ", row, " out of ", grid->getNrows());

    for (int col = 0; col < grid->getNcols(true); col++) {
      if (land_cells[row][col] == CoastTile::NotDetermined) {
        bool sea_cell = true;
        bool found = false;
        for (int i = 0; i < coastline_polygons.size(); i++) {
          Array parts = static_cast<Array>(coastline_polygons[i]);
          for (int j = 0; j < parts.size(); j++) {
            Array part = static_cast<Array>(parts[j]);
            PackedFloat64Array part_x = part[0], part_y = part[1];

            std::vector<std::pair<double, double>> part_poly;
            for (int k = 0; k < part_x.size(); k++) {
              part_poly.push_back(std::make_pair(part_x[k], part_y[k]));
            }
            GeoCoords test_point =
                cell_pos + GeoCoords(Longitude::radians(cellrad / 2.0),
                                     Latitude::radians(-cellrad / 2.0));
            if (pnpoly(part_poly.size(), part_poly,
                       test_point.lon.get_radians(),
                       test_point.lat.get_radians())) {
              sea_cell = j > 0;  // If it's not the first part, it's a sea cell
              found = true;
            }
          }

          if (found) break;
        }

        bfs(land_cells, col, row, sea_cell ? CoastTile::Sea : CoastTile::Land);
      }
      if (land_cells[row][col] == CoastTile::Coast)
        generate_partial_rectangle({col, row}, partial_polygons,
                                   coastline_cell_edges);

      // Advance on X
      cell_pos.lon = cell_pos.lon + Longitude::radians(cellrad);
    }

    // Advance on Y, reset X
    cell_pos.lon = left;
    cell_pos.lat = cell_pos.lat - Latitude::radians(cellrad);
  }

  // RenderUtil3D::area_poly(this, "GroundMesh", arrays, Color(0.745098,
  // 0.745098, 0.745098));

  // Save the tile

  // Save the grid
  tile_bytes->put_u64(grid->getNcols(false));
  tile_bytes->put_u64(grid->getNrows(false));
  tile_bytes->put_double(grid->getCellsize());
  tile_bytes->put_double(grid->getTopLeftGeo(false).lon.get_degrees());
  tile_bytes->put_double(grid->getTopLeftGeo(false).lat.get_degrees());
  tile_bytes->put_var(grid->getHeightmap());
  {
    auto geomap =
        Object::cast_to<EquirectangularGeoMap>(grid->get_geo_map().ptr());
    tile_bytes->put_double(geomap->get_geo_origin_longitude_degrees());
    tile_bytes->put_double(geomap->get_geo_origin_latitude_degrees());
    tile_bytes->put_double(geomap->get_scale_factor());
  }

  // Run-Length Encoding for land_cells
  DEV_ASSERT(!land_cells.empty());
  const auto placeholder_total_runs_pos = tile_bytes->get_position();
  tile_bytes->put_64(0);  // Placeholder for the length of the RLE
  CoastTile last = CoastTile::NotDetermined;
  uint16_t run_length = 0;
  uint64_t total_runs = 0;
  const uint16_t maximum_run_length = std::numeric_limits<uint16_t>::max() >> 2;
  for (int row = 0; row < static_cast<int>(land_cells.size()); row++) {
    for (int col = 0; col < static_cast<int>(land_cells[row].size()); col++) {
      if (land_cells[row][col] != last || run_length == maximum_run_length) {
        if (last != CoastTile::NotDetermined) {
          tile_bytes->put_16(static_cast<uint16_t>(last) + (run_length << 2));
          total_runs++;
        }
        last = land_cells[row][col];
        run_length = 1;
      } else {
        run_length++;
      }
    }
  }

  tile_bytes->put_16(static_cast<uint16_t>(last) + (run_length << 2));
  total_runs++;
  tile_bytes->seek(placeholder_total_runs_pos);
  tile_bytes->put_64(total_runs);
  tile_bytes->seek(tile_bytes->get_size());

  // Save partial polygons
  tile_bytes->put_64(partial_polygons.size());
  for (auto& polygon : partial_polygons) {
    tile_bytes->put_64(polygon.size());
    for (auto& point : polygon) {
      tile_bytes->put_double(point.first);
      tile_bytes->put_double(point.second);
    }
  }

  // Save coastline polylines
  tile_bytes->put_64(coastline_polylines.size());
  for (auto& polyline : coastline_polylines) {
    tile_bytes->put_u64(polyline.first.size());
    tile_bytes->put_u8(polyline.second);
    for (auto& point : polyline.first) {
      tile_bytes->put_double(point.first);
      tile_bytes->put_double(point.second);
    }
  }
}

Vector3 get_normal_from_heights(double L, double R, double B, double T) {
  return Vector3((L - R) / 2.0, 1, (T - B) / 2.0).normalized();
}

Vector3 get_normal_from_pos(ElevationHeightmap* hmap, const GeoCoords& pos,
                            double delta) {
  delta *= hmap->get_elevation_grid()->getCellsize();
  return get_normal_from_heights(
      hmap->getElevation(
          pos + GeoCoords(Longitude::degrees(-delta), Latitude::degrees(0))),
      hmap->getElevation(
          pos + GeoCoords(Longitude::degrees(delta), Latitude::degrees(0))),
      hmap->getElevation(
          pos + GeoCoords(Longitude::degrees(0), Latitude::degrees(-delta))),
      hmap->getElevation(
          pos + GeoCoords(Longitude::degrees(0), Latitude::degrees(delta))));
}

void grid_space_polygon(
    ElevationGrid* grid,
    const std::vector<std::pair<double, double>>& triangulated, double cellrad,
    bool flat, Array& arrays, GeoMap* coastline_geomap,
    const GeoCoords& cell_pos) {
  PackedVector3Array poly3d;
  PackedVector3Array normals;

  Ref<ElevationHeightmap> hmap = memnew(ElevationHeightmap);
  hmap->set_elevation_grid(grid);

  for (auto& point : triangulated) {
    auto v_geo =
        cell_pos + GeoCoords(Longitude::radians(point.first * cellrad),
                             Latitude::radians(-point.second * cellrad));
    auto v_world = coastline_geomap->geo_to_world(v_geo);
    poly3d.push_back(v_world + Vector3(0, hmap->getElevation(v_geo), 0));
    if (flat) {
      normals.push_back(Vector3(0, 1, 0));
    } else {
      normals.append(get_normal_from_pos(hmap.ptr(), v_geo, 0.1));
    }
  }

  arrays[Mesh::ARRAY_VERTEX].call("append_array", poly3d);
  arrays[Mesh::ARRAY_NORMAL].call("append_array", normals);
}

void HeightMap::load_tile(godot::Ref<godot::FileAccess> fa) {
  godot::Ref<ElevationGrid> grid = memnew(ElevationGrid(true));
  {
    grid->setNcols(fa->get_64());
    grid->setNrows(fa->get_64());
    grid->setCellsize(fa->get_double());
    double lon = fa->get_double();
    double lat = fa->get_double();
    grid->setTopLeftGeo(
        GeoCoords(Longitude::degrees(lon), Latitude::degrees(lat)));
    grid->setHeightmap(fa->get_var());
    auto geomap = memnew(EquirectangularGeoMap);
    geomap->set_geo_origin_longitude_degrees(fa->get_double());
    geomap->set_geo_origin_latitude_degrees(fa->get_double());
    geomap->set_scale_factor(fa->get_double());
    grid->set_geo_map(geomap);
  }
  std::vector<std::vector<CoastTile>> land_cells(
      grid->getNrows(true),
      std::vector<CoastTile>(grid->getNcols(true), CoastTile::NotDetermined));

  // Run-Length Decoding for land_cells
  uint64_t total_runs = fa->get_64();
  uint64_t cur_pos = 0;
  for (uint64_t i = 0; i < total_runs; i++) {
    uint16_t run = fa->get_16();
    CoastTile tile = static_cast<CoastTile>(run & 3);
    uint16_t run_length = run >> 2;
    for (uint64_t j = cur_pos; j < cur_pos + static_cast<uint64_t>(run_length);
         j++) {
      land_cells[j / static_cast<uint64_t>(grid->getNcols(true))]
                [j % static_cast<uint64_t>(grid->getNcols(true))] = tile;
    }
    cur_pos += static_cast<uint64_t>(run_length);
  }

  Array arrays = RenderUtil3D::get_array_mesh_arrays(
      {Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL});

  // Load partial polygons
  std::vector<std::vector<std::pair<double, double>>> partial_polygons;
  {
    uint64_t partial_polygons_count = fa->get_64();
    for (uint64_t i = 0; i < partial_polygons_count; i++) {
      uint64_t polygon_size = fa->get_64();
      std::vector<std::pair<double, double>> polygon;
      for (uint64_t j = 0; j < polygon_size; j++) {
        double x = fa->get_double();
        double y = fa->get_double();
        polygon.push_back({x, y});
      }
      partial_polygons.push_back(std::move(polygon));
    }
  }

  // Generate mesh
  auto cell_pos = grid->getTopLeftGeo(true);
  auto left = cell_pos.lon;
  auto cellrad = grid->getCellsize() / 180.0 * Math_PI;
  Ref<PolyUtil> pu = memnew(PolyUtil);

  for (int row = 0; row < grid->getNrows(true); row++) {
    for (int col = 0; col < grid->getNcols(true); col++) {
      if (land_cells[row][col] == CoastTile::Land)
        generate_full_rectangle(
            {col, row},
            grid->getTopLeftGeo(true) +
                GeoCoords(Longitude::radians(col * cellrad),
                          Latitude::radians(-row * cellrad)),
            cellrad, grid.ptr(), flat_shading, arrays,
            grid->get_geo_map().ptr());

      // Advance on X
      cell_pos.lon = cell_pos.lon + Longitude::radians(cellrad);
    }

    // Advance on Y, reset X
    cell_pos.lon = left;
    cell_pos.lat = cell_pos.lat - Latitude::radians(cellrad);
  }

  for (auto& partial_polygon : partial_polygons) {
    grid_space_polygon(grid.ptr(),
                       pu->triangulate_with_holes(partial_polygon, {}), cellrad,
                       flat_shading, arrays, grid->get_geo_map().ptr(),
                       grid->getTopLeftGeo(true));
  }

  // Load coastline polylines
  {
    std::vector<std::pair<std::vector<std::pair<double, double>>, bool>>
        coastline_polylines;
    uint64_t coastline_polylines_count = fa->get_64();
    for (uint64_t i = 0; i < coastline_polylines_count; i++) {
      uint64_t polyline_size = fa->get_64();
      bool closed = fa->get_8();
      std::vector<std::pair<double, double>> polyline;
      for (uint64_t j = 0; j < polyline_size; j++) {
        double x = fa->get_double();
        double y = fa->get_double();
        polyline.push_back({x, y});
      }
      coastline_polylines.push_back({std::move(polyline), closed});
    }

    for (auto& polyline : coastline_polylines) {
      generate_coastline(polyline.first, arrays, *grid->get_geo_map().ptr(),
                         grid.ptr(), polyline.second);
    }
  }

  if (!static_cast<PackedVector3Array>(arrays[Mesh::ARRAY_VERTEX]).is_empty())
    RenderUtil3D::area_poly(
        this,
        "GroundMesh_" +
            String::num(grid->getTopLeftGeo(true).lon.get_degrees(), 3) + "_" +
            String::num(grid->getTopLeftGeo(true).lat.get_degrees(), 3),
        arrays, Color(0.745098, 0.745098, 0.745098), true);

  if (std::abs(grid->getTopLeftGeo(true).lon.get_degrees() - 24.451) < 0.005 &&
      std::abs(grid->getTopLeftGeo(true).lat.get_degrees() - 36.697) < 0.005) {
    // Print land_cells
    for (int row = 0; row < land_cells.size(); row++) {
      for (int col = 0; col < land_cells[row].size(); col++) {
        std::cout << static_cast<int>(land_cells[row][col]) << " ";
      }
      std::cout << std::endl;
    }
  }
}

void HeightMap::_bind_methods() {
  ClassDB::bind_method(D_METHOD("import_polygons_geo", "polygons", "geomap"),
                       &HeightMap::import_polygons_geo);
  ClassDB::bind_method(D_METHOD("import_grid", "grid"),
                       &HeightMap::import_grid);
  ClassDB::bind_method(D_METHOD("load_tile", "fa"), &HeightMap::load_tile);
  ClassDB::bind_method(D_METHOD("set_flat_shading", "flat"),
                       &HeightMap::set_flat_shading);
  ClassDB::bind_method(D_METHOD("get_flat_shading"),
                       &HeightMap::get_flat_shading);

  ClassDB::bind_method(D_METHOD("set_cliffs", "cliffs"),
                       &HeightMap::set_cliffs);
  ClassDB::bind_method(D_METHOD("get_cliffs"), &HeightMap::get_cliffs);

  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flat_shading"), "set_flat_shading",
               "get_flat_shading");
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cliffs"), "set_cliffs",
               "get_cliffs");
}

void generate_full_rectangle(const std::pair<int, int>& col_row,
                             const GeoCoords& cell_pos, double cellrad,
                             ElevationGrid* grid, bool flat, Array& arrays,
                             GeoMap* coastline_geomap) {
  std::vector<std::pair<double, double>> poly;
  poly.push_back({0, 0});
  poly.push_back({1, 0});
  poly.push_back({1, 1});

  poly.push_back({0, 0});
  poly.push_back({1, 1});
  poly.push_back({0, 1});
  // poly.push_back({col_row.first + 1, col_row.second});
  // poly.push_back({col_row.first + 1, col_row.second + 1});

  // poly.push_back(col_row);
  // poly.push_back({col_row.first + 1, col_row.second + 1});
  // poly.push_back({col_row.first, col_row.second + 1});

  grid_space_polygon(grid, poly, cellrad, flat, arrays, coastline_geomap,
                     cell_pos);
}

struct CellCoastlineTip {
  std::reference_wrapper<std::vector<std::pair<double, double>>> edges;
  int index;
  bool reverse;
};

void generate_partial_rectangle(
    const std::pair<int, int>& col_row,
    std::vector<std::vector<std::pair<double, double>>>& partial_polygons,
    std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>&
        cell_edges) {
  if (!cell_edges[col_row]) {
    std::cout << "Invalid cell " << col_row.first << " " << col_row.second
              << std::endl;
    return;
  }

  std::vector<std::vector<std::pair<double, double>>>& all_edges =
      cell_edges[col_row]->edges;
  DEV_ASSERT(!(all_edges.empty() &&
               cell_edges[col_row]->fully_contained_edges.empty()));

  std::vector<CellCoastlineTip> edges_left;
  std::vector<CellCoastlineTip> edges_right;
  std::vector<CellCoastlineTip> edges_top;
  std::vector<CellCoastlineTip> edges_bottom;

  for (int i = 0; i < static_cast<int>(all_edges.size()); i++) {
    auto& edge = all_edges[i];
    std::vector<std::pair<int, bool>> check = {{0, false},
                                               {edge.size() - 1, true}};
    for (auto [check_index, reverse] : check) {
      CellCoastlineTip tip = {std::ref(edge), i, reverse};
      auto& point = edge[check_index];
      if (approximately_equal(point.first, 0.0)) {
        edges_left.push_back(std::move(tip));
      } else if (approximately_equal(point.first, 1.0)) {
        edges_right.push_back(std::move(tip));
      } else if (approximately_equal(point.second, 0.0)) {
        edges_bottom.push_back(std::move(tip));
      } else if (approximately_equal(point.second, 1.0)) {
        edges_top.push_back(std::move(tip));
      } else {
        std::cout << "Unreachable " << point.first << " " << point.second
                  << std::endl;
        return;
        // DEV_ASSERT(false); // unreachable
      }
    }
  }

  // Sort edges
  std::sort(
      edges_left.begin(), edges_left.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].second >
               b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].second;
      });
  std::sort(
      edges_right.begin(), edges_right.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].second <
               b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].second;
      });
  std::sort(
      edges_top.begin(), edges_top.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].first >
               b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].first;
      });
  std::sort(
      edges_bottom.begin(), edges_bottom.end(),
      [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].first <
               b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].first;
      });

  std::vector<bool> visited(all_edges.size(), false);

  for (int i = 0; i < static_cast<int>(all_edges.size()); i++) {
    if (visited[i]) continue;

    auto& this_edge = all_edges[i];
    visited[i] = true;

    std::vector<std::pair<double, double>> poly = this_edge;

    std::pair<double, double> starting_point = poly[0];

    while (true) {
      CellCoastlineTip* next_tip = nullptr;

      if (approximately_equal(poly.back().first, 0.0) &&
          !approximately_equal(poly.back().second, 0.0)) {
        // Binary search to find the index before the current point

        auto next_point = std::upper_bound(
            edges_left.begin(), edges_left.end(), poly.back(),
            [](const auto& a, const auto& b) {
              return a.second >
                     b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0]
                         .second;
            });

        if (next_point == edges_left.end()) {
          poly.push_back({0.0, 0.0});
          continue;
        }

        next_tip = &*(next_point);

      } else if (approximately_equal(poly.back().second, 1.0) &&
                 !approximately_equal(poly.back().first, 0.0)) {
        // Binary search to find the index before the current point

        auto next_point = std::upper_bound(
            edges_top.begin(), edges_top.end(), poly.back(),
            [](const auto& a, const auto& b) {
              return a.first >
                     b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0]
                         .first;
            });

        if (next_point == edges_top.end()) {
          poly.push_back({0.0, 1.0});
          continue;
        }

        next_tip = &*(next_point);
      } else if (approximately_equal(poly.back().first, 1.0) &&
                 !approximately_equal(poly.back().second, 1.0)) {
        // Binary search to find the index before the current point

        auto next_point = std::upper_bound(
            edges_right.begin(), edges_right.end(), poly.back(),
            [](const auto& a, const auto& b) {
              return a.second <
                     b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0]
                         .second;
            });

        if (next_point == edges_right.end()) {
          poly.push_back({1.0, 1.0});
          continue;
        }

        next_tip = &*(next_point);
      } else if (approximately_equal(poly.back().second, 0.0) &&
                 !approximately_equal(poly.back().first, 1.0)) {
        // Binary search to find the index before the current point

        auto next_point = std::upper_bound(
            edges_bottom.begin(), edges_bottom.end(), poly.back(),
            [](const auto& a, const auto& b) {
              return a.first <
                     b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0]
                         .first;
            });

        if (next_point == edges_bottom.end()) {
          poly.push_back({1.0, 0.0});
          continue;
        }

        next_tip = &*(next_point);
      }

      DEV_ASSERT(next_tip != nullptr);
      if ((!next_tip->reverse && next_tip->edges.get()[0] == starting_point) ||
          (next_tip->reverse &&
           next_tip->edges.get().back() == starting_point)) {
        poly.push_back(starting_point);
        break;
      }
      visited[next_tip->index] = true;
      if (!next_tip->reverse) {
        for (auto& point : next_tip->edges.get()) {
          poly.push_back(point);
        }
      } else {
        for (int i = next_tip->edges.get().size() - 1; i >= 0; i--) {
          poly.push_back(next_tip->edges.get()[i]);
        }
      }
    }

    std::vector<std::pair<double, double>> grid_poly;
    for (auto& [x, y] : poly) {
      grid_poly.push_back({col_row.first + x, col_row.second + y});
    }
    partial_polygons.push_back(std::move(grid_poly));
  }

  for (auto& poly : cell_edges[col_row]->fully_contained_edges) {
    std::vector<std::pair<double, double>> grid_poly;
    for (auto& [x, y] : poly) {
      grid_poly.push_back({col_row.first + x, col_row.second + y});
    }
    partial_polygons.push_back(std::move(grid_poly));
  }
}

const int LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8;

// Determine the region code for a point
int computeOutCode(double x, double y, int grid_x, int grid_y) {
  int code = 0;
  if (x < 0)
    code |= LEFT;
  else if (x > grid_x)
    code |= RIGHT;
  if (y < 0)
    code |= BOTTOM;
  else if (y > grid_y)
    code |= TOP;
  return code;
}

// Cohen-Sutherland clipping algorithm
bool clipLine(double& x1, double& y1, double& x2, double& y2, int grid_x,
              int grid_y) {
  int outcode1 = computeOutCode(x1, y1, grid_x, grid_y);
  int outcode2 = computeOutCode(x2, y2, grid_x, grid_y);

  bool accept = false;

  while (true) {
    if (!(outcode1 | outcode2)) {  // Both points inside grid
      accept = true;
      break;
    } else if (outcode1 &
               outcode2) {  // Both points outside grid, in the same region
      break;
    } else {
      // Calculate intersection with the grid boundary
      double x, y;
      int outcodeOut = outcode1 ? outcode1 : outcode2;

      if (outcodeOut & TOP) {  // Point is above grid
        x = x1 + (x2 - x1) * (grid_y - y1) / (y2 - y1);
        y = grid_y;
      } else if (outcodeOut & BOTTOM) {  // Point is below grid
        x = x1 + (x2 - x1) * (0 - y1) / (y2 - y1);
        y = 0;
      } else if (outcodeOut & RIGHT) {  // Point is to the right of grid
        y = y1 + (y2 - y1) * (grid_x - x1) / (x2 - x1);
        x = grid_x;
      } else if (outcodeOut & LEFT) {  // Point is to the left of grid
        y = y1 + (y2 - y1) * (0 - x1) / (x2 - x1);
        x = 0;
      }

      // Replace point outside grid with intersection point
      if (outcodeOut == outcode1) {
        x1 = x;
        y1 = y;
        outcode1 = computeOutCode(x1, y1, grid_x, grid_y);
      } else {
        x2 = x;
        y2 = y;
        outcode2 = computeOutCode(x2, y2, grid_x, grid_y);
      }
    }
  }

  return accept;
}

/**
 * @brief Rasterize a coastline line by stepping one cell at a time. This
 * function has multiple responsibilities, because stepping one cell at a time
 * allows us to kill many birds with one stone:
 *  - Set the grid cells the line passes through to 1. This way we can only
 * perform the expensive partial rectangle generation on these cells, saving us
 * precious time.
 *  - For each cell the line passes through, store the line in the cell_edges
 * map, clipped to the cell. This way we can efficiently generate partial
 * rectangles later.
 *  - For each step, add the step position to the coastline_polyline vector.
 * This way the polyline is grid-aligned and can be used to generate cliffs that
 * perfectly align with the heightmap.
 * @param grid The land cells grid (full sea/partial/full land)
 * @param x1 The x-coordinate of the start of the line, in grid space
 * @param y1 The y-coordinate of the start of the line, in grid space
 * @param x2 The x-coordinate of the end of the line, in grid space
 * @param y2 The y-coordinate of the end of the line, in grid space
 * @param cell_edges A map of cell coordinates to a vector of edges that pass
 * through the cell
 * @param this_edge A pointer to the current edge vector in the cell_edges map.
 * Moving to another cell pushes a new vector to that cell and sets this_edge to
 * point to it. Precondition: must be a valid pointer if the current point is
 * inside the grid, nullptr otherwise.
 * @param initial_cell The cell the line started in. Set it to {-1, -1}
 * initially. You can use it to determine if the line has made a full loop.
 * @param coastline_polyline The vector to store the grid-aligned polyline in.
 * @returns True if the line is completely outside the grid, false otherwise.
 *
 */
bool rasterizeLine(std::vector<std::vector<CoastTile>>& grid, double x1,
                   double y1, double x2, double y2,
                   std::unordered_map<std::pair<int, int>,
                                      std::unique_ptr<CellEdges>>& cell_edges,
                   std::vector<std::pair<double, double>>*& this_edge,
                   std::pair<int, int>& initial_cell,
                   std::vector<std::pair<double, double>>& coastline_polyline) {
  const auto max_grid_x = static_cast<int>(grid[0].size()),
             max_grid_y = static_cast<int>(grid.size());
  bool break_coastline = false;
  {
    double prev_x1 = x1, prev_y1 = y1;
    double prev_x2 = x2, prev_y2 = y2;
    if (!clipLine(
            x1, y1, x2, y2, max_grid_x,
            max_grid_y)) {  // Clip line to grid; exit if completely outside
      // Ensure we have to create a new edge if
      // 1. We have already started creating edges
      // 2. We left out of bounds
      if (initial_cell != std::make_pair(-1, -1)) this_edge = nullptr;
      return false;
    }

    // If the line's starting point is clipped, we need to create a new edge
    if ((prev_x1 != x1 || prev_y1 != y1) &&
        initial_cell != std::make_pair(-1, -1)) {
      this_edge = nullptr;
    }
    if ((prev_x2 != x2 || prev_y2 != y2) &&
        initial_cell != std::make_pair(-1, -1)) {
      break_coastline = true;
    }
  }

  double dx = x2 - x1;
  double dy = y2 - y1;
  const double distance = std::sqrt(dx * dx + dy * dy);

  if (distance == 0.0) {
    return true;
  }

  double dirX = dx / distance, dirY = dy / distance;

  double x = x1;
  double y = y1;
  int gridX = static_cast<int>(x), gridY = static_cast<int>(y);

  if (gridX == max_grid_x) gridX--;
  if (gridY == max_grid_y) gridY--;

  // Edge case: coastline is interrupted as it goes out of bounds, it goes back
  // in again so we need to create a new edge in the entry cell
  if (!this_edge) {
    if (!(x1 == 0 || x1 == max_grid_x || y1 == 0 || y1 == max_grid_y)) {
      ERR_PRINT_ED("Interrupted line was not clipped to the edge of the grid");
    }

    if (!cell_edges[{gridX, gridY}])
      cell_edges[{gridX, gridY}] = std::make_unique<CellEdges>();

    cell_edges[{gridX, gridY}]->edges.push_back(
        std::vector<std::pair<double, double>>());
    this_edge = &cell_edges[{gridX, gridY}]->edges.back();
    this_edge->push_back(std::make_pair(x - gridX, y - gridY));
    coastline_polyline.push_back(std::make_pair(x, y));
  }

  if (initial_cell == std::make_pair(-1, -1)) {
    initial_cell = {gridX, gridY};
    this_edge->push_back(std::make_pair(x - gridX, y - gridY));
    coastline_polyline.push_back(std::make_pair(x, y));
    // std::cout << "Initial edge " << x << " " << y << std::endl;
  }

  double remainingDistance = distance;

  while (true) {
    const double left = static_cast<double>(gridX);
    const double right = static_cast<double>(gridX + 1);
    const double bottom = static_cast<double>(gridY);
    const double top = static_cast<double>(gridY + 1);

    if (x < left || x > right || y < bottom || y > top) {
      std::cout << "Error: point outside grid" << std::endl;
      std::cout << "x: " << x << " left: " << left << " " << (x >= left)
                << " right: " << right << " " << (x <= right) << std::endl;
      std::cout << "y: " << y << " bottom: " << bottom << " " << (y >= bottom)
                << " top: " << top << " " << (y <= top) << std::endl;
    }
    DEV_ASSERT(x >= left || approximately_equal(x, left));
    DEV_ASSERT(x <= right || approximately_equal(x, right));
    DEV_ASSERT(y >= bottom || approximately_equal(y, bottom));
    DEV_ASSERT(y <= top || approximately_equal(y, top));

    double len = std::numeric_limits<double>::max();
    std::pair<int, int> change = {0, 0};

    if (dirX < 0) {
      const double lenX = -(x - left) / dirX;
      if (lenX < len) {
        len = lenX;
        change = {-1, 0};
      }
    } else if (dirX > 0) {
      const double lenX = (right - x) / dirX;
      if (lenX < len) {
        len = lenX;
        change = {1, 0};
      }
    }
    if (dirY < 0) {
      const double lenY = -(y - bottom) / dirY;
      if (lenY < len) {
        len = lenY;
        change = {0, -1};
      }
    } else if (dirY > 0) {
      const double lenY = (top - y) / dirY;
      if (lenY < len) {
        len = lenY;
        change = {0, 1};
      }
    }

    DEV_ASSERT(len >= 0.0);

    len = std::min(len, remainingDistance);

    x = x + dirX * len;
    y = y + dirY * len;

    if (this_edge) this_edge->push_back(std::make_pair(x - gridX, y - gridY));
    // std::cout << "Point " << x << " " << y << std::endl;
    coastline_polyline.push_back(std::make_pair(x, y));

    grid[gridY][gridX] = CoastTile::Coast;

    remainingDistance -= len;

    if (remainingDistance <= 1e-6) {
      break;
    }

    gridX += change.first;
    gridY += change.second;

    if (!cell_edges[{gridX, gridY}])
      cell_edges[{gridX, gridY}] = std::make_unique<CellEdges>();

    cell_edges[{gridX, gridY}]->edges.push_back(
        std::vector<std::pair<double, double>>());
    this_edge = &cell_edges[{gridX, gridY}]->edges.back();
    this_edge->push_back(std::make_pair(x - gridX, y - gridY));
    // std::cout << "Point " << x << " " << y << std::endl;
  }

  return !break_coastline;
}

void bfs_try_add(std::vector<std::vector<CoastTile>>& grid,
                 std::queue<std::pair<int, int>>& q, int x, int y,
                 CoastTile value) {
  if (x < 0 || x >= static_cast<int>(grid[0].size()) || y < 0 ||
      y >= static_cast<int>(grid.size()) ||
      grid[y][x] != CoastTile::NotDetermined)
    return;
  grid[y][x] = value;
  q.push({x, y});
}

void bfs(std::vector<std::vector<CoastTile>>& grid, int x, int y,
         CoastTile value) {
  std::queue<std::pair<int, int>> q;
  q.push({x, y});
  grid[y][x] = value;
  while (!q.empty()) {
    auto [x, y] = q.front();
    q.pop();

    bfs_try_add(grid, q, x - 1, y, value);
    bfs_try_add(grid, q, x + 1, y, value);
    bfs_try_add(grid, q, x, y - 1, value);
    bfs_try_add(grid, q, x, y + 1, value);
  }
}