@tool
extends Node
var tile_info : Dictionary = {}
var node_pos : Dictionary = {}
var building_part_nodes : Dictionary = {} # TODO: Only works if building parts are defined before building.

enum RoofType
{
	FLAT,
	PYRAMIDAL,
	SKILLION,
	HIPPED,
	GABLED
}

func get_required_globals():
	return GlobalRequirementsBuilder.new().withNodeRequirement("pos").build()

func import_begin():
	tile_info = {}
	node_pos = {}
	building_part_nodes = {}

func import_node(osm_dict : Dictionary, fa : StreamPeer):
	node_pos[osm_dict["id"]] = osm_dict["pos_elevation"]
	
func import_way(osm_dict : Dictionary, fa : StreamPeer):
	#if osm_dict["id"] != 975494520:
	#	return
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
	var path : PackedVector3Array = []
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

func get_roof_arrays(d : Dictionary):
	var nodes = d["nodes"]
	var max_height = d["max_height"]
	var min_height = d["min_height"]
	match d["roof_type"]:
		RoofType.FLAT:
			return RenderUtil.polygon(nodes, max_height)
		RoofType.PYRAMIDAL:
			return RenderUtil.pyramid(nodes, d.get("roof_height", max_height - min_height), max_height)
		RoofType.SKILLION:
			return RenderUtil.skillion(nodes, d.get("roof_height", max_height - min_height), max_height, d["roof_dir"])
		RoofType.HIPPED:
			return RenderUtil.hipped(nodes, d.get("roof_height", max_height - min_height), max_height)
		RoofType.GABLED:
			return RenderUtil.hipped(nodes, d.get("roof_height", max_height - min_height), max_height, true)

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
		
		#if max_height - min_height > 0:
		#	RenderUtil.area_poly(building, "floor", RenderUtil.polygon(path["nodes"], min_height), color)
		
		# Label
		#var label = Label3D.new()
		#label.position = Vector3(path["nodes"][0].x,max_height + 10.0, path["nodes"][0].y)
		#label.pixel_size = 0.04
		#label.autowrap_mode = TextServer.AUTOWRAP_ARBITRARY
		#RenderUtil.achild(building, label, "Keys")
		#label.text = path["dupa"]
		
