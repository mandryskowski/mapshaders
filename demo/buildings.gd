@tool
extends Node
var tile_info : Dictionary = {}
var node_pos : Dictionary = {}
var building_part_nodes : Dictionary = {} # TODO: Only works if building parts are defined before building.

enum RoofType
{
	FLAT,
	PYRAMIDAL,
	SKILLION
}

func get_required_globals():
	return GlobalRequirementsBuilder.new().withNodeRequirement("pos").build()

func import_begin():
	tile_info = {}
	node_pos = {}
	building_part_nodes = {}

func import_node(osm_dict : Dictionary, fa : StreamPeer):
	node_pos[osm_dict["id"]] = osm_dict["pos"]
	
func import_way(osm_dict : Dictionary, fa : StreamPeer):
	if osm_dict.get("building:part", "no") != "no":
		# mark its nodes as building parts
		#for node_id in osm_dict["nodes"]:
		#	building_part_nodes[node_id] = true
		var a = 0
	elif osm_dict.get("building", "no") != "no":
		# check building's nodes for existing building parts
		for node_id in osm_dict["nodes"]:
			if building_part_nodes.has(node_id):
				return
	else:
		return

	if !tile_info.has(fa):
		tile_info[fa] = []
	var path : PackedVector2Array = []
	for node_id in osm_dict["nodes"]:
		if !node_pos.has(node_id):
			continue
		path.push_back(node_pos[node_id])
		
	tile_info[fa].push_back(RenderUtil.building_info(osm_dict, path))

func import_finished():
	for fa in tile_info:
		fa.put_var(tile_info[fa])
	for child in get_children():
		remove_child(child)


	
func get_roof_type(code : String):
	match code:
		"pyramidal": return RoofType.PYRAMIDAL
		"skillion": return RoofType.SKILLION
		"flat", _: return RoofType.FLAT
func get_roof_arrays(d : Dictionary):
	var nodes_3d = PackedVector3Array()
	for n in d["nodes"]:
		nodes_3d.push_back(Vector3(n.x, 0.0, n.y))
	var max_height = d["max_height"]
	var min_height = d["min_height"]
	match d["roof_type"]:
		RoofType.FLAT:
			return RenderUtil.polygon(nodes_3d, max_height)
		RoofType.PYRAMIDAL:
			return RenderUtil.pyramid(nodes_3d, d.get("roof_height", max_height - min_height), max_height)
		RoofType.SKILLION:
			return RenderUtil.skillion(nodes_3d, d.get("roof_height", max_height - min_height), max_height, d["roof_dir"])

func load_tile(fa : FileAccess):
	var paths = fa.get_var()
	for path in paths:
		# Path
		if path["nodes"].is_empty():
			continue

			
		var max_height = path["max_height"]
		var min_height = path["min_height"]
		var half_thickness = 0.01
		var color = path["color"]
		var roof_color = path["roof_color"] if path.has("roof_color") else color
		
		var building = RenderUtil.achild(self, Node3D.new(), "Building"+path["name"])
		RenderUtil.wall_poly_np(building, path["name"], path["nodes"], color, min_height, max_height)

		RenderUtil.area_poly(building, "roof", get_roof_arrays(path), roof_color)
		
		if max_height - min_height > 0:
			var nodes_3d = PackedVector3Array()
			for n in path["nodes"]:
				nodes_3d.push_back(Vector3(n.x, 0.0, n.y))
			RenderUtil.area_poly(building, "floor", RenderUtil.polygon(nodes_3d, min_height), color)
		
		# Label
		var label = Label3D.new()
		label.position = Vector3(path["nodes"][0].x,max_height + 10.0, path["nodes"][0].y)
		label.pixel_size = 0.04
		label.autowrap_mode = TextServer.AUTOWRAP_ARBITRARY
		RenderUtil.achild(building, label, "Keys")
		label.text = path["dupa"]
		