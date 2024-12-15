@tool
extends Button

@export
var search_node : TextEdit
@export
var item_list : ItemList
var list_geo_coords : PackedVector2Array

@export
var sgimport : SGImport

@export_tool_button("Search", "Callable")
var search_action = do_search
@onready var search_request : HTTPRequest = $PlaceNameRequest

func _ready() -> void:
	self.pressed.connect(self.do_search)
	search_request.request_completed.connect(self.finish_search)
	item_list.item_clicked.connect(self.clicked_search_item)

func do_search():
	print(search_node.text)
	search_request.request("https://nominatim.openstreetmap.org/search?format=jsonv2&q=" + search_node.text.uri_encode())

func finish_search(result, response_code, headers, body):
	var json = JSON.parse_string(body.get_string_from_utf8())
	item_list.clear()
	list_geo_coords.clear()
	
	if not json:
		print(result, response_code, body.get_string_from_utf8())
		return
	
	for place in json:
		item_list.add_item(place['display_name'])
		list_geo_coords.append(Vector2(float(place['lon']), float(place['lat'])))

func clicked_search_item(index: int, at_position: Vector2, mouse_button_index: int):
	var geomap = EquirectangularGeoMap.new()
	geomap.geo_origin_longitude_degrees = list_geo_coords[index].x
	geomap.geo_origin_latitude_degrees = list_geo_coords[index].y
	sgimport.geo_map = geomap
