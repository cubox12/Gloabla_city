extends CharacterBody3D

@export var move_speed := 7.0
@export var acceleration := 22.0
@export var gravity := 20.0
var camera_pivot: Node3D

func _physics_process(delta: float) -> void:
	var input_vector := Vector2(Input.get_action_strength("move_right") - Input.get_action_strength("move_left"), Input.get_action_strength("move_back") - Input.get_action_strength("move_forward"))
	if input_vector.length() > 1.0:
		input_vector = input_vector.normalized()
	var forward := -camera_pivot.global_basis.z if camera_pivot != null else Vector3.FORWARD
	var right := camera_pivot.global_basis.x if camera_pivot != null else Vector3.RIGHT
	forward.y = 0.0
	right.y = 0.0
	var direction := (right.normalized() * input_vector.x + forward.normalized() * input_vector.y).normalized()
	var target := direction * move_speed
	velocity.x = move_toward(velocity.x, target.x, acceleration * delta)
	velocity.z = move_toward(velocity.z, target.z, acceleration * delta)
	if not is_on_floor():
		velocity.y -= gravity * delta
	else:
		velocity.y = 0.0
	move_and_slide()
	if direction.length_squared() > 0.01:
		rotation.y = atan2(direction.x, direction.z)
