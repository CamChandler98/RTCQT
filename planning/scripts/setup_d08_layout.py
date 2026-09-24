"""Create or verify the D08 level in Unreal's editor Python environment.

Run with -EnablePlugins=PythonScriptPlugin -run=pythonscript and
-script="<project>/planning/scripts/setup_d08_layout.py --create".
Use --verify in a fresh editor process to check the saved level. Both modes
write a JSON report under Saved/Automation/D08. No licensed audio is needed.
"""

import argparse
import json
from pathlib import Path

import unreal

MAP = "/Game/Maps/L_D08Layout"
CAMERA_LOCATION = unreal.Vector(0, -3300, 1800)
CAMERA_ROTATION = unreal.MathLibrary.find_look_at_rotation(CAMERA_LOCATION, unreal.Vector(0, 0, 25))


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def vector(value):
    return [float(value.x), float(value.y), float(value.z)]


def create(levels, actors, assets):
    require(not assets.does_asset_exist(MAP), "Level already exists; use --verify to avoid overwriting it.")
    require(levels.new_level(MAP), "Could not create D08 level")
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    require(cube is not None, "Engine basic cube is missing")
    visualizer = actors.spawn_actor_from_class(unreal.RTCQTVisualizer, unreal.Vector(0, 0, 25))
    visualizer.set_actor_label("D08 - 32 Static Bands")
    settings = unreal.RTCQTLayoutSettings()
    settings.set_editor_property("base_scale", unreal.Vector(0.5, 0.5, 0.5))
    settings.set_editor_property("spacing", 100.0)
    visualizer.set_editor_property("layout_settings", settings)
    visualizer.set_editor_property("instance_material", cube.get_material(0))
    visualizer.set_editor_property("instance_mesh", cube)
    require(visualizer.rebuild_instances(), "Initial instance build failed")

    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -10))
    floor.set_actor_label("D08 - Ground")
    floor.static_mesh_component.set_static_mesh(cube)
    floor.set_actor_scale3d(unreal.Vector(38, 10, 0.2))

    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(pitch=-50, yaw=-40, roll=0))
    sun.set_actor_label("D08 - Key Light")
    sun.light_component.set_editor_property("intensity", 4.0)
    sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
    sky.set_actor_label("D08 - Fill Light")
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    sky.light_component.set_editor_property("cubemap", unreal.load_asset("/Engine/EngineMaterials/DefaultCubemap.DefaultCubemap"))
    sky.light_component.set_editor_property("intensity", 0.8)

    camera = actors.spawn_actor_from_class(unreal.CameraActor, CAMERA_LOCATION, CAMERA_ROTATION)
    camera.set_actor_label("D08 - Layout Camera")
    camera.camera_component.set_editor_property("field_of_view", 55.0)
    camera.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)
    actors.set_selected_level_actors([visualizer])
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(CAMERA_LOCATION, CAMERA_ROTATION)
    require(levels.save_current_level(), "Could not save D08 level")


def verify(levels, actors):
    require(levels.load_level(MAP), "Could not load saved D08 level")
    all_actors = actors.get_all_level_actors()
    visualizers = [a for a in all_actors if isinstance(a, unreal.RTCQTVisualizer)]
    require(len(visualizers) == 1, "Expected exactly one visualizer actor")
    visualizer = visualizers[0]
    components = visualizer.get_components_by_class(unreal.InstancedStaticMeshComponent)
    require(len(components) == 1, "Expected exactly one instanced mesh component")
    instances = components[0]
    require(instances.get_instance_count() == 32, "Saved level did not restore 32 instances")
    before = len(all_actors)
    transforms = []
    for index in range(32):
        transform = instances.get_instance_transform(index, world_space=False)
        location = vector(transform.translation)
        scale = vector(transform.scale3d)
        expected = [-1550.0 + index * 100.0, 0.0, 0.0]
        require(all(abs(a - b) < 1e-5 for a, b in zip(location, expected)), "Unexpected saved instance center")
        require(all(abs(v - 0.5) < 1e-5 for v in scale), "Unexpected saved instance scale")
        transforms.append({"index": index, "translation": location, "scale": scale})

    # set_editor_property issues the same change notifications as the Details panel.
    visualizer.set_editor_property("expected_band_count", 5)
    require(instances.get_instance_count() == 5, "Editor count change did not refresh automatically")
    settings = visualizer.get_editor_property("layout_settings")
    settings.set_editor_property("spacing", 140.0)
    visualizer.set_editor_property("layout_settings", settings)
    require(abs(instances.get_instance_transform(0, world_space=False).translation.x + 280) < 1e-5,
            "Editor spacing change did not refresh automatically")
    original_location = visualizer.get_actor_location()
    # The root's Details property works in commandlets too; typed-element gizmos
    # are only available in the interactive editor.
    instances.set_editor_property("relative_location", unreal.Vector(100, 200, 325))
    local = instances.get_instance_transform(0, world_space=False).translation
    world = instances.get_instance_transform(0, world_space=True).translation
    require(vector(local) == [-280.0, 0.0, 0.0], "Actor movement changed local layout")
    require(all(abs(a - b) < 1e-5 for a, b in zip(vector(world), [-180.0, 200.0, 325.0])),
            "Actor movement did not move the whole row: " + repr(vector(world))
            + "; actor=" + repr(vector(visualizer.get_actor_location()))
            + "; scale=" + repr(vector(visualizer.get_actor_scale3d())))
    instances.set_editor_property("relative_location", original_location)
    mesh = visualizer.get_editor_property("instance_mesh")
    visualizer.set_editor_property("instance_mesh", None)
    require(instances.get_instance_count() == 0, "Removing mesh left stale instances")
    visualizer.set_editor_property("instance_mesh", mesh)
    require(instances.get_instance_count() == 5, "Restoring mesh did not recover")
    visualizer.set_editor_property("expected_band_count", 0)
    require(instances.get_instance_count() == 0, "Zero count left stale instances")
    visualizer.set_editor_property("expected_band_count", 32)
    settings.set_editor_property("spacing", 100.0)
    visualizer.set_editor_property("layout_settings", settings)
    require(visualizer.rebuild_instances(), "Could not restore test scene")
    require(len(actors.get_all_level_actors()) == before, "Rebuilding spawned extra actors")
    require(levels.save_current_level(), "Could not save restored scene")
    actors.set_selected_level_actors([visualizer])
    return {
        "map": MAP, "engine_version": unreal.SystemLibrary.get_engine_version(),
        "visualizer_count": 1, "component_count": 1, "instance_count": instances.get_instance_count(),
        "component_name": instances.get_name(), "actor_count_before_and_after": before,
        "custom_data_floats": instances.get_editor_property("num_custom_data_floats"),
        "checks": ["saved level loads with 32 instances", "count edits refresh", "spacing edits refresh",
                   "actor movement preserves local layout", "missing mesh clears", "mesh restoration recovers",
                   "zero count clears", "rebuild does not spawn actors"],
        "instances": transforms,
    }


parser = argparse.ArgumentParser()
mode = parser.add_mutually_exclusive_group(required=True)
mode.add_argument("--create", action="store_true")
mode.add_argument("--verify", action="store_true")
args = parser.parse_args()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
if args.create:
    create(levels, actors, assets)
report = verify(levels, actors)
report["mode"] = "create" if args.create else "fresh_process_verify"
output = Path(unreal.Paths.project_saved_dir()) / "Automation" / "D08"
output.mkdir(parents=True, exist_ok=True)
(output / ("scene-created.json" if args.create else "scene-reopened.json")).write_text(json.dumps(report, indent=2) + "\n")
unreal.log("D08_SCENE_VERIFIED " + json.dumps({key: report[key] for key in ("mode", "visualizer_count", "component_count", "instance_count")}))
