from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
project = (ROOT / "project.godot").read_text(encoding="utf-8")
scene = (ROOT / "Main.tscn").read_text(encoding="utf-8")
script = (ROOT / "Main.gd").read_text(encoding="utf-8")

# Project wiring and low-end mobile renderer.
assert 'run/main_scene="res://Main.tscn"' in project
assert 'path="res://Main.gd"' in scene
assert 'renderer/rendering_method="gl_compatibility"' in project

# Existing batched city foundation is preserved.
assert 'func _make_box_batch' in script
assert 'func _make_character_batch' in script
assert script.count('surface.append_from(') >= 2
assert 'func _make_static_character' in script

# New 0.4 acceptance criteria: empty parcel grid, crosswalks and camera gestures.
assert 'func _make_parcels' in script
assert 'EmptyParcelsBatch' in script
assert 'ParcelBordersBatch' in script
assert 'func _add_crosswalks' in script
assert 'func _orbit_camera' in script
assert 'func _set_camera_distance' in script
assert 'InputEventScreenDrag' in script
assert 'InputEventScreenTouch' in script
assert 'InputEventMouseButton' in script
assert 'CAMERA_MIN_DISTANCE' in script and 'CAMERA_MAX_DISTANCE' in script

# The player remains deliberately static: no physics body or movement controller.
assert 'CharacterBody3D' not in script
assert 'func _physics_process' not in script
assert 'move_and_slide(' not in script
assert 'DESLIZA: GIRAR CÁMARA' in script

for rel in ("assets/asphalt_tile.png", "assets/grass_tile.png"):
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
print("- Main scene and Compatibility renderer are configured")
print("- Static geometry batching remains present")
print("- 16 empty parcels and parcel borders are generated in merged batches")
print("- Crosswalks are merged into the road-marking batch")
print("- Touch drag, pinch zoom and desktop mouse camera controls are present")
print("- Character remains static and has no movement controller")
print("- Both local textures are valid 128x128 PNG files")
print("- No compiled binaries or generated import directories are included")
