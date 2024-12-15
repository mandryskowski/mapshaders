#include "HeightMap.h"
#include "../../common/3D/RenderUtil3D.h"
#include "../../../../src/import/osm_parser/OSMHeightmap.h"
#include <godot_cpp/classes/geometry2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <queue>

using namespace godot;

void generate_full_rectangle(const GeoCoords cell_pos, double cellrad, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap);
void generate_partial_rectangle(const GeoCoords cell_pos, double cellrad, PackedVector2Array& gridrectpoly, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap);
void rasterizeLine(std::vector<std::vector<int>>& grid, double x1, double y1, double x2, double y2);
void bfs(std::vector<std::vector<int>>& grid, int x, int y, int value);


void HeightMap::import_polygons_geo(godot::Array polygons, GeoMap * geomap) {
    std::cout << "Imported coastline polygons\n";
    coastline_polygons = polygons;
    coastline_geomap = geomap;
}

void HeightMap::import_grid(ElevationGrid *grid) {
    std::cout << "Called import grid\n";
    auto children = get_children();
    for (int j = 0; j < children.size(); j++) {
        remove_child(Object::cast_to<Node>(children[j]));
    }
    auto cell_pos = grid->getTopLeftGeo();
    auto left = cell_pos.lon;
    auto heights = grid->getHeightmap();
    std::vector<std::vector<int>> land_cells(grid->getNrows(), std::vector<int>(grid->getNcols(), 0));

    // Grid rectangle
    auto cellrad = grid->getCellsize() / 180.0 * Math_PI;
    PackedVector2Array gridrectpoly;
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation());
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation() + cellrad * Vector2(grid->getNcols(), 0));
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation() + cellrad * Vector2(grid->getNcols(), -grid->getNrows()));
    gridrectpoly.append(grid->getTopLeftGeo().to_vector2_representation() + cellrad * Vector2(0, -grid->getNrows()));
    // Create the grid poly
    PackedVector2Array xd;

    std::cout << "Coastline polygons:\n";
    for (int i = 0; i < coastline_polygons.size(); i++) {
        Array poly = static_cast<Array>(coastline_polygons[i]);
        Array poly_segment = poly[0];
        PackedFloat64Array poly_segment_x = poly_segment[0];
        std::cout << poly_segment_x.size() << std::endl;
    }
    
    for (int i = 0; i < coastline_polygons.size(); i++) {
        Array poly = static_cast<Array>(coastline_polygons[i]);
        for (int j = 0; j < poly.size(); j++) {
            Array part = static_cast<Array>(poly[j]);
            PackedFloat64Array part_x = part[0], part_y = part[1];
            for (int k = 0; k < part_x.size(); k++) {
                Vector2 v1(part_x[k], part_y[k]);
                Vector2 v2(part_x[(k + 1) % part_x.size()], part_y[(k + 1) % part_y.size()]);

                double v1_tile_x = (v1.x - grid->getTopLeftGeo().lon.get_radians()) / cellrad;
                double v1_tile_y = -(v1.y - grid->getTopLeftGeo().lat.get_radians()) / cellrad;
                double v2_tile_x = (v2.x - grid->getTopLeftGeo().lon.get_radians()) / cellrad;
                double v2_tile_y = -(v2.y - grid->getTopLeftGeo().lat.get_radians()) / cellrad;

                rasterizeLine(land_cells, v1_tile_x, v1_tile_y, v2_tile_x, v2_tile_y);
            }
        }
    }

    /*for (int y = 670; y < 680; y++) {
        for (int x = 892; x < 922; x++) {
            if (land_cells[y][x] == -1)
                std::cout << ' ';
            else
                std::cout << land_cells[y][x];
        }
        std::cout << std::endl;
    }*/


    Array chosen_poly_parts = static_cast<Array>(coastline_polygons[0]);
    Array chosen_poly_part = static_cast<Array>(chosen_poly_parts[0]);
    PackedFloat64Array part_x = chosen_poly_part[0], part_y = chosen_poly_part[1];
    for (int i = 0; i < part_x.size(); i++)
        xd.append(Vector2(part_x[i] * 100.0, part_y[i] * 100.0));
    for (int i = 0; i < gridrectpoly.size(); i++)
        gridrectpoly[i] *= 100.0;

    std::cout << "Xd size is " << xd.size() << std::endl;
    
    gridrectpoly = Geometry2D::get_singleton()->intersect_polygons(gridrectpoly, xd)[0];
    for (int i = 0; i < xd.size(); i++)
        xd[i] /= 100.0;
    for (int i = 0; i < gridrectpoly.size(); i++)
        gridrectpoly[i] /= 100.0;


    PackedVector3Array gridrectpoly3d;
    for (int i = 0; i < gridrectpoly.size(); i++) {
        gridrectpoly3d.append(coastline_geomap->geo_to_world(GeoCoords::from_vector2_representation(gridrectpoly[i])));
    }

    RenderUtil3D::area_poly(this, "zupa", RenderUtil3D::polygon(gridrectpoly3d, 0));

    UtilityFunctions::print("Total rows ", grid->getNrows());
    
    Array arrays = RenderUtil3D::get_array_mesh_arrays({Mesh::ARRAY_VERTEX, Mesh::ARRAY_NORMAL});

    for (int row = 0; row < grid->getNrows() - 1; row++) {
        if (row % 20 == 0)
            UtilityFunctions::print("Row ", row);
        
        for (int col = 0; col < grid->getNcols() - 1; col++) {
            if (land_cells[row][col] == 0) {
                bool sea_cell = true;
                bool found = false;
                for (int i = 0; i < coastline_polygons.size(); i++) {
                    Array parts = static_cast<Array>(coastline_polygons[i]);
                    for (int j = 0; j < parts.size(); j++) {
                        Array part = static_cast<Array>(parts[j]);
                        PackedFloat64Array part_x = part[0], part_y = part[1];

                        PackedVector2Array part_poly;
                        for (int k = 0; k < part_x.size(); k++) {
                            part_poly.append(Vector2(part_x[k] * 1000.0, part_y[k] * 1000.0));
                        }
                        if (Geometry2D::get_singleton()->is_point_in_polygon(cell_pos.to_vector2_representation() * 1000.0, part_poly)) {
                            sea_cell = j > 0; // If it's not the first part, it's a sea cell
                            found = true;
                        }
                    }

                    if (found)
                        break;
                    
                }

                bfs(land_cells, col, row, sea_cell ? -1 : 2);
            }
            if (land_cells[row][col] == 1)
                generate_partial_rectangle(cell_pos, cellrad, gridrectpoly, *grid, flat_shading, arrays, coastline_geomap);
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

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flat_shading"), "set_flat_shading", "get_flat_shading");
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

void generate_partial_rectangle(const GeoCoords cell_pos, double cellrad, PackedVector2Array& gridrectpoly, ElevationGrid& grid, bool flat, Array& arrays, GeoMap* coastline_geomap) {
    PackedVector2Array poly;
    poly.push_back(cell_pos.to_vector2_representation());
    poly.push_back((cell_pos + GeoCoords(Longitude::radians(cellrad), Latitude::radians(0))).to_vector2_representation());
    poly.push_back((cell_pos + GeoCoords(Longitude::radians(cellrad), Latitude::radians(-cellrad))).to_vector2_representation());
    poly.push_back((cell_pos + GeoCoords(Longitude::radians(0), Latitude::radians(-cellrad))).to_vector2_representation());

    for (int i = 0; i < poly.size(); i++)
        poly[i] *= 100.0;
    for (int i = 0; i < gridrectpoly.size(); i++)
        gridrectpoly[i] *= 100.0;

    Array out_polys = Geometry2D::get_singleton()->intersect_polygons(poly, gridrectpoly);

    for (int i = 0; i < poly.size(); i++)
        poly[i] /= 100.0;
    for (int i = 0; i < gridrectpoly.size(); i++)
        gridrectpoly[i] /= 100.0;

    if (out_polys.size() == 0) {
        return;
    }
    for (int part = 0; part < out_polys.size(); part++) {
        PackedVector2Array out = out_polys[part];

        
        for (int i = 0; i < out.size(); i++)
            out[i] /= 100.0;

        PackedVector3Array poly3d;
        PackedVector3Array normals;
        Ref<ElevationHeightmap> hmap = memnew(ElevationHeightmap);
        hmap->set_elevation_grid(&grid);

        PackedInt32Array triangulation_indices = Geometry2D::get_singleton()->triangulate_polygon(out);
        for (int i = 0; i < triangulation_indices.size(); i++) {
            auto v_geo = GeoCoords::from_vector2_representation(out[triangulation_indices[i]]);
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

        // Coastline cliffs
        for (int i = 0; i < out.size(); i++) {
            auto v1_geo = GeoCoords::from_vector2_representation(out[i]);
            auto v2_geo = GeoCoords::from_vector2_representation(out[(i + 1) % out.size()]);

            auto v1_world = coastline_geomap->geo_to_world(v1_geo);
            auto v2_world = coastline_geomap->geo_to_world(v2_geo);

            // Rectangle that goes to 0
            poly3d.push_back(v1_world + Vector3(0, hmap->getElevation(v1_geo), 0));
            poly3d.push_back(v2_world + Vector3(0, hmap->getElevation(v2_geo), 0));
            poly3d.push_back(v2_world);

            poly3d.push_back(v2_world);
            poly3d.push_back(v1_world);
            poly3d.push_back(v1_world + Vector3(0, hmap->getElevation(v1_geo), 0));

            for (int j = 0; j < 6; j++) {
                auto v1 = poly3d[poly3d.size() - 6 + j];
                auto v2 = poly3d[poly3d.size() - 6 + (j + 1) % 6];

                normals.push_back((v2 - v1).cross(Vector3(0, 1, 0)).normalized());
            }
        }


        arrays[Mesh::ARRAY_VERTEX].call("append_array", poly3d);
        arrays[Mesh::ARRAY_NORMAL].call("append_array", normals);
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

// Rasterize line on a clipped grid using DDA
void rasterizeLine(std::vector<std::vector<int>>& grid, double x1, double y1, double x2, double y2) {
    if (!clipLine(x1, y1, x2, y2, static_cast<int>(grid[0].size()), static_cast<int>(grid.size()))) return; // Clip line to grid; exit if completely outside

    bool print = false;
    if (x1 > 892.0 && x1 < 922.0 && y1 > 670.0 && y1 < 680.0) {
        std::cout << "Line from " << x1 << ", " << y1 << " to " << x2 << ", " << y2 << std::endl;
        print = true;
    }

    double dx = x2 - x1;
    double dy = y2 - y1;
    double distance = std::sqrt(dx * dx + dy * dy);

    // Use more steps for shorter segments
    int steps = static_cast<int>(std::max(distance * 8, 1.0));  // Oversample by factor of 2

    double xIncrement = dx / steps;
    double yIncrement = dy / steps;

    double x = x1;
    double y = y1;

    for (int i = 0; i <= steps; ++i) {
        int gridX = std::min(static_cast<int>(grid[0].size()) - 1, std::max(0, static_cast<int>(std::floor(x))));
        int gridY = std::min(static_cast<int>(grid.size()) - 1, std::max(0, static_cast<int>(std::floor(y))));

        if (print) {
            std::cout << "Set " << gridX << ", " << gridY << std::endl;
        }
        grid[gridY][gridX] = 1;

        x += xIncrement;
        y += yIncrement;
    }

    if (print)
    std::cout << std::endl;
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