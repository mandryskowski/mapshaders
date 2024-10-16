@tool
extends SGImport
	
@export var xd : Array[Node3D]
@export var zupa : int = 3


# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
	
func meow(osm_dict : Dictionary, fa : FileAccess):
	print("meow")
