@tool
extends Node
var tile_info : Dictionary = {}
var node_pos : Dictionary = {}

func import_begin():
	tile_info = {}
	node_pos = {}
	for child in self.get_children():
		self.remove_child(child)

func import_node(osm_dict : Dictionary, fa : StreamPeer):
	node_pos[osm_dict["id"]] = osm_dict["pos"]
	
func import_way(osm_dict : Dictionary, fa : StreamPeer):
	

	if osm_dict.has("highway"):
		var path : PackedVector2Array = []
		for node_id in osm_dict["nodes"]:
			if !node_pos.has(node_id):
				continue
			path.push_back(node_pos[node_id])
			
		if !tile_info.has(fa):
			tile_info[fa] = []
		tile_info[fa].push_back({"name" : osm_dict.get("name", str(osm_dict["id"])), "nodes": path})

func import_finished():
	for fa in tile_info:
		var start = fa.get_position()
		fa.put_var(tile_info[fa])

func achild(parent, node, name):
	node.name = name
	parent.add_child (node, true)
	node.set_owner(get_tree().edited_scene_root)
	return node
	
func load_tile(fa : FileAccess):
	var paths = fa.get_var()
	for path in paths:
		# Path
		var child : Path3D = achild(self, Path3D.new(), path["name"])
		child.curve = Curve3D.new()
		for node in path["nodes"]:
			child.curve.add_point(Vector3(node.x, 0.0, node.y))
			
		var poly : CSGPolygon3D = achild(child, CSGPolygon3D.new(), "polygon")
		poly.mode = CSGPolygon3D.MODE_PATH
		poly.path_node = child.get_path()
		poly.path_interval = 0.5
		poly.path_interval_type = CSGPolygon3D.PATH_INTERVAL_SUBDIVIDE
		poly.path_simplify_angle = 0.1
		poly.polygon = PackedVector2Array([Vector2(-1, 0), Vector2(-1, 0.1), Vector2(1, 0.1), Vector2(1, 0)])
