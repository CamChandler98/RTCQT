"""Capture the saved D08 scene with Unreal's editor screenshot API.

Run the full editor with -EnablePlugins=PythonScriptPlugin and
-ExecutePythonScript=<this-file>. Requires a working graphics device.
The editor exits after writing Saved/Automation/D08/line-layout.png.
"""

from pathlib import Path
import json
import time
import unreal

unreal.EditorPythonScripting.set_keep_python_script_alive(True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
if not levels.load_level("/Game/Maps/L_D08Layout"):
    raise RuntimeError("D08 level is missing; run setup_d08_layout.py --create first")
scene = actors.get_all_level_actors()
visualizer = next(a for a in scene if isinstance(a, unreal.RTCQTVisualizer))
camera = next(a for a in scene if isinstance(a, unreal.CameraActor))
unreal.log("D08_CAMERA " + str(camera.get_actor_transform()) + " forward=" + str(camera.get_actor_forward_vector()))
camera.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(camera.get_actor_location(), unreal.Vector(0, 0, 25)), False)
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(camera.get_actor_location(), camera.get_actor_rotation())
levels.save_current_level()
actors.set_selected_level_actors([visualizer])
output = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D08" / "line-layout.png"
output.parent.mkdir(parents=True, exist_ok=True)
previous_capture_time = output.stat().st_mtime_ns if output.is_file() else None
started = time.monotonic()
task = None
phase = "play_start"
phase_started = started


def tick(delta_seconds):
    global task, phase, phase_started
    try:
        if time.monotonic() - started > 180:
            raise RuntimeError("D08 screenshot timed out")
        if phase == "play_start" and time.monotonic() - started > 5:
            levels.editor_request_begin_play()
            phase = "play_check"
            phase_started = time.monotonic()
        elif phase == "play_check" and levels.is_in_play_in_editor() and time.monotonic() - phase_started > 3:
            game_world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
            playing = unreal.GameplayStatics.get_all_actors_of_class(game_world, unreal.RTCQTVisualizer)
            if len(playing) != 1:
                raise RuntimeError("Play mode did not contain exactly one visualizer")
            component = playing[0].get_component_by_class(unreal.InstancedStaticMeshComponent)
            if component.get_instance_count() != 32:
                raise RuntimeError("Play mode did not retain 32 instances")
            (output.parent / "play-mode.json").write_text(json.dumps({"visualizer_count": 1, "instance_count": 32, "begin_play_verified": True}, indent=2) + "\n")
            unreal.log("D08_PLAY_VERIFIED 1 actor, 32 instances")
            levels.editor_request_end_play()
            phase = "capture"
            phase_started = time.monotonic()
        elif phase == "capture" and not levels.is_in_play_in_editor() and task is None and time.monotonic() - phase_started > 5:
            task = unreal.AutomationLibrary.take_high_res_screenshot(
                1920, 1080, str(output), camera=camera, delay=2.0)
            if not task or not task.is_valid_task():
                raise RuntimeError("Unreal could not schedule the viewport screenshot")
        elif task is not None and task.is_task_done():
            if not output.is_file():
                raise RuntimeError("Screenshot task finished without its output file")
            if previous_capture_time is not None and output.stat().st_mtime_ns <= previous_capture_time:
                raise RuntimeError("Screenshot task did not replace the previous capture")
            unreal.log("D08_CAPTURE_SAVED " + str(output))
            unreal.unregister_slate_post_tick_callback(callback)
            unreal.EditorPythonScripting.set_keep_python_script_alive(False)
            unreal.SystemLibrary.quit_editor()
    except Exception as error:
        unreal.log_error(str(error))
        unreal.unregister_slate_post_tick_callback(callback)
        unreal.EditorPythonScripting.set_keep_python_script_alive(False)
        unreal.SystemLibrary.quit_editor()


callback = unreal.register_slate_post_tick_callback(tick)
