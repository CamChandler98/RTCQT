"""Verify the saved D11 scene in fresh Play mode and capture its restored frame.

Run the full editor with -EnablePlugins=PythonScriptPlugin and
-ExecutePythonScript=<this-file>. Requires a working graphics device.
No level or response-settings asset is modified or saved by this script.
"""

from pathlib import Path
import json
import time
import unreal


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level("/Game/Audio/PrivateTest/L_D10D11ResponseTest")
scene = actors.get_all_level_actors()
visualizers = [actor for actor in scene if isinstance(actor, unreal.RTCQTVisualizer)]
assert len(visualizers) == 1
editor_actor = visualizers[0]
settings = editor_actor.get_editor_property("response_settings")
assert settings is not None and float(settings.get_editor_property("response_exponent")) == 2.0
camera = next(actor for actor in scene if isinstance(actor, unreal.CameraActor))
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
    camera.get_actor_location(), camera.get_actor_rotation())
actors.set_selected_level_actors([])
directory = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D11"
directory.mkdir(parents=True, exist_ok=True)
output = directory / "saved-frame-restored.png"
previous = output.stat().st_mtime_ns if output.exists() else None
report = {"scene": "/Game/Audio/PrivateTest/L_D10D11ResponseTest", "response_exponent": 2.0,
          "evaluated_time_seconds": 16.75, "level_or_settings_saved": False}
started = time.monotonic()
phase = "play_start"
phase_started = started
task = None


def frame(actor):
    result = actor.evaluate_at_time(16.75)
    assert result.get_editor_property("succeeded")
    assert result.get_editor_property("failure_reason") == unreal.RTCQTFailureReason.NONE
    assert result.get_editor_property("evaluated_time_seconds") == 16.75
    assert result.get_editor_property("observed_band_count") == 32
    components = actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    assert len(components) == 1 and components[0].get_instance_count() == 32
    transforms = []
    for index in range(32):
        transform = components[0].get_instance_transform(index, world_space=False)
        transforms.append({"band": index,
                           "translation": [float(transform.translation.x), float(transform.translation.y), float(transform.translation.z)],
                           "scale": [float(transform.scale3d.x), float(transform.scale3d.y), float(transform.scale3d.z)]})
    return transforms


def finish(error=None):
    if error:
        unreal.log_error("D11_CAPTURE_FAILED " + str(error))
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()


def tick(delta_seconds):
    global phase, phase_started, task
    try:
        elapsed = time.monotonic()
        if elapsed - started > 180:
            raise RuntimeError("D11 saved-frame capture timed out")
        if phase == "play_start" and elapsed - started > 5:
            levels.editor_request_begin_play()
            phase, phase_started = "play_check", elapsed
        elif phase == "play_check" and levels.is_in_play_in_editor() and elapsed - phase_started > 3:
            world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            playing = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RTCQTVisualizer)
            assert len(playing) == 1
            report["play_transforms"] = frame(playing[0])
            report["play_in_editor_verified"] = True
            report["visualizer_count"] = 1
            report["instance_count"] = 32
            report["succeeded"] = True
            report["failure_reason"] = "None"
            unreal.log("D11_FRESH_PLAY_VERIFIED time=16.75 bands=32 exponent=2")
            levels.editor_request_end_play()
            phase, phase_started = "capture", elapsed
        elif phase == "capture" and not levels.is_in_play_in_editor() and task is None and elapsed - phase_started > 5:
            # Editor instances revert after Play; evaluate the same isolated frame
            # for the viewport capture and require equality with the played frame.
            report["editor_capture_transforms"] = frame(editor_actor)
            assert report["editor_capture_transforms"] == report["play_transforms"]
            report["capture_phase"] = "Editor viewport after evaluating the same verified 16.75 frame"
            task = unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, str(output), camera=camera, delay=2.0)
            assert task and task.is_valid_task()
        elif task is not None and task.is_task_done():
            assert output.is_file() and (previous is None or output.stat().st_mtime_ns > previous)
            report["screenshot_written"] = True
            (directory / "saved-frame-restored.json").write_text(json.dumps(report, indent=2) + "\n")
            unreal.log("D11_FRESH_CAPTURE_SAVED " + str(output))
            finish()
    except Exception as error:
        finish(error)


callback = unreal.register_slate_post_tick_callback(tick)
