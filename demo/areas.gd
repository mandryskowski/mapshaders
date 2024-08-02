@tool
extends Node
var tile_info : Dictionary = {}
var node_pos : Dictionary = {}
var ways : Dictionary = {}

func import_begin():
	tile_info = {}
	node_pos = {}
	ways = {}

func import_node(osm_dict : Dictionary, fa : StreamPeer):
	node_pos[osm_dict["id"]] = osm_dict["pos"]

func import_way(osm_dict : Dictionary, fa : StreamPeer):
	if (!tile_info.has("fa")):
		tile_info[fa] = {"ways": {}, "rels": []}
	tile_info[fa]["ways"][osm_dict["id"]] = osm_dict
	ways[osm_dict["id"]] = osm_dict
	if osm_dict.has("area"):
		print("area:", osm_dict["area"])
	
func import_relation(osm_dict : Dictionary, fa : StreamPeer):
	# Hospital test
	if (osm_dict["id"] == 106417):
		return
	if (osm_dict["id"] == 15697701 or true):
		print(osm_dict["id"])
		var outer = []
		var inners = []
		for member in osm_dict["members"]:
			var verts = []
			if (!ways.has(member["id"])):
				return
			for nid in ways[member["id"]]["nodes"]:
				if (!node_pos.has(nid)):
					if (osm_dict["id"] == 1165802220):
						print('nooo')
					return
				verts.append(node_pos[nid])
			if (member["role"] == "outer"):
				outer = verts
			elif (member["role"] == "inner" && member["id"] != 1159366629):
				inners.append(verts)
		print("outer ", outer)
		if outer == []:
			return
		var pu = PolyUtil.new()
		if (!tile_info[fa].has("rels")):
			tile_info[fa]["rels"] = []
		var outd : Dictionary = {}
		print("Triangulating", osm_dict['id'])
		outd["rooftris"] = pu.triangulate_with_holes(outer, inners)
		outd["outer"] = outer
		outd["inners"] = inners
		outd["name"] = osm_dict.get("name", str(osm_dict["id"]))
		if (osm_dict.has("height")):
			outd["max_height"] = osm_dict["height"]
		if (osm_dict.has("min_height")):
			outd["min_height"] = osm_dict["min_height"]
		tile_info[fa]["rels"].append(outd)

func import_finished():
	print("hau", len(tile_info))
	for fa in tile_info:
		fa.put_var(tile_info[fa])
	for child in get_children():
		remove_child(child)
	

func achild(parent, node, name):
	node.name = name
	parent.add_child (node, true)
	node.set_owner(get_tree().edited_scene_root)
	return node
	
func load_tile(fa : FileAccess):
	var d : Dictionary = fa.get_var()
	for rel in d["rels"]:
		if (!rel.has("rooftris")):
			continue
			
		var holeArea = achild(self, Node3D.new(), "HoleArea" + rel["name"])
		var triangles = rel["rooftris"]
		var min_height = float(rel.get("min_height", "0"))
		var max_height = float(rel.get("max_height", "0"))
		RenderUtil.area_poly(holeArea, "Roof"+rel["name"], RenderUtil.polygon_triangles(triangles, max_height))
		
		if (max_height - min_height) > 0:
			RenderUtil.wall_poly(holeArea, "outer", rel["outer"], Color.WHITE, 0.01, min_height, max_height)
			for inner in rel["inners"]:
				RenderUtil.wall_poly(holeArea, "inner", inner, Color.WHITE, 0.01, min_height, max_height)
			RenderUtil.area_poly(holeArea, "Floor"+rel["name"], RenderUtil.polygon_triangles(triangles, min_height))
