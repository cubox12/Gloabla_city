extends Node3D

@export var orbit_radius := 4.0
@export var orbit_speed := 0.22
@export var phase := 0.0
var center := Vector3.ZERO
var elapsed := 0.0

func _ready() -> void:
	center = position

func _process(delta: float) -> void:
	elapsed += delta
	var angle := phase + elapsed * orbit_speed
	position = center + Vector3(cos(angle) * orbit_radius, 0.0, sin(angle) * orbit_radius)
	rotation.y = -angle
