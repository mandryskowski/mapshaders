#include "HeightMap.h"
#include "../../common/3D/RenderUtil3D.h"
#include "../../../../src/import/osm_parser/OSMHeightmap.h"
#include "../../../src/util/PolyUtil.h"
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <queue>

using namespace godot;

struct CellEdges {
    std::vector<std::vector<std::pair<double, double>>> edges;
    std::vector<std::vector<std::pair<double, double>>> fully_contained_edges;
};

template <typename T, typename U>
struct std::hash<std::pair<T, U>> {
public:
        size_t operator()(std::pair<int, int> x) const throw() { return hash<T>()(x.first) ^ hash<U>()(x.second); }
};

void generate_full_rectangle(const GeoCoords cell_pos, double cellrad, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap);
void generate_partial_rectangle(const std::pair<int, int>& col_row, const GeoCoords cell_pos, double cellrad, PackedVector2Array& gridrectpoly, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap, std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>&);
bool rasterizeLine(std::vector<std::vector<int>>& grid, double x1, double y1, double x2, double y2, std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>& cell_edges, std::vector<std::pair<double, double>>*& this_edge, std::pair<int, int>& initial_cell, std::vector<std::pair<double, double>>&);
void bfs(std::vector<std::vector<int>>& grid, int x, int y, int value);

void HeightMap::import_polygons_geo(godot::Array polygons, GeoMap * geomap) {
    std::cout << "Imported coastline polygons\n";
    coastline_polygons = polygons;
    coastline_geomap = geomap;
}

int pnpoly(int nvert, std::vector<std::pair<double, double>>& verts, double testx, double testy)
{
  int i, j, c = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ( ((verts[i].second>testy) != (verts[j].second>testy)) &&
     (testx < (verts[j].first-verts[i].first) * (testy-verts[i].second) / (verts[j].second-verts[i].second) + verts[i].first) )
       c = !c;
  }
  return c;
}

GeoCoords grid_to_geo(ElevationGrid& grid, const std::pair<double, double>& gridPos) {
    auto cellrad = grid.getCellsize() / 180.0 * Math_PI;
    return grid.getTopLeftGeo() + GeoCoords(Longitude::radians(cellrad * gridPos.first), Latitude::radians(-cellrad * gridPos.second));
}

void generate_coastline(std::vector<std::pair<double, double>>& poly, Array& arrays, GeoMap& coastline_geomap, ElevationGrid* grid, bool connect_last_to_first) {
    std::cout << "Generating coastline " << poly.size() << " " << (connect_last_to_first ? "true" : "false") << std::endl;
    PackedVector3Array poly3d, normals;
    Ref<ElevationHeightmap> hmap = memnew(ElevationHeightmap);
    hmap->set_elevation_grid(grid);

    for (int k = 0; k < static_cast<int>(poly.size()) - (connect_last_to_first ? 0 : 1); k++) {

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


void HeightMap::import_grid(ElevationGrid *grid) {
    auto children = get_children();
    for (int j = 0; j < children.size(); j++) {
        remove_child(Object::cast_to<Node>(children[j]));
    }
    auto cell_pos = grid->getTopLeftGeo();
    auto left = cell_pos.lon;
    std::vector<std::vector<int>> land_cells(grid->getNrows(), std::vector<int>(grid->getNcols(), 0));
    std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>> coastline_cell_edges;
    Array arrays = RenderUtil3D::get_array_mesh_arrays({Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL});

    // Grid rectangle
    auto cellrad = grid->getCellsize() / 180.0 * Math_PI;
    PackedVector2Array gridrectpoly;
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation());
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation() + cellrad * Vector2(grid->getNcols(), 0));
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation() + cellrad * Vector2(grid->getNcols(), -grid->getNrows()));
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation() + cellrad * Vector2(0, -grid->getNrows()));
    // Create the grid poly
    PackedVector2Array xd;

    auto wskaznik = Object::cast_to<Node3D>(RenderUtil3D::achild(this, memnew(Node3D), "Wskaznik"));
    auto wskaznik2 = Object::cast_to<Node3D>(RenderUtil3D::achild(this, memnew(Node3D), "Wskaznik2"));
    auto wskaznik3 = Object::cast_to<Node3D>(RenderUtil3D::achild(this, memnew(Node3D), "Wskaznik3"));

    std::cout << "Coastline polygons:\n";
    for (int i = 0; i < coastline_polygons.size(); i++) {
        Array poly = static_cast<Array>(coastline_polygons[i]);
        Array poly_segment = poly[0];
        PackedFloat64Array poly_segment_x = poly_segment[0];
        if (i == 1) {
            PackedFloat64Array poly_segment_y = poly_segment[1];
            wskaznik->set_position(coastline_geomap->geo_to_world(GeoCoords(Longitude::radians(poly_segment_x[0]), Latitude::radians(poly_segment_y[0]))));
            wskaznik2->set_position(coastline_geomap->geo_to_world(GeoCoords(Longitude::radians(poly_segment_x[1]), Latitude::radians(poly_segment_y[1]))));
            wskaznik3->set_position(coastline_geomap->geo_to_world(GeoCoords(Longitude::radians(poly_segment_x[2]), Latitude::radians(poly_segment_y[2]))));
        }
        std::cout << poly_segment_x.size() << std::endl;
    }
    
    for (int i = 0; i < coastline_polygons.size(); i++) {
        Array poly = static_cast<Array>(coastline_polygons[i]);
        for (int j = 0; j < poly.size(); j++) {
            Array part = static_cast<Array>(poly[j]);
            PackedFloat64Array part_x = part[0], part_y = part[1];

            DEV_ASSERT(part_x.size() == part_y.size());
            DEV_ASSERT(!part_x.is_empty());
            std::vector<std::pair<double, double>>* this_edge = nullptr;

            // Initial edge will be incomplete, so we need to store it until we reach the last edge.
            std::vector<std::pair<double, double>> initial_edge;
            std::pair<int, int> initial_cell = {-1, -1};

            // Coastline polyline
            std::vector<std::pair<double, double>> coastline_polyline;
            bool broken_coastline = false;
            this_edge = &initial_edge;
            for (int k = 0; k < part_x.size(); k++) {
                std::pair<double, double> v1(part_x[k], part_y[k]);
                std::pair<double, double> v2(part_x[(k + 1) % part_x.size()], part_y[(k + 1) % part_y.size()]);

                double v1_tile_x = (v1.first - grid->getTopLeftGeo().lon.get_radians()) / cellrad;
                double v1_tile_y = -(v1.second - grid->getTopLeftGeo().lat.get_radians()) / cellrad;
                double v2_tile_x = (v2.first - grid->getTopLeftGeo().lon.get_radians()) / cellrad;
                double v2_tile_y = -(v2.second - grid->getTopLeftGeo().lat.get_radians()) / cellrad;

                if (!rasterizeLine(land_cells, v1_tile_x, v1_tile_y, v2_tile_x, v2_tile_y, coastline_cell_edges, this_edge, initial_cell, coastline_polyline) && !coastline_polyline.empty() && cliffs) {
                    generate_coastline(coastline_polyline, arrays, *coastline_geomap, grid, false);
                    broken_coastline = true;
                    coastline_polyline.clear();
                }
            }

            if (this_edge == &initial_edge) { // Fully contained edges
                if (!coastline_cell_edges[initial_cell]) {
                    coastline_cell_edges[initial_cell] = std::make_unique<CellEdges>();
                }

                coastline_cell_edges[initial_cell]->fully_contained_edges.push_back(initial_edge);
            } else if (coastline_cell_edges[initial_cell] && this_edge == &coastline_cell_edges[initial_cell]->edges.back()) { // Assuming the coastline has made a full loop
                DEV_ASSERT(initial_edge[0] == this_edge->back());
                for (int k = 1; k < initial_edge.size(); k++) { // skip the first point, as it's the same as the last one
                    this_edge->push_back(initial_edge[k]);
                }
            }

            if (cliffs)
                generate_coastline(coastline_polyline, arrays, *coastline_geomap, grid, !broken_coastline);

            
       }
    }

    UtilityFunctions::print("Total rows ", grid->getNrows());

    for (int row = 0; row < grid->getNrows() - 1; row++) {
        if (row % 20 == 0)
            UtilityFunctions::print("Row ", row, " out of ", grid->getNrows());
        
        for (int col = 0; col < grid->getNcols() - 1; col++) {
            if (land_cells[row][col] == 0) {
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
                        GeoCoords test_point = cell_pos + GeoCoords(Longitude::radians(cellrad / 2.0), Latitude::radians(-cellrad / 2.0));
                        if (pnpoly(part_poly.size(), part_poly, test_point.lon.get_radians(), test_point.lat.get_radians())) {
                            sea_cell = j > 0; // If it's not the first part, it's a sea cell
                            found = true;
                        }
                        //if (Geometry2D::get_singleton()->is_point_in_polygon((cell_pos + GeoCoords(Longitude::radians(cellrad / 2.0), Latitude::radians(-cellrad / 2.0))).to_vector2_representation() * 10000.0, part_poly)) {
                        //     sea_cell = j > 0; // If it's not the first part, it's a sea cell
                        //     found = true;
                        // }
                    }

                    if (found)
                        break;
                    
                }

                bfs(land_cells, col, row, sea_cell ? -1 : 2);
            }
            if (land_cells[row][col] == 1)
                generate_partial_rectangle({col, row}, cell_pos, cellrad, gridrectpoly, *grid, flat_shading, arrays, coastline_geomap, coastline_cell_edges);
            if (land_cells[row][col] == 2) {
                generate_full_rectangle(cell_pos, cellrad, *grid, flat_shading, arrays, coastline_geomap);
            }

            // Advance on X
            cell_pos.lon = cell_pos.lon + Longitude::radians(cellrad);
        }

        // Advance on Y, reset X
        cell_pos.lon = left;
        cell_pos.lat = cell_pos.lat - Latitude::radians(cellrad);
    }

    RenderUtil3D::area_poly(this, "GroundMesh", arrays, Color(0.745098, 0.745098, 0.745098));
    coastline_polygons = Array();
}

void HeightMap::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import_polygons_geo", "polygons", "geomap"), &HeightMap::import_polygons_geo);
    ClassDB::bind_method(D_METHOD("import_grid", "grid"), &HeightMap::import_grid);
    ClassDB::bind_method(D_METHOD("set_flat_shading", "flat"), &HeightMap::set_flat_shading);
    ClassDB::bind_method(D_METHOD("get_flat_shading"), &HeightMap::get_flat_shading);

    ClassDB::bind_method(D_METHOD("set_cliffs", "cliffs"), &HeightMap::set_cliffs);
    ClassDB::bind_method(D_METHOD("get_cliffs"), &HeightMap::get_cliffs);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flat_shading"), "set_flat_shading", "get_flat_shading");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cliffs"), "set_cliffs", "get_cliffs");
}

Vector3 get_normal_from_heights(double L, double R, double B, double T) {
    return Vector3((L - R) / 2.0, 1, (T - B) / 2.0).normalized();
}
 
Vector3 get_normal_from_pos(ElevationHeightmap* hmap, const GeoCoords& pos, double delta) {
    delta *= hmap->get_elevation_grid()->getCellsize();
    return get_normal_from_heights(hmap->getElevation(pos + GeoCoords(Longitude::degrees(-delta), Latitude::degrees(0))),
                      hmap->getElevation(pos + GeoCoords(Longitude::degrees(delta), Latitude::degrees(0))),
                      hmap->getElevation(pos + GeoCoords(Longitude::degrees(0), Latitude::degrees(-delta))),
                      hmap->getElevation(pos + GeoCoords(Longitude::degrees(0), Latitude::degrees(delta))));
}

void generate_full_rectangle(const GeoCoords cell_pos, double cellrad, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap) {
    std::vector<GeoCoords> poly;
    poly.push_back(cell_pos);
    poly.push_back(cell_pos + GeoCoords(Longitude::radians(cellrad), Latitude::radians(0)));
    poly.push_back(cell_pos + GeoCoords(Longitude::radians(cellrad), Latitude::radians(-cellrad)));
    poly.push_back(cell_pos + GeoCoords(Longitude::radians(0), Latitude::radians(-cellrad)));

    PackedVector3Array poly3d;
    PackedVector3Array normals;
    Ref<ElevationHeightmap> hmap = memnew(ElevationHeightmap);
    hmap->set_elevation_grid(&grid);

    PackedInt32Array triangulation_indices = {0, 1, 2, 0, 2, 3};
    for (int i = 0; i < triangulation_indices.size(); i++) {
        auto v_geo = poly[triangulation_indices[i]];
        auto v_world = coastline_geomap->geo_to_world(v_geo);
        poly3d.push_back(v_world + Vector3(0, hmap->getElevation(v_geo), 0)); 
        if (flat) {
            normals.push_back(Vector3(0, 1, 0));
        } else {
            normals.append(get_normal_from_pos(hmap.ptr(), v_geo, 0.1));
        }
    }

    if (RenderUtil3D::enforce_winding(poly3d, true))
        normals.reverse();
    arrays[Mesh::ARRAY_VERTEX].call("append_array", poly3d);
    arrays[Mesh::ARRAY_NORMAL].call("append_array", normals);
}

struct CellCoastlineTip {
    std::reference_wrapper<std::vector<std::pair<double, double>>> edges;
    int index;
    bool reverse;
};

void grid_space_polygon(ElevationGrid& grid, const std::vector<std::pair<double, double>>& poly, double cellrad, bool flat, Array& arrays, GeoMap* coastline_geomap, const GeoCoords& cell_pos) {
    PackedVector3Array poly3d;
    PackedVector3Array normals;

    Ref<ElevationHeightmap> hmap = memnew(ElevationHeightmap);

    hmap->set_elevation_grid(&grid);

    Ref<PolyUtil> pu = memnew(PolyUtil);
    auto triangulated = pu->triangulate_with_holes(poly, {});
    
    for (auto& point : triangulated) {

        auto v_geo = cell_pos + GeoCoords(Longitude::radians(point.first * cellrad), Latitude::radians(-point.second * cellrad));
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

void generate_partial_rectangle(const std::pair<int, int>& col_row, const GeoCoords cell_pos, double cellrad, PackedVector2Array& gridrectpoly, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap, std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>& cell_edges) {
    if (!cell_edges[col_row]) {
        std::cout << "Invalid cell " << col_row.first << " " << col_row.second << std::endl;
        return;
    }

    std::vector<std::vector<std::pair<double, double>>>& all_edges = cell_edges[col_row]->edges;
    DEV_ASSERT(!(all_edges.empty() && cell_edges[col_row]->fully_contained_edges.empty()));

    std::vector<CellCoastlineTip> edges_left;
    std::vector<CellCoastlineTip> edges_right;
    std::vector<CellCoastlineTip> edges_top;
    std::vector<CellCoastlineTip> edges_bottom;

    for (int i = 0; i < all_edges.size(); i++) {
        auto& edge = all_edges[i];
        std::vector<std::pair<int, bool>> check = {{0, false}, {edge.size() - 1, true}};
        for (auto [check_index, reverse] : check) {
            CellCoastlineTip tip = {std::ref(edge), i, reverse};
            auto& point = edge[check_index];
            if (point.first == 0.0) {
                edges_left.push_back(std::move(tip));
            } else if (point.first == 1.0) {
                edges_right.push_back(std::move(tip));
            } else if (point.second == 0.0) {
                edges_bottom.push_back(std::move(tip));
            } else if (point.second == 1.0) {
                edges_top.push_back(std::move(tip));
            }
            else {
                std::cout << "Unreachable " << point.first << " " << point.second << std::endl;
                return;
                //DEV_ASSERT(false); // unreachable
            }
        }
    }

    // Sort edges
    std::sort(edges_left.begin(), edges_left.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].second > b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].second;
    });
    std::sort(edges_right.begin(), edges_right.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].second < b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].second;
    });
    std::sort(edges_top.begin(), edges_top.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].first > b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].first;
    });
    std::sort(edges_bottom.begin(), edges_bottom.end(), [](const auto& a, const auto& b) {
        return a.edges.get()[a.reverse ? a.edges.get().size() - 1 : 0].first < b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].first;
    });
    
    std::vector<bool> visited(all_edges.size(), false);

    for (int i = 0; i < all_edges.size(); i++) {
        if (visited[i]) continue;

        auto& this_edge = all_edges[i];
        visited[i] = true;

        std::vector<std::pair<double, double>> poly = this_edge;

        std::pair<double, double> starting_point = poly[0];

        while (true) {
            CellCoastlineTip* next_tip = nullptr;
            
            if (poly.back().first == 0.0 && poly.back().second != 0.0) {
                // Binary search to find the index before the current point

                auto next_point = std::upper_bound(edges_left.begin(), edges_left.end(), poly.back(), [](const auto& a, const auto& b) {
                    return a.second > b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].second;
                });

                if (next_point == edges_left.end()) {
                    poly.push_back({0.0, 0.0});
                    continue;
                }

                next_tip = &*(next_point);
                
            }
            else if (poly.back().second == 1.0 && poly.back().first != 0.0) {
                // Binary search to find the index before the current point

                auto next_point = std::upper_bound(edges_top.begin(), edges_top.end(), poly.back(), [](const auto& a, const auto& b) {
                    return a.first > b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].first;
                });

                if (next_point == edges_top.end()) {
                    poly.push_back({0.0, 1.0});
                    continue;
                }

                next_tip = &*(next_point);
            }
            else if (poly.back().first == 1.0 && poly.back().second != 1.0) {
                // Binary search to find the index before the current point

                auto next_point = std::upper_bound(edges_right.begin(), edges_right.end(), poly.back(), [](const auto& a, const auto& b) {
                    return a.second < b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].second;
                });

                if (next_point == edges_right.end()) {
                    poly.push_back({1.0, 1.0});
                    continue;
                }

                next_tip = &*(next_point);
            }
            else if (poly.back().second == 0.0 && poly.back().first != 1.0) {
                // Binary search to find the index before the current point

                auto next_point = std::upper_bound(edges_bottom.begin(), edges_bottom.end(), poly.back(), [](const auto& a, const auto& b) {
                    return a.first < b.edges.get()[b.reverse ? b.edges.get().size() - 1 : 0].first;
                });

                if (next_point == edges_bottom.end()) {
                    poly.push_back({1.0, 0.0});
                    continue;
                }

                next_tip = &*(next_point);
            }

            DEV_ASSERT(next_tip != nullptr);
            if (!next_tip->reverse && next_tip->edges.get()[0] == starting_point || next_tip->reverse && next_tip->edges.get().back() == starting_point) {
                poly.push_back(starting_point);
                break;
            }
            visited[next_tip->index] = true;
            if (!next_tip->reverse) {
                for (auto& point : next_tip->edges.get()) {
                    poly.push_back(point);
                }
            }
            else {
                for (int i = next_tip->edges.get().size() - 1; i >= 0; i--) {
                    poly.push_back(next_tip->edges.get()[i]);
                }
            }
        }

        grid_space_polygon(grid, poly, cellrad, flat, arrays, coastline_geomap, cell_pos);
    }

    for (auto& poly : cell_edges[col_row]->fully_contained_edges) {
        grid_space_polygon(grid, poly, cellrad, flat, arrays, coastline_geomap, cell_pos);
    }
}

const int LEFT = 1, RIGHT = 2, BOTTOM = 4, TOP = 8;

// Determine the region code for a point
int computeOutCode(double x, double y, int grid_x, int grid_y) {
    int code = 0;
    if (x < 0)       code |= LEFT;
    else if (x >= grid_x) code |= RIGHT;
    if (y < 0)       code |= BOTTOM;
    else if (y >= grid_y) code |= TOP;
    return code;
}

// Cohen-Sutherland clipping algorithm
bool clipLine(double& x1, double& y1, double& x2, double& y2, int grid_x, int grid_y) {
    int outcode1 = computeOutCode(x1, y1, grid_x, grid_y);
    int outcode2 = computeOutCode(x2, y2, grid_x, grid_y);

    bool accept = false;

    while (true) {
        if (!(outcode1 | outcode2)) {  // Both points inside grid
            accept = true;
            break;
        } else if (outcode1 & outcode2) {  // Both points outside grid, in the same region
            break;
        } else {
            // Calculate intersection with the grid boundary
            double x, y;
            int outcodeOut = outcode1 ? outcode1 : outcode2;

            if (outcodeOut & TOP) {           // Point is above grid
                x = x1 + (x2 - x1) * (grid_x - 1 - y1) / (y2 - y1);
                y = grid_y - 1;
            } else if (outcodeOut & BOTTOM) {  // Point is below grid
                x = x1 + (x2 - x1) * (0 - y1) / (y2 - y1);
                y = 0;
            } else if (outcodeOut & RIGHT) {   // Point is to the right of grid
                y = y1 + (y2 - y1) * (grid_y - 1 - x1) / (x2 - x1);
                x = grid_x - 1;
            } else if (outcodeOut & LEFT) {    // Point is to the left of grid
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
 * @brief Rasterize a coastline line by stepping one cell at a time. This function has multiple responsibilities, because stepping one cell at a time allows us to kill many birds with one stone:
 *  - Set the grid cells the line passes through to 1. This way we can only perform the expensive partial rectangle generation on these cells, saving us precious time.
 *  - For each cell the line passes through, store the line in the cell_edges map, clipped to the cell. This way we can efficiently generate partial rectangles later.
 *  - For each step, add the step position to the coastline_polyline vector. This way the polyline is grid-aligned and can be used to generate cliffs that perfectly align with the heightmap.
 * @param grid The land cells grid (full sea/partial/full land)
 * @param x1 The x-coordinate of the start of the line, in grid space
 * @param y1 The y-coordinate of the start of the line, in grid space
 * @param x2 The x-coordinate of the end of the line, in grid space
 * @param y2 The y-coordinate of the end of the line, in grid space
 * @param cell_edges A map of cell coordinates to a vector of edges that pass through the cell
 * @param this_edge A pointer to the current edge vector in the cell_edges map. Moving to another cell pushes a new vector to that cell and sets this_edge to point to it.
 * @param initial_cell The cell the line started in. Set it to {-1, -1} initially. You can use it to determine if the line has made a full loop.
 * @param coastline_polyline The vector to store the grid-aligned polyline in.
 * @returns True if the line is completely outside the grid, false otherwise.
 * 
 */
bool rasterizeLine(std::vector<std::vector<int>>& grid, double x1, double y1, double x2, double y2, std::unordered_map<std::pair<int, int>, std::unique_ptr<CellEdges>>& cell_edges, std::vector<std::pair<double, double>>*& this_edge, std::pair<int, int>& initial_cell, std::vector<std::pair<double, double>>& coastline_polyline) {
    if (!clipLine(x1, y1, x2, y2, static_cast<int>(grid[0].size()), static_cast<int>(grid.size()))) return false; // Clip line to grid; exit if completely outside

    bool print = false;
    double dx = x2 - x1;
    double dy = y2 - y1;
    const double distance = std::sqrt(dx * dx + dy * dy);

    if (distance == 0.0) {
        return true;
    }

    double dirX = dx / distance, dirY = dy / distance;

    // Use more steps for shorter segments
    int steps = static_cast<int>(std::max(distance * 8, 1.0));  // Oversample by factor of 2

    double xIncrement = dx / steps;
    double yIncrement = dy / steps;

    double x = x1;
    double y = y1;
    int gridX = static_cast<int>(x), gridY = static_cast<int>(y);
    if (initial_cell == std::make_pair(-1, -1)) {
        initial_cell = {gridX, gridY};
        this_edge->push_back(std::make_pair(x - gridX, y - gridY));
    }
 
    double remainingDistance = distance;

    while (true) {
        const double left = static_cast<double>(gridX);
        const double right = static_cast<double>(gridX + 1);
        const double bottom = static_cast<double>(gridY);
        const double top = static_cast<double>(gridY + 1);

        DEV_ASSERT(x >= left);
        DEV_ASSERT(x <= right);
        DEV_ASSERT(y >= bottom);
        DEV_ASSERT(y <= top);

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

        if (this_edge)
            this_edge->push_back(std::make_pair(x - gridX, y - gridY));
        coastline_polyline.push_back(std::make_pair(x, y));

        grid[gridY][gridX] = 1;

        remainingDistance -= len;

        if (remainingDistance <= 1e-6) {
            break;
        }

        gridX += change.first;
        gridY += change.second;

        if (!cell_edges[{gridX, gridY}])
            cell_edges[{gridX, gridY}] = std::make_unique<CellEdges>();

        cell_edges[{gridX, gridY}]->edges.push_back(std::vector<std::pair<double, double>>());
        this_edge = &cell_edges[{gridX, gridY}]->edges.back();
        this_edge->push_back(std::make_pair(x - gridX, y - gridY));
    }

    if (print)
    std::cout << std::endl;
    return true;
}

void bfs_try_add(std::vector<std::vector<int>>& grid, std::queue<std::pair<int, int>>& q, int x, int y, int value) {
    if (x < 0 || x >= grid[0].size() || y < 0 || y >= grid.size() || grid[y][x] != 0)
        return;
    grid[y][x] = value;
    q.push({x, y});
}

void bfs(std::vector<std::vector<int>>& grid, int x, int y, int value) {
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