@tool
extends Node3D
class_name Ground

var heights = []
var coastline_polygons = []
var coastline_geomap : GeoMap

func to_vec2(v : Vector3) -> Vector2:
	return Vector2(v.x, v.z)
	
func import_polygons_geo(polygons : Array, geomap : GeoMap):
	print("hello")
	coastline_polygons = polygons
	coastline_geomap = geomap
	
func get_normal(L, R, B, T) -> Vector3:
	return -Vector3((R - L) / 2, -1, (B - T) / 2).normalized()
	
func bilinear(heightmap, pos):
	var pos_floor = pos.floor()
	pos_floor.x = clamp(pos_floor.x, 0, len(heightmap[0]) - 2)
	pos_floor.y = clamp(pos_floor.y, 0, len(heightmap) - 2)

	var delta = pos - pos_floor
	
	var Q11 = heightmap[pos_floor.y][pos_floor.x]
	var Q21 = heightmap[pos_floor.y][pos_floor.x + 1]
	var Q12 = heightmap[pos_floor.y + 1][pos_floor.x]
	var Q22 = heightmap[pos_floor.y + 1][pos_floor.x + 1]
	
	var elevation = (1 - delta.x) * (1 - delta.y) * Q11 + delta.x * (1 - delta.y) * Q21 + (1 - delta.x) * delta.y * Q12 + delta.x * delta.y * Q22
	return elevation
	
func get_normal_from_pos(heightmap, pos, delta = 0.2):
	return get_normal(bilinear(heightmap, pos + Vector2(-delta, 0)), bilinear(heightmap, pos + Vector2(delta, 0)), bilinear(heightmap, pos + Vector2(0, delta)), bilinear(heightmap, pos + Vector2(0, -delta)))
			
func generate_full_rectangle(cell_pos, cell_offset, grid, flat, arrays):
	var poly : PackedVector2Array
	poly.append(cell_pos)
	poly.append(cell_pos + Vector2(cell_offset.x, 0))
	poly.append(cell_pos + cell_offset)
	poly.append(cell_pos + Vector2(0, cell_offset.y))

	var pu = PolyUtil.new()
	
	var poly3d : PackedVector3Array
	var normals : PackedVector3Array
	var hmap = ElevationHeightmap.new()
	hmap.elevation_grid = grid

	var triangulation_indices = [0, 1, 2, 0, 2, 3]
	poly3d = []
	for i in triangulation_indices:
		var v_geo = poly[i]
		var v_world = coastline_geomap.geo_to_world(v_geo)
		poly3d.append(Vector3(v_world.x, hmap.get_elevation_vec(v_geo), v_world.z))
		if flat:
			normals.append(Vector3(0,1,0))
		else:
			var this_pos = ((v_geo - grid.get_top_left_geo()) / cell_offset).round()
			normals.append(get_normal_from_pos(heights, this_pos))

	var util = RenderUtil.new()
	util.enforce_winding(poly3d, true, [normals])
	arrays[Mesh.ARRAY_VERTEX].append_array(poly3d)
	arrays[Mesh.ARRAY_NORMAL].append_array(normals)
			
func generate_partial_rectangle(cell_pos, cell_offset, gridrectpoly, grid, flat, arrays):
	var poly : PackedVector2Array
	poly.append(cell_pos)
	poly.append(cell_pos + Vector2(cell_offset.x, 0))
	poly.append(cell_pos + cell_offset)
	poly.append(cell_pos + Vector2(0, cell_offset.y))
	
	for i in range(len(poly)):
		poly[i] *= 1000
	for i in range(len(gridrectpoly)):
		gridrectpoly[i] *= 1000
	var out = Geometry2D.intersect_polygons(poly, gridrectpoly)
	for i in range(len(poly)):
		poly[i] *= 0.001
	for i in range(len(gridrectpoly)):
		gridrectpoly[i] *= 0.001
	if not out:
		return
	for i in range(len(out[0])):
		out[0][i] *= 0.001
	var pu = PolyUtil.new()
	
	var poly3d : PackedVector3Array
	var normals : PackedVector3Array
	var hmap = ElevationHeightmap.new()
	hmap.elevation_grid = grid
	var triangulation_indices = Geometry2D.triangulate_polygon(out[0])
	poly = []
	for i in triangulation_indices:
		var v_geo = out[0][i]
		var v_world = coastline_geomap.geo_to_world(v_geo)
		poly3d.append(Vector3(v_world.x, hmap.get_elevation_vec(v_geo), v_world.z))
		if flat:
			normals.append(Vector3(0,1,0))
		else:
			var this_pos = ((v_geo - grid.get_top_left_geo()) / cell_offset).round()
			normals.append(get_normal_from_pos(heights, this_pos))
			
	var util = RenderUtil.new()
	util.enforce_winding(poly3d, true, [normals])
	arrays[Mesh.ARRAY_VERTEX].append_array(poly3d)
	arrays[Mesh.ARRAY_NORMAL].append_array(normals)

func import_grid(grid : ElevationGrid):
	print('importing the ground', grid.ncols, grid.nrows)
	print(grid.get_bottom_left_world())
	print(grid.get_top_right_world())
	
	for child in self.get_children():
		self.remove_child(child)

	var util = RenderUtil.new()
	var arrays = util.get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL])
	
	print("bottom left ", grid.get_bottom_left_world())
	print("bottom right ", grid.get_bottom_right_world())
	print("top left ", grid.get_top_left_world())
	print("top right ", grid.get_top_right_world())
	var cell_pos = grid.get_top_left_geo()
	var left = cell_pos.x
	var flat = false
	heights = grid.heightmap
	
	# Grid rectangle
	var cellrad = deg_to_rad(grid.cellsize)
	var cell_offset = Vector2(cellrad, -cellrad)
	var gridrectpoly : PackedVector2Array
	gridrectpoly.append(grid.get_top_left_geo())
	gridrectpoly.append(grid.get_top_left_geo() + cellrad * Vector2(grid.ncols, 0))
	gridrectpoly.append(grid.get_top_left_geo() + cellrad * Vector2(grid.ncols, -grid.nrows))
	gridrectpoly.append(grid.get_top_left_geo() + cellrad * Vector2(0, -grid.nrows))

	
	var xd : PackedVector2Array
	for p in coastline_polygons:
		print(len(p[0]))
	for v in coastline_polygons[0][0]:
		xd.append(v)
	
	for i in range(len(gridrectpoly)):
		gridrectpoly[i] *= 100
	for i in range(len(xd)):
		xd[i] *= 100
	gridrectpoly = Geometry2D.intersect_polygons(gridrectpoly, xd)[0]
	for i in range(len(gridrectpoly)):
		gridrectpoly[i] /= 100
	for i in range(len(xd)):
		xd[i] /= 100
		
	var gridrectpoly3d : PackedVector3Array
	for v in gridrectpoly:
		gridrectpoly3d.append(coastline_geomap.geo_to_world(v))
		
	print(gridrectpoly)
	
	RenderUtil.area_poly(self, "zupa", RenderUtil.polygon(gridrectpoly3d, 0))

	print("Total rows: ", grid.nrows)
	for row in range(grid.nrows - 1):
		if row % 20 == 0:
			print(row)
		for col in range(len(heights[row]) - 1):
			generate_partial_rectangle(cell_pos, cell_offset, gridrectpoly, grid, flat, arrays)
			#generate_full_rectangle(cell_pos, cell_offset, grid, flat, arrays)

			#arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x, heights[row][col], cell_pos.y))
			#arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x + cell_offset.x, heights[row][col + 1], cell_pos.y))
			#arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x, heights[row + 1][col], cell_pos.y + cell_offset.y))
			#
			#arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x + cell_offset.x, heights[row][col + 1], cell_pos.y))
			#arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x + cell_offset.x, heights[row + 1][col + 1], cell_pos.y + cell_offset.y))
			#arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x, heights[row + 1][col], cell_pos.y + cell_offset.y))
			#
			#var normal1 = 	-Vector3(cell_offset.x, heights[row][col + 1] - heights[row][col], 0).cross(Vector3(0, heights[row + 1][col] - heights[row][col], cell_offset.y)).normalized()
			#var normal2 = -Vector3(cell_offset.x, heights[row + 1][col + 1] - heights[row + 1][col], 0).cross(Vector3(0, heights[row + 1][col + 1] - heights[row][col + 1], cell_offset.y)).normalized()
		#
			## Each vertex in a triangle has the same normal
			#if not flat:
				#arrays[Mesh.ARRAY_NORMAL].append(get_normal_from_pos(heights, Vector2(col, row)))
				#arrays[Mesh.ARRAY_NORMAL].append(get_normal_from_pos(heights, Vector2(col + 1, row)))
				#arrays[Mesh.ARRAY_NORMAL].append(get_normal_from_pos(heights, Vector2(col, row + 1)))
				#
				#arrays[Mesh.ARRAY_NORMAL].append(get_normal_from_pos(heights, Vector2(col + 1, row)))
				#arrays[Mesh.ARRAY_NORMAL].append(get_normal_from_pos(heights, Vector2(col + 1, row + 1)))
				#arrays[Mesh.ARRAY_NORMAL].append(get_normal_from_pos(heights, Vector2(col, row + 1)))
			#else:
				#for j in range(3):
					#arrays[Mesh.ARRAY_NORMAL].append(normal1)
				#for j in range(3):
					#arrays[Mesh.ARRAY_NORMAL].append(normal2)
			#
			cell_pos.x += cell_offset.x
		
		cell_pos.x = left
		cell_pos.y += cell_offset.y
		
	print(len(arrays[Mesh.ARRAY_VERTEX]))
	print(cell_offset)
	print(cell_pos)
	print("end")
	util.area_poly(self, "GroundMesh", arrays,Color.GRAY)
	coastline_polygons = []
	
