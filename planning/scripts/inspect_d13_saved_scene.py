"""Read the saved D13 timeline and prove maximum-height camera coverage."""

from pathlib import Path
import itertools
import json
import math
import unreal

LEVEL = "/Game/Audio/PrivateTest/L_D13SequenceTest"
SEQUENCE = "/Game/Audio/PrivateTest/LS_RTCQT_60s"
OUTPUT = "/Game/Audio/PrivateTest/MRQ_RTCQT_1080p60_Base"


def inspect():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    assert levels.load_level(LEVEL)
    scene = actors.get_all_level_actors()
    visualizer = next(a for a in scene if isinstance(a, unreal.RTCQTVisualizer))
    camera = next(a for a in scene if isinstance(a, unreal.CameraActor))
    player = next(a for a in scene if isinstance(a, unreal.LevelSequenceActor))
    sequence = assets.load_asset(SEQUENCE)
    assert player.get_sequence() == sequence
    playback = player.get_editor_property("playback_settings")
    assert playback.get_editor_property("auto_play") and playback.get_editor_property("play_rate") == 1.0
    assert playback.get_editor_property("start_time") == 0.0 and playback.get_editor_property("loop_count").value == 0
    assert sequence.get_display_rate().numerator == 60 and sequence.get_display_rate().denominator == 1
    assert sequence.get_tick_resolution().numerator == 24000 and sequence.get_tick_resolution().denominator == 1
    assert sequence.get_playback_start() == 0 and sequence.get_playback_end() == 3600
    bindings = list(sequence.get_bindings())
    assert len(bindings) == 2
    time_tracks = [track for binding in bindings for track in binding.get_tracks() if isinstance(track, unreal.MovieSceneFloatTrack)]
    assert len(time_tracks) == 1
    time_track = time_tracks[0]
    assert str(time_track.get_property_name()) == "SequenceTimeSeconds"
    section = time_track.get_sections()[0]
    assert section.get_start_frame() == 0 and section.get_end_frame() == 3601
    assert section.get_editor_property("eval_options").get_editor_property("completion_mode") == unreal.MovieSceneCompletionMode.KEEP_STATE
    channel = section.get_all_channels()[0]
    keys = list(channel.get_keys())
    assert len(keys) == 2 and [key.get_value() for key in keys] == [0.0, 60.0]
    assert all(key.get_interpolation_mode() == unreal.RichCurveInterpMode.RCIM_LINEAR for key in keys)
    assert channel.get_pre_infinity_extrapolation() == channel.get_post_infinity_extrapolation() == unreal.RichCurveExtrapolation.RCCE_CONSTANT
    tracks = list(sequence.get_tracks())
    audio = next(t for t in tracks if isinstance(t, unreal.MovieSceneAudioTrack)).get_sections()[0]
    cuts = next(t for t in tracks if isinstance(t, unreal.MovieSceneCameraCutTrack)).get_sections()[0]
    assert audio.get_start_frame() == cuts.get_start_frame() == 0
    assert audio.get_end_frame() == cuts.get_end_frame() == 3600
    assert audio.get_start_offset().value == 0 and not audio.get_editor_property("repeating") and not audio.get_looping()
    assert audio.get_sound() == visualizer.get_editor_property("constant_qnrt").get_editor_property("sound")
    output = assets.load_asset(OUTPUT).find_setting_by_class(unreal.MoviePipelineOutputSetting)
    resolution = output.get_editor_property("output_resolution")
    assert (resolution.x, resolution.y) == (1920, 1080)
    assert output.get_editor_property("use_custom_frame_rate")
    assert output.get_editor_property("output_frame_rate").numerator == 60
    assert output.get_editor_property("output_frame_rate").denominator == 1
    assert not output.get_editor_property("use_custom_playback_range")

    name = LEVEL.rsplit("/", 1)[1]
    level = levels.get_current_level()
    bp = unreal.find_object(None, level.get_path_name() + "." + name)
    graph = unreal.BlueprintEditorLibrary.find_event_graph(bp)
    event = unreal.BlueprintGraphEditor.get_graph_editor(graph).find_event_node("ReceiveBeginPlay")
    assert not any(pin.get_owning_node().get_node_title() == "EvaluateAtTime" for pin in event.find_then_pin().list_connected_pins())

    assert camera.get_editor_property("auto_activate_for_player") == unreal.AutoReceiveInput.DISABLED
    component = camera.get_component_by_class(unreal.CameraComponent)
    rotation, origin = camera.get_actor_rotation(), camera.get_actor_location()
    pitch, yaw = math.radians(rotation.pitch), math.radians(rotation.yaw)
    assert abs(rotation.roll) < 1.e-6
    forward = (math.cos(pitch) * math.cos(yaw), math.cos(pitch) * math.sin(yaw), math.sin(pitch))
    right = (-math.sin(yaw), math.cos(yaw), 0.0)
    up = (-math.sin(pitch) * math.cos(yaw), -math.sin(pitch) * math.sin(yaw), math.cos(pitch))
    fov = float(component.get_editor_property("field_of_view"))
    aspect = float(component.get_editor_property("aspect_ratio"))
    assert abs(aspect - 16 / 9) < 1.e-6
    tan_h = math.tan(math.radians(fov) / 2)
    instances = visualizer.get_components_by_class(unreal.InstancedStaticMeshComponent)[0]
    bounds = visualizer.get_editor_property("instance_mesh").get_bounding_box()
    maximum_z = visualizer.get_editor_property("response_settings").get_editor_property("mesh_z_scale_range").max
    assert maximum_z == 4.0
    actor_transform = visualizer.get_actor_transform()
    points = []
    for index in range(32):
        transform = instances.get_instance_transform(index, world_space=False)
        transform.scale3d = unreal.Vector(transform.scale3d.x, transform.scale3d.y, maximum_z)
        for x, y, z in itertools.product([bounds.min.x, bounds.max.x], [bounds.min.y, bounds.max.y], [bounds.min.z, bounds.max.z]):
            local = unreal.MathLibrary.transform_location(transform, unreal.Vector(x, y, z))
            point = unreal.MathLibrary.transform_location(actor_transform, local)
            relative = (point.x - origin.x, point.y - origin.y, point.z - origin.z)
            dot = lambda direction: sum(a * b for a, b in zip(relative, direction))
            depth = dot(forward)
            assert depth > 0
            points.append((dot(right) / (depth * tan_h), dot(up) / (depth * tan_h / aspect)))
    max_x, max_y = max(abs(p[0]) for p in points), max(abs(p[1]) for p in points)
    assert max_x < 0.95 and max_y < 0.95, (max_x, max_y)
    report = {"saved_scene_verified": True, "display_rate": [60, 1], "tick_resolution": [24000, 1],
              "playback_frames": [0, 3600], "time_section_frames": [0, 3601], "time_key_values": [0, 60],
              "linear_keys": True, "constant_extrapolation": True, "completion_mode": "KeepState",
              "audio_asset": audio.get_sound().get_path_name(), "audio_frames": [0, 3600], "audio_offset_ticks": 0,
              "camera_cut_frames": [0, 3600], "camera_auto_activation": "Disabled", "frozen_beginplay_branch_disabled": True,
              "output_resolution": [1920, 1080], "output_frame_rate": [60, 1], "files_saved": False,
              "maximum_height_framing": {"all_instances": 32, "cube_corners_checked": len(points),
                                         "scale_z": 4.0, "maximum_abs_ndc_x": max_x, "maximum_abs_ndc_y": max_y,
                                         "required_max_ndc": 0.95, "camera_fov": fov, "camera_aspect": aspect}}
    path = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D13" / "saved-scene-inspection.json"
    path.write_text(json.dumps(report, indent=2) + "\n")
    unreal.log("D13_SAVED_SCENE_INSPECTED")


try:
    inspect()
except Exception as error:
    unreal.log_error("D13_INSPECTION_FAILED " + str(error))
    raise
finally:
    unreal.SystemLibrary.quit_editor()
