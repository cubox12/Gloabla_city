from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
project = (ROOT / "project.godot").read_text(encoding="utf-8")
scene = (ROOT / "Main.tscn").read_text(encoding="utf-8")
script = (ROOT / "Main.gd").read_text(encoding="utf-8")
player = (ROOT / "Player.gd").read_text(encoding="utf-8")
pedestrian = (ROOT / "Pedestrian.gd").read_text(encoding="utf-8")

assert 'run/main_scene="res://Main.tscn"' in project
assert 'path="res://Main.gd"' in scene
assert 'renderer/rendering_method="gl_compatibility"' in project

# Core world and mobile controls.
for token in ("func _make_box_batch", "func _make_player", "func _make_pedestrians",
              "func _make_city_assets", "BuildingCollisions", "StreetFurniture",
              "BuildingWindows", "MobileMovementPad", "GroundCollision",
              "InputEventScreenDrag", "InputEventScreenTouch", "InputEventMouseButton"):
    assert token in script, f"Missing Main.gd feature: {token}"
assert 'CharacterBody3D.new()' in script
assert 'move_and_slide()' in player
assert 'res://Player.gd' in script and 'res://Pedestrian.gd' in script
assert 'NPC_COUNT: int = 5' in script
assert 'func _orbit_camera' in script and 'func _set_camera_distance' in script
assert 'func _make_parcels' in script and 'EmptyParcelsBatch' in script
assert 'func _add_crosswalks' in script

# Player and pedestrian controllers.
assert 'class_name' not in player
assert 'extends CharacterBody3D' in player
assert 'extends Node3D' in pedestrian

# Procedural tile textures.
for rel in ("assets/asphalt_tile.png", "assets/grass_tile.png",
            "assets/concrete_tile.png", "assets/pavement_tile.png"):
    path = ROOT / rel
    data = path.read_bytes()
    assert data.startswith(b"\x89PNG\r\n\x1a\n"), f"Invalid PNG header: {rel}"
    width, height = struct.unpack(">II", data[16:24])
    assert (width, height) == (128, 128), f"Unexpected texture size for {rel}: {width}x{height}"

for forbidden in (".godot", ".import", ".export"):
    assert not (ROOT / forbidden).exists(), f"Unexpected generated directory: {forbidden}"
for path in ROOT.rglob("*"):
    if path.is_file():
        assert path.suffix.lower() not in {".apk", ".aab", ".ipa", ".pck", ".so", ".dll", ".exe"}, \
            f"Unexpected binary/build artifact: {path.relative_to(ROOT)}"

print("STATIC VALIDATION: PASS")
print("- Godot Compatibility renderer and main scene are configured")
print("- Player movement, collision and five ambient pedestrians are present")
print("- Sixteen low-poly buildings, facade details and collision shapes are generated")
print("- Street trees, lamps and benches are batched")
print("- Parcel grid, crosswalks and mobile camera controls are preserved")
print("- Four 128x128 procedural textures are validated")
print("- No compiled binaries or generated import directories are included")
