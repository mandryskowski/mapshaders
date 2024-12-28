@tool
extends Button

@export var sgimport : SGImport
@export var size_slider : Slider
@onready var download_request : HTTPRequest = $DownloadRequest
@onready var elevation_request : HTTPRequest = $ElevationRequest

var finish_count : int = 0

func _ready() -> void:
	self.pressed.connect(self.download)
	download_request.request_completed.connect(self.finish_download)
	elevation_request.request_completed.connect(self.finish_download)
	
func download():
	var origin = Vector2(sgimport.geo_map.geo_origin_longitude_degrees, sgimport.geo_map.geo_origin_latitude_degrees)
	var size = size_slider.value
	var bb = [origin.x - size, origin.y - size, origin.x + size, origin.y + size]
	var bb_str : String
	
	for side in bb:
		bb_str += str(side) + ","
	bb_str = bb_str.erase(len(bb_str) - 1)
	print(bb_str)
	
	download_request.request("https://overpass-api.de/api/map?bbox=" + bb_str)
	elevation_request.request("https://portal.opentopography.org/API/globaldem?demtype=NASADEM&south=" + str(origin.y - size) + "&north=" + str(origin.y + size) + "&west=" + str(origin.x - size) + "&east=" + str(origin.x + size) + "&API_Key=" + OS.get_environment("OPENTOPOGRAPHY_API_KEY"))
	
	
func finish_download(result, response_code, headers, body):
	finish_count += 1
	if finish_count == 1:
		return
	print("Finished!", response_code)
	for parser in sgimport.parsers:
		if is_instance_of(parser, OSMParser):
			var parser_osm : OSMParser = parser
			parser_osm.filename = "res://map.osm"
		elif is_instance_of(parser, ElevationParser):
			var parser_elevation : ElevationParser = parser
			parser_elevation.filename = "res://map.tif"
	sgimport.import(true)
	sgimport.load_tiles(true)
	
