"""Verify fresh D13 playback and capture actual Sequencer-driven editor frames.

Run in the rendered editor with PythonScriptPlugin and SequencerScripting.
Never saves a level/asset. Audible workstation confirmation is recorded separately.
"""

from pathlib import Path
import json
import math
import time
import traceback
import unreal

LEVEL = "/Game/Audio/PrivateTest/L_D13SequenceTest"
SEQUENCE = "/Game/Audio/PrivateTest/LS_RTCQT_60s"
OUTPUT = "/Game/Audio/PrivateTest/MRQ_RTCQT_1080p60_Base"
MATERIAL = "/Game/Materials/M_RTCQT_Bands.M_RTCQT_Bands"
FRAMES = [1005, 45, 2655, 120, 705, 1995, 1005]
LABELS = ["strong-first", "quiet", "late", "moderate", "onset", "bright", "strong-repeat", "maximum-height"]
try:
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    directory = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D13"
    directory.mkdir(parents=True, exist_ok=True)
    report = {"scene": LEVEL, "sequence": SEQUENCE, "files_saved": False,
              "audible_confirmation": "Pending workstation listening check",
              "play_frames": [], "editor_frames": [], "forward_samples": [], "stills": []}
    assert levels.load_level(LEVEL)
    scene = actors.get_all_level_actors()
    visualizers = [actor for actor in scene if isinstance(actor, unreal.RTCQTVisualizer)]
    sequence_actors = [actor for actor in scene if isinstance(actor, unreal.LevelSequenceActor)]
    assert len(visualizers) == len(sequence_actors) == 1
    editor_actor = visualizers[0]
    camera = next(actor for actor in scene if isinstance(actor, unreal.CameraActor))
    sequence = assets.load_asset(SEQUENCE)
    assert sequence_actors[0].get_sequence() == sequence
    assert (sequence.get_display_rate().numerator, sequence.get_display_rate().denominator) == (60, 1)
    assert (sequence.get_tick_resolution().numerator, sequence.get_tick_resolution().denominator) == (24000, 1)
    assert sequence.get_playback_start() == 0 and sequence.get_playback_end() == 3600
    assert sequence.get_evaluation_type() == unreal.MovieSceneEvaluationType.WITH_SUB_FRAMES
    assert sequence.get_clock_source() == unreal.UpdateClockSource.TICK
    settings = editor_actor.get_editor_property("response_settings")
    assert settings.get_editor_property("response_exponent") == 2.0
    assert not list(settings.get_editor_property("band_weights"))
    report["settings"] = {"noise_floor": float(settings.get_editor_property("noise_floor")),
                          "input_ceiling": float(settings.get_editor_property("input_ceiling")),
                          "response_exponent": 2.0, "scale_range": [0.1, 4.0], "emissive_range": [0.0, 10.0]}
    post = next(actor for actor in scene if isinstance(actor, unreal.PostProcessVolume)).get_editor_property("settings")
    assert post.get_editor_property("override_auto_exposure_min_brightness")
    assert post.get_editor_property("override_auto_exposure_max_brightness")
    assert post.get_editor_property("auto_exposure_min_brightness") == post.get_editor_property("auto_exposure_max_brightness") == 2.0
    assert camera.get_editor_property("auto_activate_for_player") == unreal.AutoReceiveInput.DISABLED
    camera_component = camera.get_component_by_class(unreal.CameraComponent)
    camera_post = camera_component.get_editor_property("post_process_settings")
    for flag in ["override_auto_exposure_method", "override_auto_exposure_min_brightness",
                 "override_auto_exposure_max_brightness", "override_auto_exposure_bias",
                 "override_auto_exposure_bias_curve", "override_auto_exposure_apply_physical_camera_exposure"]:
        assert not camera_post.get_editor_property(flag), flag
    output = assets.load_asset(OUTPUT).find_setting_by_class(unreal.MoviePipelineOutputSetting)
    assert (output.get_editor_property("output_resolution").x, output.get_editor_property("output_resolution").y) == (1920, 1080)
    assert (output.get_editor_property("output_frame_rate").numerator, output.get_editor_property("output_frame_rate").denominator) == (60, 1)
    assert output.get_editor_property("use_custom_frame_rate")
    assert not output.get_editor_property("use_custom_playback_range")
    report.update({"display_rate": [60, 1], "tick_resolution": [24000, 1],
                   "playback_frames": [0, 3600], "output_resolution": [1920, 1080], "fixed_ev100": 2.0})
    editor.set_level_viewport_camera_info(camera.get_actor_location(), camera.get_actor_rotation())
    actors.set_selected_level_actors([])
    previous_mtimes = {label: (directory / ("frame-" + label + ".png")).stat().st_mtime_ns
                       if (directory / ("frame-" + label + ".png")).exists() else 0 for label in LABELS}
    started = time.monotonic()
    phase = "play_start"
    phase_started = started
    last_sample = -1.0
    capture_index = 0
    task = None
    player = None
    controller = None
    playing_actor = None
except Exception:
    unreal.log_error("D13_CAPTURE_INITIALIZATION_FAILED " + traceback.format_exc())
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()
    raise


def player_seconds(value):
    return float(unreal.TimeManagementLibrary.conv_qualified_frame_time_to_seconds(value.get_current_time()))


def params(frame):
    return unreal.MovieSceneSequencePlaybackParams(
        time=float(frame) / 60.0,
        position_type=unreal.MovieScenePositionType.TIME,
        update_method=unreal.UpdatePositionMethod.JUMP)


def frame_values(actor, song_time):
    diagnostic = actor.get_editor_property("last_sequence_evaluation")
    assert diagnostic.get_editor_property("succeeded"), str(diagnostic)
    actual_time = float(diagnostic.get_editor_property("evaluated_time_seconds"))
    assert abs(actual_time - song_time) < 1.e-4, (actual_time, song_time)
    assert abs(float(actor.get_editor_property("sequence_time_seconds")) - song_time) < 1.e-4
    components = actor.get_components_by_class(unreal.InstancedStaticMeshComponent)
    assert len(components) == 1 and components[0].get_instance_count() == 32
    component = components[0]
    material = component.get_material(0)
    assert isinstance(material, unreal.MaterialInstanceDynamic) and material.get_outer() == actor
    assert material.get_editor_property("parent").get_path_name() == MATERIAL
    assert material.get_scalar_parameter_value("EmissiveMin") == 0.0
    assert material.get_scalar_parameter_value("EmissiveMax") == 10.0
    data = [float(value) for value in component.get_editor_property("per_instance_sm_custom_data")]
    assert len(data) == 64 and all(math.isfinite(value) for value in data)
    transforms = []
    for index in range(32):
        transform = component.get_instance_transform(index, world_space=False)
        transforms.append({"band": index,
                           "translation": [float(transform.translation.x), float(transform.translation.y), float(transform.translation.z)],
                           "scale": [float(transform.scale3d.x), float(transform.scale3d.y), float(transform.scale3d.z)]})
    return {"time_seconds": song_time, "evaluated_time_seconds": actual_time, "transforms": transforms,
            "custom_data": data, "shared_material": material.get_path_name(), "emissive_range": [0.0, 10.0]}


def seek_editor(display_frame):
    unreal.LevelSequenceEditorBlueprintLibrary.set_global_position(params(display_frame))
    unreal.LevelSequenceEditorBlueprintLibrary.force_update()
    return frame_values(editor_actor, display_frame / 60.0)


def finish(error=None):
    if error:
        report["error"] = str(error)
        (directory / "capture-failure.json").write_text(json.dumps(report, indent=2) + "\n")
        unreal.log_error("D13_CAPTURE_FAILED " + str(error))
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()


def tick(delta_seconds):
    global phase, phase_started, player, controller, playing_actor, last_sample, capture_index, task
    try:
        now = time.monotonic()
        assert now - started < 360, "D13 capture timeout"
        if phase == "play_start" and now - started > 5:
            levels.editor_request_begin_play()
            phase, phase_started = "forward_play", now
        elif phase == "forward_play" and levels.is_in_play_in_editor():
            if player is None:
                world = editor.get_game_world()
                active = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RTCQTVisualizer)
                sequence_players = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LevelSequenceActor)
                assert len(active) == len(sequence_players) == 1
                playing_actor = active[0]
                player = sequence_players[0].get_editor_property("sequence_player")
                assert player is not None and player.is_playing()
                assert unreal.TimeManagementLibrary.conv_qualified_frame_time_to_seconds(player.get_duration()) == 60.0
                report["autoplay_verified"] = True
                cut = next(track for track in sequence.get_tracks() if isinstance(track, unreal.MovieSceneCameraCutTrack)).get_sections()[0]
                bound = player.get_bound_objects(cut.get_camera_binding_id())
                controller = unreal.GameplayStatics.get_player_controller(world, 0)
                report["camera_binding_diagnostics"] = {"bound_objects": [obj.get_path_name() for obj in bound],
                    "controller": controller.get_path_name() if controller else None,
                    "initial_view_target": controller.get_view_target().get_path_name() if controller else None,
                    "camera_cuts_disabled": sequence_players[0].get_editor_property("playback_settings").get_editor_property("disable_camera_cuts")}
                unreal.log("D13_CAMERA_BINDING " + json.dumps(report["camera_binding_diagnostics"]))
            seconds = player_seconds(player)
            if seconds - last_sample >= 5 or not player.is_playing():
                values = frame_values(playing_actor, seconds)
                active_camera = player.get_active_camera_component()
                assert active_camera is not None
                view_target = controller.get_view_target()
                camera_is_view_target = view_target == active_camera.get_owner()
                if player.is_playing():
                    assert camera_is_view_target
                    assert view_target.get_path_name() in report["camera_binding_diagnostics"]["bound_objects"]
                else:
                    assert seconds >= 59.98
                    # Camera Cuts restores the gameplay view at the exclusive end.
                    report["sequence_end_view_target"] = view_target.get_path_name()
                report["forward_samples"].append({"player_time": seconds, "evaluation_time": values["evaluated_time_seconds"],
                                                  "camera": active_camera.get_owner().get_actor_label(),
                                                  "player_is_playing": player.is_playing(),
                                                  "camera_is_view_target": camera_is_view_target,
                                                  "instance_count": 32, "shared_material": values["shared_material"]})
                last_sample = seconds
                unreal.log("D13_FORWARD_SAMPLE " + str(seconds))
            if not player.is_playing() and seconds >= 59.98:
                report["full_forward_preview_verified"] = True
                report["preview_elapsed_wall_seconds"] = now - phase_started
                for display_frame in FRAMES:
                    player.set_playback_position(params(display_frame))
                    report["play_frames"].append(frame_values(playing_actor, display_frame / 60.0))
                assert report["play_frames"][0] == report["play_frames"][-1]
                report["boundary_frames"] = []
                for display_frame in [0, 3599, 3600]:
                    player.set_playback_position(params(display_frame))
                    report["boundary_frames"].append(frame_values(playing_actor, display_frame / 60.0))
                    if display_frame == 3599:
                        assert controller.get_view_target().get_path_name() in report["camera_binding_diagnostics"]["bound_objects"]
                        report["final_output_frame_camera_verified"] = True
                player.set_playback_position(params(1005))
                player.pause()
                frame_values(playing_actor, 16.75)
                player.stop()
                frame_values(playing_actor, 16.75)
                player.set_playback_position(params(0))
                player.play()
                player.pause()
                frame_values(playing_actor, 0.0)
                report["pause_stop_restart_verified"] = True
                report["play_in_editor_verified"] = True
                levels.editor_request_end_play()
                phase, phase_started = "editor_open", now
        elif phase == "editor_open" and not levels.is_in_play_in_editor() and now - phase_started > 3:
            assert unreal.LevelSequenceEditorBlueprintLibrary.open_level_sequence(sequence)
            phase, phase_started = "evaluate", now
        elif phase == "evaluate" and now - phase_started > 1:
            if capture_index < len(FRAMES):
                values = seek_editor(FRAMES[capture_index])
                played = report["play_frames"][capture_index]
                assert values["transforms"] == played["transforms"]
                assert values["custom_data"] == played["custom_data"]
                report["editor_frames"].append(values)
            else:
                component = editor_actor.get_components_by_class(unreal.InstancedStaticMeshComponent)[0]
                for index in range(32):
                    transform = component.get_instance_transform(index, world_space=False)
                    transform.scale3d = unreal.Vector(0.5, 0.5, 4.0)
                    assert component.update_instance_transform(index, transform, world_space=False, mark_render_state_dirty=True, teleport=True)
                    assert component.set_custom_data_value(index, 0, 1.0, True)
                report["maximum_height_fixture"] = {"scale_z": 4.0, "response": 1.0, "song_data": False}
            phase, phase_started = "settle", now
        elif phase == "settle" and now - phase_started > 4:
            output_path = directory / ("frame-" + LABELS[capture_index] + ".png")
            task = unreal.AutomationLibrary.take_high_res_screenshot(1920, 1080, str(output_path), camera=camera, delay=2.0)
            assert task and task.is_valid_task()
            phase = "capture"
        elif phase == "capture" and task is not None and task.is_task_done():
            output_path = directory / ("frame-" + LABELS[capture_index] + ".png")
            assert output_path.exists() and output_path.stat().st_size > 1000
            assert output_path.stat().st_mtime_ns > previous_mtimes[LABELS[capture_index]]
            report["stills"].append(output_path.name)
            unreal.log("D13_FRAME_SAVED " + output_path.name)
            capture_index += 1
            task = None
            if capture_index == len(LABELS):
                restored = seek_editor(1005)
                assert restored == report["editor_frames"][0]
                report["maximum_height_fixture_restored"] = True
                report["visualizer_count"] = 1
                report["instance_count"] = 32
                report["active_shared_material_count"] = 1
                (directory / "fresh-sequence-frames.json").write_text(json.dumps(report, indent=2) + "\n")
                unreal.log("D13_FRESH_SEQUENCE_AND_FRAMES_VERIFIED")
                finish()
            else:
                phase, phase_started = "evaluate", now
    except Exception:
        finish(traceback.format_exc())


callback = unreal.register_slate_post_tick_callback(tick)
