#include "Coastline.h"

#include <godot_cpp/classes/node3d.hpp>

#include "../../../src/util/PolyUtil.h"
#include "../../common/3D/RenderUtil3D.h"

using namespace godot;

void Coastline::import_polygons_geo(godot::Array polygons,
                                    godot::Ref<GeoMap> geomap) {
  for (int i = 0; i < polygons.size(); i++) {
    Array poly = static_cast<Array>(polygons[i]);

    std::vector<GeoCoords> outer;
    {
      Array outer_two_axes = poly[0];
      PackedFloat64Array outer_x = outer_two_axes[0],
                         outer_y = outer_two_axes[1];
      for (int j = 0; j < outer_x.size(); j++) {
        outer.push_back(GeoCoords(Longitude::radians(outer_x[j]),
                                  Latitude::radians(outer_y[j])));
      }
    }

    std::vector<std::vector<GeoCoords>> inners;
    {
      for (int j = 1; j < poly.size(); j++) {
        Array inner_two_axes = poly[j];
        PackedFloat64Array inner_x = inner_two_axes[0],
                           inner_y = inner_two_axes[1];
        std::vector<GeoCoords> inner;
        for (int k = 0; k < inner_x.size(); k++) {
          inner.push_back(GeoCoords(Longitude::radians(inner_x[k]),
                                    Latitude::radians(inner_y[k])));
        }
        inners.push_back(inner);
      }
    }

    auto pu = memnew(PolyUtil);
    auto triangles_geo = pu->triangulate_with_holes(outer, inners);
    if (triangles_geo.empty()) {
      continue;
    }

    auto triangles_world =
        RenderUtil3D::geo_polygon_to_world(triangles_geo, geomap.ptr());
    auto normals =
        RenderUtil3D::geo_polygon_to_world_up(triangles_geo, geomap.ptr());

    if (RenderUtil3D::enforce_winding(triangles_world, false)) {  // fix this
      normals.reverse();
    }
    auto sea_area = RenderUtil3D::achild(this, memnew(Node3D), "SeaArea");

    auto water = RenderUtil3D::area_poly(
        sea_area, "Water",
        RenderUtil3D::polygon_triangles(triangles_world, 0.0, &normals));
    water->get_mesh()->surface_set_material(0, material);

    if (add_seabed) {
      auto seabed = Object::cast_to<MeshInstance3D>(
          RenderUtil3D::achild(sea_area, memnew(MeshInstance3D), "Seabed"));
      seabed->set_mesh(water->get_mesh()->duplicate());
      // seabed->get_mesh()->surface_set_material(0,
      // load("res://assets/ground/ground.tres"));
      seabed->set_position(seabed->get_position() + Vector3(0, -1, 0));
    }
  }
}

void Coastline::_bind_methods() {
  ClassDB::bind_method(D_METHOD("import_polygons_geo", "polygons", "geomap"),
                       &Coastline::import_polygons_geo);
  ClassDB::bind_method(D_METHOD("set_material", "mat"),
                       &Coastline::set_material);
  ClassDB::bind_method(D_METHOD("get_material"), &Coastline::get_material);
  ClassDB::bind_method(D_METHOD("set_add_seabed", "add"),
                       &Coastline::set_add_seabed);
  ClassDB::bind_method(D_METHOD("get_add_seabed"), &Coastline::get_add_seabed);

  ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "material",
                            PROPERTY_HINT_RESOURCE_TYPE, "Material"),
               "set_material", "get_material");
  ADD_PROPERTY(PropertyInfo(Variant::BOOL, "add_seabed"), "set_add_seabed",
               "get_add_seabed");
}
