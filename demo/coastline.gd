@tool
extends Node


func import_polygons(polygons : Array):
	for child in self.get_children():
		self.remove_child(child)
	var pu = PolyUtil.new()
	print(len(polygons))
	for poly : Array in polygons:
		var outer = poly[0]
		var inners = poly.slice(1)
		var triangles = pu.triangulate_with_holes(outer, inners)
		var sea_area = RenderUtil.achild(self, Node3D.new(), "SeaArea")
		
		var water = RenderUtil.area_poly(sea_area, "Water", RenderUtil.polygon_triangles(triangles, 0))
		water.mesh.surface_set_material(0, load("res://assets/water/water.tres"))
		
		var seabed = RenderUtil.achild(sea_area, MeshInstance3D.new(), "Seabed")
		seabed.mesh = water.mesh.duplicate()
		seabed.mesh.surface_set_material(0, load("res://assets/ground/ground.tres"))
		seabed.position.y -= 1
		
		
		
		