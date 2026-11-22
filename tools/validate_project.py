from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
project = (ROOT / "project.godot").read_text(encoding="utf-8")
scene = (ROOT / "Main.tscn").read_text(encoding="utf-8")
script = (ROOT / "Main.gd").read_text(encoding="utf-8")

assert 'run/main_scene="res://Main.tscn"' in project
assert 'path="res://Main.gd"' in scene
assert 'renderer/rendering_method="gl_compatibility"' in project
assert 'func _make_box_batch' in script
assert 'func _make_character_batch' in script
assert script.count('surface.append_from(') >= 2
assert 'Input.' not in script and 'CharacterBody3D' not in script
assert 'func _make_static_character' in script

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
print("- Main scene and script references exist")
print("- Compatibility renderer is configured")
print("- Static geometry batching is present")
print("- Character has no movement/controller code")
print("- Both local textures are valid 128x128 PNG files")
print("- No compiled binaries or generated import directories are included")
