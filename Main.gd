extends Node3D

# GLOBAL CITY 0.6 — first low-poly buildings and street furniture, mobile-first.

const WORLD_SIZE: float = 140.0
const ROAD_WIDTH: float = 10.0
const ROAD_SPACING: float = 34.0
const SIDEWALK_WIDTH: float = 2.2
const SIDEWALK_OFFSET: float = ROAD_WIDTH * 0.5 + 1.4
const CAMERA_MIN_DISTANCE: float = 18.0
const CAMERA_MAX_DISTANCE: float = 75.0
const CAMERA_DRAG_SENSITIVITY: float = 0.0045
const NPC_COUNT: int = 5
const PlayerController = preload("res://Player.gd")
const PedestrianController = preload("res://Pedestrian.gd")

var mat_ground: StandardMaterial3D
var mat_road: StandardMaterial3D
var mat_sidewalk: StandardMaterial3D
var mat_line: StandardMaterial3D
var mat_parcel: StandardMaterial3D
var mat_parcel_edge: StandardMaterial3D
var mat_npc: StandardMaterial3D
var mat_concrete: StandardMaterial3D
var mat_building_a: StandardMaterial3D
var mat_building_b: StandardMaterial3D
var mat_building_c: StandardMaterial3D
var mat_windows: StandardMaterial3D
var mat_roof: StandardMaterial3D
var mat_furniture: StandardMaterial3D
var player: CharacterBody3D
var pedestrians: Array[Node3D] = []
var mat_skin: StandardMaterial3D
var mat_shirt: StandardMaterial3D
var mat_pants: StandardMaterial3D
var mat_dark: StandardMaterial3D

var camera_pivot: Node3D
var city_camera: Camera3D
var touch_points: Dictionary = {}
var previous_pinch_distance: float = 0.0
var mouse_dragging: bool = false

func _ready() -> void:
	_make_materials()
	_setup_input_actions()
	_make_environment()
	_make_ground()
	_make_roads()
	_make_parcels()
	_make_city_assets()
	_make_player()
	_make_pedestrians()
	_make_camera()
	_make_sun()
	_make_hud()

func _make_materials() -> void:
	mat_ground = _make_material(Color("#ffffff"), "res://assets/grass_tile.png", Vector3(28.0, 28.0, 1.0))
	mat_road = _make_material(Color("#ffffff"), "res://assets/asphalt_tile.png", Vector3(8.0, 8.0, 1.0))
	mat_sidewalk = _make_material(Color("#b9bbb5"))
	mat_line = _make_material(Color("#f2e9c9"))
	mat_parcel = _make_material(Color("#9db783"))
	mat_parcel_edge = _make_material(Color("#718d61"))
	mat_npc = _make_material(Color("#b56a45"))
	mat_concrete = _make_material(Color("#ffffff"), "res://assets/concrete_tile.png", Vector3(5.0, 5.0, 1.0))
	mat_skin = _make_material(Color("#d9a078"))
	mat_shirt = _make_material(Color("#2676a8"))
	mat_pants = _make_material(Color("#263448"))
	mat_dark = _make_material(Color("#20242a"))
	mat_building_a = _make_material(Color("#d7c5a6"))
	mat_building_b = _make_material(Color("#b9c9cb"))
	mat_building_c = _make_material(Color("#c58f70"))
	mat_windows = _make_material(Color("#547d91"))
	mat_roof = _make_material(Color("#777b78"))
	mat_furniture = _make_material(Color("#454e50"))

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
	environment.ambient_light_energy = 0.82
	world.environment = environment
	add_child(world)

func _make_ground() -> void:
	var ground_boxes: Array[Dictionary] = []
	_add_box_spec(ground_boxes, Vector3(WORLD_SIZE, 1.0, WORLD_SIZE), Vector3(0.0, -0.55, 0.0))
	_make_box_batch("GroundBatch", ground_boxes, mat_ground)
	var floor_body := StaticBody3D.new()
	floor_body.name = "GroundCollision"
	floor_body.position = Vector3(0.0, -0.56, 0.0)
	var floor_shape := CollisionShape3D.new()
	var box_shape := BoxShape3D.new()
	box_shape.size = Vector3(WORLD_SIZE, 0.1, WORLD_SIZE)
	floor_shape.shape = box_shape
	floor_body.add_child(floor_shape)
	add_child(floor_body)

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
	_add_crosswalks(marking_boxes)
	_make_box_batch("RoadBatch", road_boxes, mat_road)
	_make_box_batch("SidewalkBatch", sidewalk_boxes, mat_concrete)
	_make_box_batch("RoadMarkingsBatch", marking_boxes, mat_line)

func _add_crosswalks(target: Array[Dictionary]) -> void:
	# Crosswalk bars are part of the existing marking batch: no extra scene nodes.
	for road_x in range(-2, 3):
		for road_z in range(-2, 3):
			var x := float(road_x) * ROAD_SPACING
			var z := float(road_z) * ROAD_SPACING
			for stripe in range(6):
				var offset := (float(stripe) - 2.5) * 0.95
				_add_box_spec(target, Vector3(8.0, 0.028, 0.38), Vector3(x, 0.112, z + offset))
				_add_box_spec(target, Vector3(0.38, 0.028, 8.0), Vector3(x + offset, 0.112, z))

func _make_parcels() -> void:
	var plot_boxes: Array[Dictionary] = []
	var edge_boxes: Array[Dictionary] = []
	var plot_width := ROAD_SPACING - (SIDEWALK_OFFSET * 2.0) - 0.8
	for x_index in range(-2, 2):
		for z_index in range(-2, 2):
			var center := Vector3(
				(float(x_index) + 0.5) * ROAD_SPACING,
				-0.025,
				(float(z_index) + 0.5) * ROAD_SPACING
			)
			_add_box_spec(plot_boxes, Vector3(plot_width, 0.025, plot_width), center)
			var half := plot_width * 0.5
			var border := 0.16
			_add_box_spec(edge_boxes, Vector3(plot_width + border, 0.025, border), Vector3(center.x, 0.0, center.z - half))
			_add_box_spec(edge_boxes, Vector3(plot_width + border, 0.025, border), Vector3(center.x, 0.0, center.z + half))
			_add_box_spec(edge_boxes, Vector3(border, 0.025, plot_width + border), Vector3(center.x - half, 0.0, center.z))
			_add_box_spec(edge_boxes, Vector3(border, 0.025, plot_width + border), Vector3(center.x + half, 0.0, center.z))
	_make_box_batch("EmptyParcelsBatch", plot_boxes, mat_parcel)
	_make_box_batch("ParcelBordersBatch", edge_boxes, mat_parcel_edge)


func _make_city_assets() -> void:
	# Low-poly buildings are merged by material to keep draw calls low on Android.
	var facades_a: Array[Dictionary] = []
	var facades_b: Array[Dictionary] = []
	var facades_c: Array[Dictionary] = []
	var roofs: Array[Dictionary] = []
	var windows: Array[Dictionary] = []
	var doors: Array[Dictionary] = []
	var furniture: Array[Dictionary] = []
	var trees: Array[Dictionary] = []
	var lot_index := 0
	for x_index in range(-2, 2):
		for z_index in range(-2, 2):
			var center := Vector3((float(x_index) + 0.5) * ROAD_SPACING, 0.0, (float(z_index) + 0.5) * ROAD_SPACING)
			var width := 11.5 + float((lot_index * 3) % 4) * 1.2
			var depth := 11.0 + float((lot_index * 5) % 3) * 1.3
			var height := 7.5 + float((lot_index * 7) % 5) * 1.45
			var facade := facades_a
			if lot_index % 3 == 1:
				facade = facades_b
			elif lot_index % 3 == 2:
				facade = facades_c
			_add_box_spec(facade, Vector3(width, height, depth), center + Vector3(0.0, height * 0.5, 0.0))
			_add_box_spec(roofs, Vector3(width + 0.35, 0.28, depth + 0.35), center + Vector3(0.0, height + 0.14, 0.0))
			# Repeated front windows and a ground-floor entrance.
			var columns := 3
			for floor_index in range(maxi(1, int(height / 3.0))):
				var y := 2.0 + float(floor_index) * 2.5
				if y > height - 1.0:
					continue
				for column in range(columns):
					var x := center.x + (float(column) - 1.0) * (width * 0.27)
					_add_box_spec(windows, Vector3(1.15, 1.25, 0.10), Vector3(x, y, center.z - depth * 0.5 - 0.06))
			_add_box_spec(doors, Vector3(1.35, 2.1, 0.12), Vector3(center.x, 1.05, center.z - depth * 0.5 - 0.07))
			lot_index += 1
	# Small trees, benches and lamp posts are also batched instead of spawning many nodes.
	for side in [-1.0, 1.0]:
		for segment in range(-2, 3):
			var z := float(segment) * ROAD_SPACING + 10.0
			var x := side * (ROAD_WIDTH * 0.5 + 3.0)
			_add_box_spec(trees, Vector3(0.45, 2.0, 0.45), Vector3(x, 1.0, z))
			_add_box_spec(trees, Vector3(1.8, 1.8, 1.8), Vector3(x, 2.65, z))
			var lamp_x := side * (ROAD_WIDTH * 0.5 + 1.5)
			_add_box_spec(furniture, Vector3(0.16, 4.2, 0.16), Vector3(lamp_x, 2.1, z + 7.0))
			_add_box_spec(furniture, Vector3(0.75, 0.16, 0.75), Vector3(lamp_x, 4.2, z + 7.0))
			_add_box_spec(furniture, Vector3(1.8, 0.18, 0.55), Vector3(lamp_x + side * 1.2, 0.55, z - 5.0))
			_add_box_spec(furniture, Vector3(1.8, 0.65, 0.14), Vector3(lamp_x + side * 1.2, 0.9, z - 5.25))
	_make_box_batch("BuildingFacadeA", facades_a, mat_building_a)
	_make_box_batch("BuildingFacadeB", facades_b, mat_building_b)
	_make_box_batch("BuildingFacadeC", facades_c, mat_building_c)
	_make_box_batch("BuildingRoofs", roofs, mat_roof)
	_make_box_batch("BuildingWindows", windows, mat_windows)
	_make_box_batch("BuildingEntrances", doors, mat_furniture)
	_make_box_batch("StreetTrees", trees, mat_parcel_edge)
	_make_box_batch("StreetFurniture", furniture, mat_furniture)

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

func _setup_input_actions() -> void:
	var keys := {
		"move_forward": [KEY_W, KEY_UP],
		"move_back": [KEY_S, KEY_DOWN],
		"move_left": [KEY_A, KEY_LEFT],
		"move_right": [KEY_D, KEY_RIGHT]
	}
	for action in keys:
		if not InputMap.has_action(action):
			InputMap.add_action(action)
		for keycode in keys[action]:
			var event := InputEventKey.new()
			event.physical_keycode = keycode
			InputMap.action_add_event(action, event)

func _make_player() -> void:
	player = CharacterBody3D.new()
	player.name = "Player"
	player.set_script(PlayerController)
	player.position = Vector3(7.5, 0.1, 7.5)
	var capsule := CapsuleShape3D.new()
	capsule.radius = 0.36
	capsule.height = 1.8
	var collision := CollisionShape3D.new()
	collision.shape = capsule
	collision.position.y = 0.9
	player.add_child(collision)
	var body := MeshInstance3D.new()
	var body_mesh := CapsuleMesh.new()
	body_mesh.radius = 0.36
	body_mesh.height = 1.15
	body.mesh = body_mesh
	body.position.y = 1.12
	body.material_override = mat_shirt
	player.add_child(body)
	var head := MeshInstance3D.new()
	var head_mesh := SphereMesh.new()
	head_mesh.radius = 0.25
	head_mesh.height = 0.5
	head.mesh = head_mesh
	head.position.y = 1.88
	head.material_override = mat_skin
	player.add_child(head)
	for side in [-1.0, 1.0]:
		var leg := MeshInstance3D.new()
		var leg_mesh := CapsuleMesh.new()
		leg_mesh.radius = 0.13
		leg_mesh.height = 0.72
		leg.mesh = leg_mesh
		leg.position = Vector3(side * 0.16, 0.38, 0.0)
		leg.material_override = mat_pants
		player.add_child(leg)
	add_child(player)

func _make_pedestrians() -> void:
	for i in range(NPC_COUNT):
		var npc := Node3D.new()
		npc.name = "Pedestrian_%02d" % (i + 1)
		npc.set_script(PedestrianController)
		npc.set("phase", float(i) * TAU / float(NPC_COUNT))
		npc.set("orbit_radius", 4.0 + float(i % 2) * 2.0)
		npc.position = Vector3(cos(float(i) * TAU / float(NPC_COUNT)) * 25.0, 0.0, sin(float(i) * TAU / float(NPC_COUNT)) * 25.0)
		var torso := MeshInstance3D.new()
		var torso_mesh := CapsuleMesh.new()
		torso_mesh.radius = 0.28
		torso_mesh.height = 0.9
		torso.mesh = torso_mesh
		torso.position.y = 0.95
		torso.material_override = mat_npc if i % 2 == 0 else mat_shirt
		npc.add_child(torso)
		var head := MeshInstance3D.new()
		var head_mesh := SphereMesh.new()
		head_mesh.radius = 0.2
		head_mesh.height = 0.4
		head.mesh = head_mesh
		head.position.y = 1.62
		head.material_override = mat_skin
		npc.add_child(head)
		add_child(npc)
		pedestrians.append(npc)

func _process(_delta: float) -> void:
	if player != null and camera_pivot != null:
		camera_pivot.global_position = player.global_position
	for npc in pedestrians:
		pass

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
	camera_pivot = Node3D.new()
	camera_pivot.name = "CameraOrbitPivot"
	camera_pivot.position = player.global_position if player != null else Vector3.ZERO
	add_child(camera_pivot)
	city_camera = Camera3D.new()
	city_camera.name = "CityCamera"
	city_camera.position = Vector3(0.0, 24.0, 28.0)
	city_camera.fov = 55.0
	camera_pivot.add_child(city_camera)
	city_camera.look_at(Vector3.ZERO, Vector3.UP)
	city_camera.current = true
	if player != null:
		player.set("camera_pivot", camera_pivot)

func _input(event: InputEvent) -> void:
	if event is InputEventScreenTouch:
		if event.pressed:
			touch_points[event.index] = event.position
			if touch_points.size() == 2:
				previous_pinch_distance = _get_pinch_distance()
		else:
			touch_points.erase(event.index)
			if touch_points.size() < 2:
				previous_pinch_distance = 0.0
	elif event is InputEventScreenDrag:
		if not touch_points.has(event.index):
			return
		touch_points[event.index] = event.position
		if touch_points.size() == 1:
			_orbit_camera(event.relative)
		elif touch_points.size() >= 2:
			var current_distance := _get_pinch_distance()
			if previous_pinch_distance > 0.0 and current_distance > 0.0:
				var zoom_ratio := previous_pinch_distance / current_distance
				_set_camera_distance(city_camera.position.length() * zoom_ratio)
			previous_pinch_distance = current_distance
	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT:
			mouse_dragging = event.pressed
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_set_camera_distance(city_camera.position.length() * 0.90)
		elif event.pressed and event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_set_camera_distance(city_camera.position.length() * 1.10)
	elif event is InputEventMouseMotion and mouse_dragging:
		_orbit_camera(event.relative)

func _get_pinch_distance() -> float:
	if touch_points.size() < 2:
		return 0.0
	var points: Array = touch_points.values()
	return points[0].distance_to(points[1])

func _orbit_camera(delta: Vector2) -> void:
	camera_pivot.rotation.y -= delta.x * CAMERA_DRAG_SENSITIVITY
	camera_pivot.rotation.x = clampf(
		camera_pivot.rotation.x - delta.y * CAMERA_DRAG_SENSITIVITY,
		-0.22,
		0.38
	)

func _set_camera_distance(distance: float) -> void:
	var direction := city_camera.position.normalized()
	var clamped_distance := clampf(distance, CAMERA_MIN_DISTANCE, CAMERA_MAX_DISTANCE)
	city_camera.position = direction * clamped_distance

func _make_sun() -> void:
	var sun := DirectionalLight3D.new()
	sun.name = "Sun"
	sun.rotation_degrees = Vector3(-52.0, -32.0, 0.0)
	sun.light_energy = 1.2
	sun.shadow_enabled = false
	add_child(sun)

func _make_hud() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var title := Label.new()
	title.text = "GLOBAL CITY  |  PROTOTIPO 0.6"
	title.position = Vector2(24.0, 18.0)
	title.add_theme_font_size_override("font_size", 24)
	title.add_theme_color_override("font_color", Color("#ffffff"))
	title.add_theme_color_override("font_shadow_color", Color("#18232d"))
	title.add_theme_constant_override("shadow_offset_x", 2)
	title.add_theme_constant_override("shadow_offset_y", 2)
	layer.add_child(title)

	var info := Label.new()
	info.text = "16 EDIFICIOS  •  5 PEATONES  •  CIUDAD EN CONSTRUCCIÓN"
	info.position = Vector2(24.0, 52.0)
	info.add_theme_font_size_override("font_size", 15)
	info.add_theme_color_override("font_color", Color("#f3f5e9"))
	info.add_theme_color_override("font_shadow_color", Color("#18232d"))
	info.add_theme_constant_override("shadow_offset_x", 2)
	info.add_theme_constant_override("shadow_offset_y", 2)
	layer.add_child(info)

	var controls := Label.new()
	controls.text = "WASD / FLECHAS: MOVER   •   ARRASTRA: CÁMARA   •   PINZA: ZOOM"
	controls.anchor_left = 0.0
	controls.anchor_top = 1.0
	controls.anchor_right = 1.0
	controls.anchor_bottom = 1.0
	controls.offset_left = 24.0
	controls.offset_top = -48.0
	controls.offset_right = -24.0
	controls.offset_bottom = -18.0
	controls.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	controls.add_theme_font_size_override("font_size", 16)
	controls.add_theme_color_override("font_color", Color("#ffffff"))
	controls.add_theme_color_override("font_shadow_color", Color("#18232d"))
	controls.add_theme_constant_override("shadow_offset_x", 2)
	controls.add_theme_constant_override("shadow_offset_y", 2)
	layer.add_child(controls)
	_make_mobile_controls(layer)

func _make_mobile_controls(layer: CanvasLayer) -> void:
	var panel := GridContainer.new()
	panel.name = "MobileMovementPad"
	panel.columns = 3
	panel.anchor_left = 1.0
	panel.anchor_top = 1.0
	panel.anchor_right = 1.0
	panel.anchor_bottom = 1.0
	panel.offset_left = -214.0
	panel.offset_top = -198.0
	panel.offset_right = -18.0
	panel.offset_bottom = -18.0
	panel.add_theme_constant_override("h_separation", 5)
	panel.add_theme_constant_override("v_separation", 5)
	layer.add_child(panel)
	var layout := [["", "", ""], ["move_left", "move_forward", "move_right"], ["", "move_back", ""]]
	for row in layout:
		for action in row:
			var button := Button.new()
			button.custom_minimum_size = Vector2(60.0, 54.0)
			if action == "":
				button.visible = false
			else:
				button.text = {"move_left": "◀", "move_forward": "▲", "move_right": "▶", "move_back": "▼"}[action]
				button.add_theme_font_size_override("font_size", 22)
				button.button_down.connect(_on_move_button_down.bind(action))
				button.button_up.connect(_on_move_button_up.bind(action))
			panel.add_child(button)

func _on_move_button_down(action: String) -> void:
	Input.action_press(action)

func _on_move_button_up(action: String) -> void:
	Input.action_release(action)
