@tool
extends Object
class_name RenderUtil
static func get_array_mesh_arrays(attributes : Array[int]) -> Array:
		var arrays = []
		arrays.resize(Mesh.ARRAY_MAX)
		for attribute in attributes:
			match attribute:
				Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL: arrays[attribute] = PackedVector3Array()
				Mesh.ARRAY_TANGENT: arrays[attribute] = PackedFloat32Array()
				Mesh.ARRAY_TEX_UV, Mesh.ARRAY_TEX_UV2: arrays[attribute] = PackedVector2Array()
				Mesh.ARRAY_INDEX: arrays[attribute] = PackedInt32Array()
		
		return arrays
		
static func get_middle_of_polygon(verts : PackedVector3Array):
		var mid = Vector3.ZERO
		for v in verts:
			mid += v
			
		return mid / verts.size()
			
static func enforce_winding(verts):
		#if normal.dot((verts[1] - verts[0]).cross(verts[2] - verts[0])) > 0.0:
		#	verts.reverse()
		var mid = get_middle_of_polygon(verts)
		var winding_val = 0
		for v in verts.size():
			var this = verts[v] - mid
			var next = verts[(v + 1) % verts.size()] - mid
			winding_val += (next.x - this.x) * (next.z + this.z)
		if winding_val > 0.0:
			verts.reverse()

static func polygon(verts : PackedVector3Array, height : float = 0.0) -> Array:
		var arrays = get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL, Mesh.ARRAY_TEX_UV])
		var nodes_2d = PackedVector2Array()
		for v in verts:
			nodes_2d.push_back(Vector2(v.x, v.z))
						
		var triangulated = Geometry2D.triangulate_polygon(nodes_2d)
		if triangulated.is_empty():
			return []
		
		for v_index in triangulated:
			var v = verts[v_index] + Vector3(0, height, 0)
			arrays[Mesh.ARRAY_VERTEX].push_back(v)
			arrays[Mesh.ARRAY_TEX_UV].push_back(Vector2(v.x, v.z) * 0.05)
			arrays[Mesh.ARRAY_NORMAL].push_back(Vector3(0,1,0))
		
		return arrays

static func polygon_triangles(triangles : PackedVector2Array, height = 0.0):
	var arrays = get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL])
	for v in triangles:
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v.x, height, v.y))
		arrays[Mesh.ARRAY_NORMAL].append(Vector3(0, 1, 0))
		
	return arrays
		
static func pyramid(verts : PackedVector3Array, height : float = 0.0, min_height : float = 0.0) -> Array:
		var arrays = get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL])
		
		var min_height_vec = Vector3(0, min_height, 0)
		
		var tip = get_middle_of_polygon(verts)
		tip.y += height + min_height
		
		
		
		for i in verts.size():
			var v1 = verts[i]
			var v2 = verts[(i + 1) % verts.size()]
			
			var normal = -(v2 - v1).cross(tip - v1).normalized()
			arrays[Mesh.ARRAY_VERTEX].append_array([v1 + min_height_vec, v2 + min_height_vec, tip])
			arrays[Mesh.ARRAY_NORMAL].append_array([normal, normal, normal])
		
		return arrays

static func skillion(verts : PackedVector3Array, height : float = 0.0, min_height :float = 0.0, direction_deg : float = 0.0) -> Array:
	if verts.is_empty():
		return []
		
	# Turn into polygon
	var arrays = get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL, Mesh.ARRAY_TEX_UV])
	var nodes_2d = PackedVector2Array()
	for v in verts:
		nodes_2d.push_back(Vector2(v.x, v.z))
					
	var triangulated = Geometry2D.triangulate_polygon(nodes_2d)
	if triangulated.is_empty():
		return []
		
	# Calculate range of projection values
	var dir_vec = Vector2.from_angle(PI / 2 - deg_to_rad(direction_deg))
	dir_vec.x *= -1.0
	var min_proj = nodes_2d[0].dot(dir_vec)
	var max_proj = min_proj
	
	for v in nodes_2d:
		var proj = v.dot(dir_vec)
		min_proj = min(min_proj, proj)
		max_proj = max(max_proj, proj)

	# Setup polygon mesh
	for v_index in triangulated:
		var slope = (nodes_2d[v_index].dot(dir_vec) - min_proj) / (max_proj - min_proj)
		var v = verts[v_index] + Vector3(0, min_height + height * slope, 0)
		arrays[Mesh.ARRAY_VERTEX].push_back(v)
		arrays[Mesh.ARRAY_TEX_UV].push_back(Vector2(v.x, v.z) * 0.05)
		arrays[Mesh.ARRAY_NORMAL].push_back(Vector3(0,1,0))
	
	return arrays
	
static func hipped(verts : PackedVector3Array, height : float = 0.0, min_height :float = 0.0) -> Array:
	if verts.is_empty():
		return []
		
	var nodes_2d = PackedVector2Array()
	for v in verts:
		nodes_2d.push_back(Vector2(v.x, v.z))
	
	var arrays = get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL, Mesh.ARRAY_TEX_UV])
	nodes_2d = [Vector2(40, 40),
Vector2(40, 310),
Vector2(520, 310),
Vector2(520, 40)
]
	var subtrees = PolyUtil.new().straight_skeleton(nodes_2d, [])
	subtrees[0].source = Vector2(40, 175)
	subtrees[1].source = Vector2(520, 175)
	
	subtrees[1].sinks[0] = subtrees[0].source
	# BFS
	var subtree_map = {}
	for subtree in subtrees:
		subtree_map[subtree.source] = subtree
	
	for i in range(len(nodes_2d)):
		var v1 = nodes_2d[i] # this vertex
		var v2 = nodes_2d[(i + 1) % len(nodes_2d)] # next vertex 
		
		var queue = []
		var visited = {}
		var subtrees_to_v2 = [] # subtrees from v1 to v2
		
		for subtree : SkeletonSubtree in subtrees:
			var found = false
			for sink in subtree.sinks:
				if sink.is_equal_approx(v1):
					queue.append([subtree])
					visited[subtree] = true
					found = true
					break
			if found:
				break
		
		while queue.size() > 0:
			var nodes_so_far : Array = queue.pop_front()
			var current_node: SkeletonSubtree = nodes_so_far.back()
			
			if v2 in current_node.sinks:
				subtrees_to_v2 = nodes_so_far
				break
				
			for neighbor in current_node.sinks:
				if subtree_map.has(neighbor):
					var neighbor_subtree = subtree_map[neighbor]
					if not visited.has(neighbor_subtree):
						queue.append(nodes_so_far + [neighbor_subtree])
						visited[neighbor_subtree] = true
			for subtree in subtrees:
				if current_node.source in subtree.sinks:
					if not visited.has(subtree):
						queue.append(nodes_so_far + [subtree])
						visited[subtree] = true
				
		var poly = [v1]
		for subtree : SkeletonSubtree in subtrees_to_v2:
			poly.append(subtree.source)
		poly.append(v2)
		
		var triangulated = Geometry2D.triangulate_polygon(poly)
		print("triangulated is",  triangulated)
		
		for tri in range(0, len(triangulated), 3):
			var vs = []
			
			for tri_v in range(3):
				var v_index = triangulated[tri + tri_v]
				var v = Vector3(poly[v_index].x, min_height, poly[v_index].y)
				if v_index != 0 and v_index != len(poly) - 1:
					v.y += subtrees_to_v2[v_index - 1].height
					
				vs.append(v)
					
			var normal = (vs[2] - vs[1]).cross(vs[1] - vs[0])
					
			for v in vs:
				arrays[Mesh.ARRAY_VERTEX].push_back(v)
				arrays[Mesh.ARRAY_TEX_UV].push_back(Vector2(v.x, v.z))
				arrays[Mesh.ARRAY_NORMAL].push_back(normal)
			
	return arrays
	
	
static func achild(parent, node, name):
	node.name = name
	parent.add_child (node, true)
	node.set_owner(parent.get_tree().edited_scene_root)
	return node
	
static func wall_poly(parent, name, nodes, color, half_thickness, min_height, max_height):
	var child : Path3D = achild(parent, Path3D.new(), name)
	child.curve = Curve3D.new()
	for node in nodes:
		child.curve.add_point(Vector3(node.x, 0.0, node.y))
	var poly : CSGPolygon3D = achild(child, CSGPolygon3D.new(), "polygon")
	poly.mode = CSGPolygon3D.MODE_PATH
	poly.path_node = child.get_path()
	poly.path_interval = 1
	poly.path_simplify_angle = 0.1
	poly.material = StandardMaterial3D.new()
	poly.material.albedo_color = color
	poly.polygon = PackedVector2Array([Vector2(-half_thickness, min_height), Vector2(-half_thickness, max_height), Vector2(half_thickness, max_height), Vector2(half_thickness, min_height)])

static func wall_poly_np(parent, name, nodes, color, min_height, max_height):
	if nodes.size() < 3:
		return

	var arrays = get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_NORMAL])
	for i in nodes.size() - 1:
		var v_this = nodes[i]
		var v_next = nodes[i+1]
		
		# Triangle 1
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v_this.x, min_height, v_this.y))
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v_next.x, min_height, v_next.y))
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v_next.x, max_height, v_next.y))
		
		# Triangle 2
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v_this.x, min_height, v_this.y))
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v_next.x, max_height, v_next.y))
		arrays[Mesh.ARRAY_VERTEX].append(Vector3(v_this.x, max_height, v_this.y))
		
		var normal = Vector3(v_next.x - v_this.x, 0, v_next.y - v_this.y).cross(-Vector3.UP)
		
		# Each vertex has the same normal
		for j in range(6):
			arrays[Mesh.ARRAY_NORMAL].append(normal)
		
	area_poly(parent, name, arrays, color)

static func area_poly(parent, name, arrays, color = Color.WHITE):
	var area = achild(parent, MeshInstance3D.new(), name)
	area.mesh = ArrayMesh.new()
	if !arrays.is_empty():
		area.mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
		area.mesh.surface_set_material(0, StandardMaterial3D.new())
		area.mesh.surface_get_material(0).cull_mode = StandardMaterial3D.CULL_DISABLED
		area.mesh.surface_get_material(0).albedo_color = color

enum RoofType
{
	FLAT,
	PYRAMIDAL,
	SKILLION,
	HIPPED
}

static func get_roof_type(code : String):
	match code:
		"pyramidal": return RoofType.PYRAMIDAL
		"skillion": return RoofType.SKILLION
		"hipped": return RoofType.HIPPED
		"flat", _: return RoofType.FLAT
		
static func osm_to_gd_color(code : String):
	return Color(code.replace("grey", "gray"))

static func building_info(osm_d : Dictionary, nodes : PackedVector2Array):
	var d = {"name" : osm_d.get("name", str(osm_d["id"])), "nodes": nodes,
			 "max_height": osm_d.get("height", "3").to_float(),
			 "min_height": osm_d.get("min_height", "0").to_float(),
			 "color": osm_to_gd_color(osm_d.get("building:colour", "white")),
			 "roof_type": get_roof_type(osm_d.get("roof:shape", "flat"))}
	if (osm_d.has("roof:colour")):
		d["roof_color"] = osm_to_gd_color(osm_d["roof:colour"])
	if (osm_d.has("roof:height")):
		d["max_height"] -= osm_d["roof:height"].to_float()
		d["roof_height"] = osm_d["roof:height"].to_float()
	if (osm_d.has("roof:direction")):
		d["roof_dir"] = osm_d["roof:direction"].to_float()
		
	apply_defaults(d, osm_d)
	
	var label_d = osm_d
	label_d.erase("nodes")
	label_d.erase("element_type")
	label_d.erase("id")
	d["dupa"] = str(label_d)
	return d

static func apply_defaults(d, osm_d):
	# HEIGHT
	if !osm_d.has("height") and osm_d.has("building:levels"):
		var levels = osm_d["building:levels"].to_float()
		if osm_d.has("roof:levels"):
			levels += osm_d["roof:levels"].to_float()
		d["max_height"] = levels * 3 # floor is 3m high
		
	# COLOR/TEXTURE
	if !osm_d.has("building:color") and osm_d.has("building:material"):
		var c = Color.WHITE
		match osm_d["building:material"]:
			"brick" : c=Color.DARK_RED
			
		d["color"] = c
			
