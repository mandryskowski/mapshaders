@tool
extends Node
var tile_info : Dictionary = {}
var node_pos : Dictionary = {}
var building_part_nodes : Dictionary = {} # TODO: Only works if building parts are defined before building.

@export var one_node_per_building : bool = false

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
	#if osm_dict["id"] != 471388226:
	
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
		
	RenderUtil.enforce_winding(path)
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
	
	# For combined node
	# (color, material) -> Array[Array]
	var material_arrays : Dictionary
	var add_to_material_arrays = func (color, material, arrays):
		var key = [color, material]
		if not material_arrays.has(key):
			material_arrays[key] = RenderUtil.get_array_mesh_arrays([Mesh.ARRAY_VERTEX, Mesh.ARRAY_TEX_UV, Mesh.ARRAY_NORMAL])
		RenderUtil.combine_mesh_arrays(material_arrays[key], arrays)

	for path in paths:
		# Path
		if path["nodes"].is_empty():
			continue
		
		var max_height = path["max_height"]
		var min_height = path["min_height"]
		var half_thickness = 0.01
		var color = path["color"]
		var roof_color = path["roof_color"] if path.has("roof_color") else color
		
		var walls_arrays = RenderUtil.wall_poly_np(path["nodes"], min_height, max_height)
		var roof_arrays = get_roof_arrays(path)
		
		if one_node_per_building:
			var building = RenderUtil.achild(self, Node3D.new(), "Building"+path["name"])
			RenderUtil.area_poly(building, path["name"], walls_arrays, color)
			RenderUtil.area_poly(building, "roof", roof_arrays, roof_color)
		else:
			add_to_material_arrays.call(color, '', walls_arrays)
			add_to_material_arrays.call(roof_color, '', roof_arrays)

		#if max_height - min_height > 0:
		#	RenderUtil.area_poly(building, "floor", RenderUtil.polygon(path["nodes"], min_height), color)
		
		# Label
		#var label = Label3D.new()
		#label.position = Vector3(path["nodes"][0].x,max_height + 10.0, path["nodes"][0].y)
		#label.pixel_size = 0.04
		#label.autowrap_mode = TextServer.AUTOWRAP_ARBITRARY
		#RenderUtil.achild(building, label, "Keys")
		#label.text = path["dupa"]
		
	if not one_node_per_building:
		for key in material_arrays:
			var arrays = material_arrays[key]
			RenderUtil.area_poly(self, "Combined mesh", arrays, key[0], true)
