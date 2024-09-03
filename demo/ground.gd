@tool
extends Node3D
class_name Ground

var heights = []
var coastline_polygons = []

func import_polygons(polygons : Array):
	coastline_polygons = polygons

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
	var cell_pos = grid.get_top_left_world()
	var left = cell_pos.x
	var cell_offset = (grid.get_bottom_right_world() - cell_pos) / Vector2(grid.ncols, grid.nrows)
	
	heights = grid.heightmap
	
	for row in range(grid.nrows - 1):
		for col in range(len(heights[row]) - 1):

			arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x, heights[row][col], cell_pos.y))
			arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x + cell_offset.x, heights[row][col + 1], cell_pos.y))
			arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x, heights[row + 1][col], cell_pos.y + cell_offset.y))
			
			arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x + cell_offset.x, heights[row][col + 1], cell_pos.y))
			arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x + cell_offset.x, heights[row + 1][col + 1], cell_pos.y + cell_offset.y))
			arrays[Mesh.ARRAY_VERTEX].append(Vector3(cell_pos.x, heights[row + 1][col], cell_pos.y + cell_offset.y))
			
			var normal1 = -Vector3(cell_offset.x, heights[row][col + 1] - heights[row][col], 0).cross(Vector3(0, heights[row + 1][col] - heights[row][col], cell_offset.y)).normalized()
			var normal2 = -Vector3(cell_offset.x, heights[row + 1][col + 1] - heights[row + 1][col], 0).cross(Vector3(0, heights[row + 1][col + 1] - heights[row][col + 1], cell_offset.y)).normalized()
		
			# Each vertex in a triangle has the same normal
			for j in range(3):
				arrays[Mesh.ARRAY_NORMAL].append(normal1)
			for j in range(3):
				arrays[Mesh.ARRAY_NORMAL].append(normal2)
			
			cell_pos.x += cell_offset.x
		
		cell_pos.x = left
		cell_pos.y += cell_offset.y
		
	print(len(arrays[Mesh.ARRAY_VERTEX]))
	print(cell_offset)
	print(cell_pos)
	
	util.area_poly(self, "GroundMesh", arrays,Color.GRAY)
	
