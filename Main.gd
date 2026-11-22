extends Node3D

# GLOBAL CITY 0.2 — low-object-count Android prototype.
# The city stays intentionally empty: base platform, roads and one static character.
# Static scenery is merged by material to reduce MeshInstance3D count and draw calls.

const WORLD_SIZE: float = 140.0
const ROAD_WIDTH: float = 10.0
const ROAD_SPACING: float = 34.0
const SIDEWALK_WIDTH: float = 2.2
const SIDEWALK_OFFSET: float = ROAD_WIDTH * 0.5 + 1.4

var mat_ground: StandardMaterial3D
var mat_road: StandardMaterial3D
var mat_sidewalk: StandardMaterial3D
var mat_line: StandardMaterial3D
var mat_skin: StandardMaterial3D
var mat_shirt: StandardMaterial3D
var mat_pants: StandardMaterial3D
var mat_dark: StandardMaterial3D

func _ready() -> void:
	_make_materials()
	_make_environment()
	_make_ground()
	_make_roads()
	_make_static_character()
	_make_camera()
	_make_sun()
	_make_title_label()

func _make_materials() -> void:
	mat_ground = _make_material(Color("#ffffff"), "res://assets/grass_tile.png", Vector3(28.0, 28.0, 1.0))
	mat_road = _make_material(Color("#ffffff"), "res://assets/asphalt_tile.png", Vector3(8.0, 8.0, 1.0))
	mat_sidewalk = _make_material(Color("#b9bbb5"))
	mat_line = _make_material(Color("#eee5b7"))
	mat_skin = _make_material(Color("#d9a078"))
	mat_shirt = _make_material(Color("#2676a8"))
	mat_pants = _make_material(Color("#263448"))
	mat_dark = _make_material(Color("#20242a"))

func _make_material(color: Color, texture_path: String = "", uv_scale: Vector3 = Vector3.ONE) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = 0.94
	material.metallic = 0.0
	if not texture_path.is_empty():
		var texture := load(texture_path) as Texture2D
		if texture != null:
			material.albedo_texture = texture
			material.uv1_scale = uv_scale
	return material

func _make_environment() -> void:
	var world := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_SKY
	var sky := Sky.new()
	var sky_material := ProceduralSkyMaterial.new()
	sky_material.sky_top_color = Color("#4d82b5")
	sky_material.sky_horizon_color = Color("#c4d9e6")
	sky_material.ground_bottom_color = Color("#52624c")
	sky_material.ground_horizon_color = Color("#aab7a0")
	sky.sky_material = sky_material
	environment.sky = sky
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color("#c9d9e7")
	environment.ambient_light_energy = 0.78
	world.environment = environment
	add_child(world)

func _make_ground() -> void:
	var ground_boxes: Array[Dictionary] = []
	_add_box_spec(ground_boxes, Vector3(WORLD_SIZE, 1.0, WORLD_SIZE), Vector3(0.0, -0.55, 0.0))
	_make_box_batch("GroundBatch", ground_boxes, mat_ground)

func _make_roads() -> void:
	var road_boxes: Array[Dictionary] = []
	var sidewalk_boxes: Array[Dictionary] = []
	var marking_boxes: Array[Dictionary] = []
	for i in range(-2, 3):
		var road_offset := float(i) * ROAD_SPACING
		_add_box_spec(road_boxes, Vector3(ROAD_WIDTH, 0.12, WORLD_SIZE), Vector3(road_offset, 0.02, 0.0))
		_add_box_spec(road_boxes, Vector3(WORLD_SIZE, 0.12, ROAD_WIDTH), Vector3(0.0, 0.025, road_offset))
		for segment in range(-6, 7):
			_add_box_spec(marking_boxes, Vector3(0.18, 0.025, 2.0), Vector3(road_offset, 0.095, float(segment) * 10.0))
			_add_box_spec(marking_boxes, Vector3(2.0, 0.025, 0.18), Vector3(float(segment) * 10.0, 0.10, road_offset))
		for side in [-1.0, 1.0]:
			var edge := SIDEWALK_OFFSET
			_add_box_spec(sidewalk_boxes, Vector3(SIDEWALK_WIDTH, 0.18, WORLD_SIZE), Vector3(road_offset + side * edge, 0.11, 0.0))
			_add_box_spec(sidewalk_boxes, Vector3(WORLD_SIZE, 0.18, SIDEWALK_WIDTH), Vector3(0.0, 0.115, road_offset + side * edge))
	_make_box_batch("RoadBatch", road_boxes, mat_road)
	_make_box_batch("SidewalkBatch", sidewalk_boxes, mat_sidewalk)
	_make_box_batch("RoadMarkingsBatch", marking_boxes, mat_line)

func _add_box_spec(target: Array[Dictionary], size: Vector3, position: Vector3) -> void:
	target.append({"size": size, "position": position})

func _make_box_batch(node_name: String, boxes: Array[Dictionary], material: Material) -> void:
	if boxes.is_empty():
		return
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	var box_mesh_cache: Dictionary = {}
	for box_spec in boxes:
		var box_size: Vector3 = box_spec["size"]
		if not box_mesh_cache.has(box_size):
			var cached_box := BoxMesh.new()
			cached_box.size = box_size
			box_mesh_cache[box_size] = cached_box
		var transform := Transform3D(Basis.IDENTITY, box_spec["position"])
		surface.append_from(box_mesh_cache[box_size], 0, transform)
	var merged_mesh := surface.commit()
	if merged_mesh == null:
		push_warning("No se pudo construir el lote: " + node_name)
		return
	var instance := MeshInstance3D.new()
	instance.name = node_name
	instance.mesh = merged_mesh
	instance.material_override = material
	add_child(instance)

func _make_static_character() -> void:
	var root := Node3D.new()
	root.name = "StaticCharacter"
	root.position = Vector3(7.5, 0.0, 7.5)
	add_child(root)
	var parts_by_material: Dictionary = {"shirt": [], "skin": [], "pants": [], "dark": []}
	_add_capsule_part(parts_by_material["shirt"], 0.46, 1.05, Vector3(0.0, 1.25, 0.0))
	_add_sphere_part(parts_by_material["skin"], 0.34, Vector3(0.0, 2.05, 0.0))
	_add_capsule_part(parts_by_material["pants"], 0.16, 0.82, Vector3(-0.17, 0.48, 0.0))
	_add_capsule_part(parts_by_material["pants"], 0.16, 0.82, Vector3(0.17, 0.48, 0.0))
	_add_capsule_part(parts_by_material["skin"], 0.12, 0.78, Vector3(-0.48, 1.28, 0.0), Vector3(0.0, 0.0, -0.12))
	_add_capsule_part(parts_by_material["skin"], 0.12, 0.78, Vector3(0.48, 1.28, 0.0), Vector3(0.0, 0.0, 0.12))
	_add_box_part(parts_by_material["dark"], Vector3(0.28, 0.12, 0.48), Vector3(-0.17, 0.08, -0.08))
	_add_box_part(parts_by_material["dark"], Vector3(0.28, 0.12, 0.48), Vector3(0.17, 0.08, -0.08))
	_make_character_batch(root, "ShirtBatch", parts_by_material["shirt"], mat_shirt)
	_make_character_batch(root, "SkinBatch", parts_by_material["skin"], mat_skin)
	_make_character_batch(root, "PantsBatch", parts_by_material["pants"], mat_pants)
	_make_character_batch(root, "ShoesBatch", parts_by_material["dark"], mat_dark)

func _add_capsule_part(parts: Array, radius: float, height: float, position: Vector3, rotation: Vector3 = Vector3.ZERO) -> void:
	var mesh := CapsuleMesh.new()
	mesh.radius = radius
	mesh.height = height
	parts.append({"mesh": mesh, "position": position, "rotation": rotation})

func _add_sphere_part(parts: Array, radius: float, position: Vector3) -> void:
	var mesh := SphereMesh.new()
	mesh.radius = radius
	mesh.height = radius * 2.0
	parts.append({"mesh": mesh, "position": position, "rotation": Vector3.ZERO})

func _add_box_part(parts: Array, size: Vector3, position: Vector3) -> void:
	var mesh := BoxMesh.new()
	mesh.size = size
	parts.append({"mesh": mesh, "position": position, "rotation": Vector3.ZERO})

func _make_character_batch(parent: Node3D, node_name: String, parts: Array, material: Material) -> void:
	if parts.is_empty():
		return
	var surface := SurfaceTool.new()
	surface.begin(Mesh.PRIMITIVE_TRIANGLES)
	for part in parts:
		var basis := Basis.from_euler(part["rotation"])
		var transform := Transform3D(basis, part["position"])
		surface.append_from(part["mesh"], 0, transform)
	var merged_mesh := surface.commit()
	if merged_mesh == null:
		push_warning("No se pudo construir el personaje: " + node_name)
		return
	var instance := MeshInstance3D.new()
	instance.name = node_name
	instance.mesh = merged_mesh
	instance.material_override = material
	parent.add_child(instance)

func _make_camera() -> void:
	var camera := Camera3D.new()
	camera.name = "CityCamera"
	camera.position = Vector3(58.0, 62.0, 72.0)
	camera.fov = 55.0
	add_child(camera)
	camera.look_at(Vector3.ZERO, Vector3.UP)
	camera.current = true

func _make_sun() -> void:
	var sun := DirectionalLight3D.new()
	sun.name = "Sun"
	sun.rotation_degrees = Vector3(-52.0, -32.0, 0.0)
	sun.light_energy = 1.2
	sun.shadow_enabled = false
	add_child(sun)

func _make_title_label() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var label := Label.new()
	label.text = "GLOBAL CITY  |  PROTOTIPO 0.2"
	label.position = Vector2(24.0, 18.0)
	label.add_theme_font_size_override("font_size", 24)
	label.add_theme_color_override("font_color", Color("#ffffff"))
	label.add_theme_color_override("font_shadow_color", Color("#18232d"))
	label.add_theme_constant_override("shadow_offset_x", 2)
	label.add_theme_constant_override("shadow_offset_y", 2)
	layer.add_child(label)
