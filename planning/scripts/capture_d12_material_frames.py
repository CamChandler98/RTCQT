"""Verify the saved D12 scene in fresh Play mode, then capture explicit editor frames.

Run in the full editor with PythonScriptPlugin enabled. Requires a graphics
device. Does not save or modify the material, settings asset, or level on disk.
"""

from pathlib import Path
import json
import time
import traceback
import unreal


LEVEL = "/Game/Audio/PrivateTest/L_D10D11ResponseTest"
MATERIAL = "/Game/Materials/M_RTCQT_Bands.M_RTCQT_Bands"
TIMES = [16.75, 0.75, 2.0, 33.25, 16.75]
LABELS = ["strong-first", "quiet", "moderate", "bright", "strong-repeat", "controlled-endpoints"]
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(LEVEL)
scene = actors.get_all_level_actors()
visualizers = [actor for actor in scene if isinstance(actor, unreal.RTCQTVisualizer)]
assert len(visualizers) == 1
editor_actor = visualizers[0]
settings = editor_actor.get_editor_property("response_settings")
assert settings.get_editor_property("response_exponent") == 2.0
assert editor_actor.get_editor_property("instance_material").get_path_name() == MATERIAL
camera = next(actor for actor in scene if isinstance(actor, unreal.CameraActor))
camera_component = camera.get_component_by_class(unreal.CameraComponent)
if camera_component.get_editor_property("post_process_blend_weight") > 0:
    camera_post = camera_component.get_editor_property("post_process_settings")
    for flag in ["override_auto_exposure_method", "override_auto_exposure_min_brightness",
                 "override_auto_exposure_max_brightness", "override_auto_exposure_bias",
                 "override_auto_exposure_bias_curve", "override_auto_exposure_apply_physical_camera_exposure"]:
        assert not camera_post.get_editor_property(flag), flag
volumes = [actor for actor in scene if isinstance(actor, unreal.PostProcessVolume)]
assert len(volumes) == 1
volume = volumes[0]
post = volume.get_editor_property("settings")
assert volume.get_editor_property("unbound") and volume.get_editor_property("enabled")
assert volume.get_editor_property("blend_weight") == 1.0
assert post.get_editor_property("override_auto_exposure_min_brightness")
assert post.get_editor_property("override_auto_exposure_max_brightness")
minimum = float(post.get_editor_property("auto_exposure_min_brightness"))
maximum = float(post.get_editor_property("auto_exposure_max_brightness"))
assert minimum == maximum
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
    camera.get_actor_location(), camera.get_actor_rotation())
actors.set_selected_level_actors([])
directory = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D12"
directory.mkdir(parents=True, exist_ok=True)
previous_mtimes = {label: (directory / ("frame-" + label + ".png")).stat().st_mtime_ns
                   if (directory / ("frame-" + label + ".png")).exists() else 0 for label in LABELS}
report = {"scene": LEVEL, "material": MATERIAL, "response_exponent": 2.0,
          "min_ev100": minimum, "max_ev100": maximum,
          "exposure_compensation": float(post.get_editor_property("auto_exposure_bias")),
          "bloom_intensity": float(post.get_editor_property("bloom_intensity")),
          "global_eye_adaptation_disabled": True, "files_saved": False,
          "play_frames": [], "editor_frames": [], "stills": []}
started = time.monotonic()
phase = "play_start"
phase_started = started
task = None
capture_index = 0


def frame(actor, song_time):
    result = actor.evaluate_at_time(song_time)
    assert result.get_editor_property("succeeded")
    assert result.get_editor_property("failure_reason") == unreal.RTCQTFailureReason.NONE
    assert result.get_editor_property("observed_band_count") == 32
    components = actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    assert len(components) == 1 and components[0].get_instance_count() == 32
    component = components[0]
    material = component.get_material(0)
    assert isinstance(material, unreal.MaterialInstanceDynamic)
    assert material.get_outer() == actor
    assert material.get_editor_property("parent").get_path_name() == MATERIAL
    assert material.get_scalar_parameter_value("EmissiveMin") == 0.0
    assert material.get_scalar_parameter_value("EmissiveMax") == 10.0
    data = [float(value) for value in component.get_editor_property("per_instance_sm_custom_data")]
    assert len(data) == 64
    transforms = []
    for index in range(32):
        transform = component.get_instance_transform(index, world_space=False)
        transforms.append({"band": index,
                           "translation": [float(transform.translation.x), float(transform.translation.y), float(transform.translation.z)],
                           "scale": [float(transform.scale3d.x), float(transform.scale3d.y), float(transform.scale3d.z)]})
    return {"time_seconds": song_time, "transforms": transforms, "custom_data": data,
            "shared_material": material.get_path_name(), "emissive_range": [0.0, 10.0]}, material


def finish(error=None):
    if error:
        unreal.log_error("D12_CAPTURE_FAILED " + str(error))
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()


def tick(delta_seconds):
    global phase, phase_started, task, capture_index
    try:
        now = time.monotonic()
        if now - started > 300:
            raise RuntimeError("D12 capture timed out")
        if phase == "play_start" and now - started > 5:
            levels.editor_request_begin_play()
            phase, phase_started = "play_check", now
        elif phase == "play_check" and levels.is_in_play_in_editor() and now - phase_started > 3:
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            playing = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RTCQTVisualizer)
            assert len(playing) == 1
            first_material = None
            for song_time in TIMES:
                values, material = frame(playing[0], song_time)
                if first_material is None:
                    first_material = material
                assert material == first_material
                report["play_frames"].append(values)
            assert report["play_frames"][0] == report["play_frames"][-1]
            report["play_in_editor_verified"] = True
            report["active_shared_material_count"] = 1
            report["visualizer_count"] = 1
            report["instance_count"] = 32
            unreal.log("D12_FRESH_PLAY_VERIFIED one_actor=1 instances=32 shared_materials=1")
            levels.editor_request_end_play()
            phase, phase_started = "evaluate", now
        elif phase == "evaluate" and not levels.is_in_play_in_editor() and now - phase_started > 3:
            if capture_index < len(TIMES):
                values, material = frame(editor_actor, TIMES[capture_index])
                played = report["play_frames"][capture_index]
                assert values["transforms"] == played["transforms"]
                assert values["custom_data"] == played["custom_data"]
                report["editor_frames"].append(values)
            else:
                # Controlled shader-endpoint fixture; explicitly not a song-time frame.
                component = editor_actor.get_components_by_class(unreal.InstancedStaticMeshComponent)[0]
                for index in range(32):
                    response = [0.0, 0.5, 1.0][min(2, index // 11)]
                    transform = component.get_instance_transform(index, world_space=False)
                    transform.scale3d = unreal.Vector(0.5, 0.5, 0.1 + 3.9 * response)
                    assert component.update_instance_transform(index, transform, world_space=False, mark_render_state_dirty=True, teleport=True)
                    assert component.set_custom_data_value(index, 0, response, True)
                report["controlled_endpoints"] = {"response": [0, 0.5, 1], "emissive": [0, 5, 10], "song_data": False}
            phase, phase_started = "settle", now
        elif phase == "settle" and now - phase_started > 5:
            output = directory / ("frame-" + LABELS[capture_index] + ".png")
            task = unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, str(output), camera=camera, delay=2.0)
            assert task and task.is_valid_task()
            phase = "capture"
        elif phase == "capture" and task is not None and task.is_task_done():
            output = directory / ("frame-" + LABELS[capture_index] + ".png")
            assert output.is_file() and output.stat().st_size > 1000
            assert output.stat().st_mtime_ns > previous_mtimes[LABELS[capture_index]], "Screenshot was not refreshed"
            report["stills"].append(output.name)
            unreal.log("D12_FRAME_SAVED " + output.name)
            capture_index += 1
            task = None
            if capture_index == len(LABELS):
                (directory / "fresh-material-frames.json").write_text(json.dumps(report, indent=2) + "\n")
                unreal.log("D12_FRESH_SCENE_AND_FRAMES_VERIFIED")
                finish()
            else:
                phase, phase_started = "evaluate", now
    except Exception as error:
        finish(traceback.format_exc())


callback = unreal.register_slate_post_tick_callback(tick)
