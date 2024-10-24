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
	
func get_normal_from_pos(heightmap, pos):
	var col = clamp(pos.x, 1, len(heightmap[0]) - 2)
	var row = clamp(pos.y, 1, len(heightmap) - 2)
	return get_normal(heights[row][col - 1], heights[row][col + 1], heights[row + 1][col], heights[row - 1][col])
	
func triangulate_rectangle(verts : PackedVector3Array) -> PackedVector3Array:
	assert(len(verts) == 4)
	return [verts[0], verts[1], verts[3],
			verts[1], verts[2], verts[3]]

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
	var flat = true
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
	for v in coastline_polygons[3][0]:
		xd.append(v)
		
	for i in range(len(gridrectpoly)):
		gridrectpoly[i] *= 100
	for i in range(len(xd)):
		xd[i] *= 100
	
	gridrectpoly = Geometry2D.intersect_polygons(gridrectpoly, xd)[0]
	
	for i in range(len(gridrectpoly)):
		gridrectpoly[i] *= 0.01
	for i in range(len(xd)):
		xd[i] *= 0.01
	
	var gridrectpoly3d : PackedVector3Array
	for v in gridrectpoly:
		gridrectpoly3d.append(coastline_geomap.geo_to_world(v))
		
	print(gridrectpoly)
	
	RenderUtil.area_poly(self, "zupa", RenderUtil.polygon(gridrectpoly3d, 0))

	
	for row in range(grid.nrows - 1):
		for col in range(len(heights[row]) - 1):
			var poly : PackedVector2Array
			poly.append(cell_pos)
			poly.append(cell_pos + Vector2(cell_offset.x, 0))
			poly.append(cell_pos + cell_offset)
			poly.append(cell_pos + Vector2(0, cell_offset.y))
			
			for i in range(len(poly)):
				poly[i] *= 100
			for i in range(len(gridrectpoly)):
				gridrectpoly[i] *= 100
			var out = Geometry2D.intersect_polygons(poly, gridrectpoly)
			for i in range(len(poly)):
				poly[i] *= 0.01
			for i in range(len(gridrectpoly)):
				gridrectpoly[i] *= 0.01
			if not out:
				cell_pos.x += cell_offset.x
				continue
			for i in range(len(out[0])):
				out[0][i] *= 0.01
			var pu = PolyUtil.new()
			
			var poly3d : PackedVector3Array
			var hmap = ElevationHeightmap.new()
			hmap.elevation_grid = grid
			var triangulation_indices = Geometry2D.triangulate_polygon(out[0])
			poly = []
			for i in triangulation_indices:
				var v_geo = out[0][i]
				var v_world = coastline_geomap.geo_to_world(v_geo)
				poly3d.append(Vector3(v_world.x, hmap.get_elevation_vec(v_geo), v_world.z))
				arrays[Mesh.ARRAY_NORMAL].append(Vector3(0,1,0))
			arrays[Mesh.ARRAY_VERTEX].append_array(poly3d)

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
	
