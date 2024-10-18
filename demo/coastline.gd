@tool
extends Node

@export var material : Material
@export var add_seabed : bool


func import_polygons_geo(polygons : Array, geomap : GeoMap):
	for child in self.get_children():
		self.remove_child(child)
	var pu = PolyUtil.new()
	print(len(polygons))
	for poly : Array in polygons:
		var outer = poly[0]
		var inners = poly.slice(1)
		var triangles_geo = pu.triangulate_with_holes(outer, inners)
		if not triangles_geo:
			continue
			
		var triangles_world = RenderUtil.geo_polygon_to_world(triangles_geo, geomap)
		var normals = RenderUtil.geo_polygon_to_world_up(triangles_geo, geomap)
		
		
		var sea_area = RenderUtil.achild(self, Node3D.new(), "SeaArea")

		
		var water = RenderUtil.area_poly(sea_area, "Water", RenderUtil.polygon_triangles(triangles_world, 0, normals))
		water.mesh.surface_set_material(0, material)
		
		if add_seabed:
			var seabed = RenderUtil.achild(sea_area, MeshInstance3D.new(), "Seabed")
			seabed.mesh = water.mesh.duplicate()
			seabed.mesh.surface_set_material(0, load("res://assets/ground/ground.tres"))
			seabed.position.y -= 1
		
		
		
		
