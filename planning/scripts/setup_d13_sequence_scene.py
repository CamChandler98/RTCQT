"""Author the private D13 scene using UE 5.8 editor APIs.

Launch with PythonScriptPlugin and SequencerScripting enabled. Preserve D12;
only modify the named D13 copy and assets carrying RTCQT.Delivery=D13.
"""

from pathlib import Path
import hashlib
import json
import unreal

PRIVATE = "/Game/Audio/PrivateTest"
SOURCE_LEVEL = PRIVATE + "/L_D10D11ResponseTest"
LEVEL = PRIVATE + "/L_D13SequenceTest"
SEQUENCE = PRIVATE + "/LS_RTCQT_60s"
OUTPUT = PRIVATE + "/MRQ_RTCQT_1080p60_Base"
SOUND = PRIVATE + "/SW_NoTimeToExplain_60s"
assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
tools = unreal.AssetToolsHelpers.get_asset_tools()
directory = Path(unreal.Paths.project_saved_dir()).resolve() / "Automation" / "D13"
directory.mkdir(parents=True, exist_ok=True)


def vector(value):
    return [float(value.x), float(value.y), float(value.z)]


def owned_asset(path, cls, factory):
    if assets.does_asset_exist(path):
        value = assets.load_asset(path)
        assert isinstance(value, cls)
        assert assets.get_metadata_tag(value, "RTCQT.Delivery") == "D13", path
        return value
    value = tools.create_asset(path.rsplit("/", 1)[1], PRIVATE, cls, factory)
    assert value is not None
    assets.set_metadata_tag(value, "RTCQT.Delivery", "D13")
    return value


def disable_frozen_branch(level):
    name = LEVEL.rsplit("/", 1)[1]
    bp = unreal.find_object(None, level.get_path_name() + "." + name)
    if bp is None:
        bp = unreal.BlueprintEditorLibrary.get_blueprint_asset(level.get_outer())
    assert bp is not None, "Inspect the copied Level Blueprint before continuing"
    graph = unreal.BlueprintEditorLibrary.find_event_graph(bp)
    editor = unreal.BlueprintGraphEditor.get_graph_editor(graph)
    event = editor.find_event_node("ReceiveBeginPlay")
    disabled = []
    if event is not None:
        pin = event.find_then_pin()
        for connection in list(pin.list_connected_pins()):
            node = connection.get_owning_node()
            if node.get_node_title() != "EvaluateAtTime":
                continue
            time_pin = node.find_input_pin("TimeSeconds")
            target_pin = node.find_self_pin()
            targets = [other.get_owning_node().get_node_title() for other in target_pin.list_connected_pins()]
            assert time_pin.is_valid() and float(time_pin.get_pin_value()) == 16.75
            assert targets == ["D08 - 32 Static Bands"], targets
            assert pin.break_single_pin_link(connection)
            disabled.append({"node": node.get_name(), "time": 16.75, "target": targets[0]})
    assert unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    return disabled


def setup():
    source_file = Path(unreal.Paths.project_content_dir()).resolve() / "Audio" / "PrivateTest" / "L_D10D11ResponseTest.umap"
    source_hash = hashlib.sha256(source_file.read_bytes()).hexdigest()
    if assets.does_asset_exist(LEVEL):
        assert levels.load_level(LEVEL)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assert assets.get_metadata_tag(world, "RTCQT.Delivery") == "D13", "Existing D13 level is not owned by this helper"
    else:
        assert levels.load_level(SOURCE_LEVEL)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assert unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL)
        assert levels.load_level(LEVEL)
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        assets.set_metadata_tag(world, "RTCQT.Delivery", "D13")
        assert levels.save_current_level()
    disabled = disable_frozen_branch(levels.get_current_level())
    scene = actors.get_all_level_actors()
    visualizers = [item for item in scene if isinstance(item, unreal.RTCQTVisualizer)]
    cameras = [item for item in scene if isinstance(item, unreal.CameraActor)]
    assert len(visualizers) == len(cameras) == 1
    visualizer, camera = visualizers[0], cameras[0]
    settings = visualizer.get_editor_property("response_settings")
    assert settings.get_editor_property("response_exponent") == 2.0
    analyzer = visualizer.get_editor_property("constant_qnrt")
    sound = assets.load_asset(SOUND)
    assert analyzer.get_editor_property("sound") == sound
    assert abs(float(sound.get_editor_property("duration")) - 60.0) < 1.e-4
    assert visualizer.get_editor_property("instance_material").get_path_name() == "/Game/Materials/M_RTCQT_Bands.M_RTCQT_Bands"

    sequence = owned_asset(SEQUENCE, unreal.LevelSequence, unreal.LevelSequenceFactoryNew())
    for binding in list(sequence.get_bindings()):
        binding.remove()
    for track in list(sequence.get_tracks()):
        sequence.remove_track(track)
    sequence.set_display_rate(unreal.FrameRate(numerator=60, denominator=1))
    sequence.set_tick_resolution_directly(unreal.FrameRate(numerator=24000, denominator=1))
    sequence.set_evaluation_type(unreal.MovieSceneEvaluationType.WITH_SUB_FRAMES)
    sequence.set_clock_source(unreal.UpdateClockSource.TICK)
    assert sequence.get_display_rate().numerator == 60 and sequence.get_display_rate().denominator == 1
    sequence.set_playback_start(0)
    sequence.set_playback_end(3600)
    sequence.set_view_range_start(0.0)
    sequence.set_view_range_end(60.0)
    sequence.set_work_range_start(0.0)
    sequence.set_work_range_end(60.0)

    binding = sequence.add_possessable(visualizer)
    track = binding.add_track(unreal.MovieSceneFloatTrack)
    track.set_property_name_and_path("SequenceTimeSeconds", "SequenceTimeSeconds")
    section = track.add_section()
    # Cover the exact 60-second diagnostic endpoint too; playback stays [0,3600).
    section.set_range(0, 3601)
    options = section.get_editor_property("eval_options")
    options.set_editor_property("completion_mode", unreal.MovieSceneCompletionMode.KEEP_STATE)
    section.set_editor_property("eval_options", options)
    channel = section.get_all_channels()[0]
    for frame, seconds in [(0, 0.0), (3600, 60.0)]:
        channel.add_key(unreal.FrameNumber(frame), seconds, sub_frame=0.0,
                        interpolation=unreal.MovieSceneKeyInterpolation.LINEAR)
    channel.set_pre_infinity_extrapolation(unreal.RichCurveExtrapolation.RCCE_CONSTANT)
    channel.set_post_infinity_extrapolation(unreal.RichCurveExtrapolation.RCCE_CONSTANT)

    audio_track = sequence.add_track(unreal.MovieSceneAudioTrack)
    audio = audio_track.add_section()
    audio.set_sound(sound)
    audio.set_start_offset(unreal.FrameNumber(0))
    audio.set_range(0, 3600)
    audio.set_repeating(False)
    audio.set_looping(False)

    # Sequencer owns the runtime camera handoff; D08 autoactivation bypasses it.
    camera.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.DISABLED)
    component = camera.get_component_by_class(unreal.CameraComponent)
    component.set_editor_property("aspect_ratio", 16.0 / 9.0)
    component.set_editor_property("constrain_aspect_ratio", True)
    # Keep D08's oblique view with room around the full band strip.
    component.set_editor_property("field_of_view", 65.0)
    camera_binding = sequence.add_possessable(camera)
    cut_track = sequence.add_track(unreal.MovieSceneCameraCutTrack)
    cut = cut_track.add_section()
    cut.set_range(0, 3600)
    camera_id = sequence.get_binding_id(camera_binding)
    cut.set_camera_binding_id(camera_id)

    players = [item for item in scene if isinstance(item, unreal.LevelSequenceActor)]
    assert len(players) <= 1
    player_actor = players[0] if players else actors.spawn_actor_from_class(unreal.LevelSequenceActor, unreal.Vector())
    player_actor.set_actor_label("D13 - Sequence Player")
    player_actor.set_sequence(sequence)
    playback = player_actor.get_editor_property("playback_settings")
    playback.set_editor_property("auto_play", True)
    playback.set_editor_property("play_rate", 1.0)
    playback.set_editor_property("start_time", 0.0)
    playback.set_editor_property("random_start_time", False)
    playback.set_editor_property("disable_camera_cuts", False)
    playback.set_editor_property("loop_count", unreal.MovieSceneSequenceLoopCount(value=0))
    player_actor.set_editor_property("playback_settings", playback)

    config = owned_asset(OUTPUT, unreal.MoviePipelinePrimaryConfig, unreal.MoviePipelinePrimaryConfigFactory())
    output = config.find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
    output.set_editor_property("output_resolution", unreal.IntPoint(1920, 1080))
    output.set_editor_property("use_custom_frame_rate", True)
    output.set_editor_property("output_frame_rate", unreal.FrameRate(numerator=60, denominator=1))
    output.set_editor_property("use_custom_playback_range", False)

    assert unreal.LevelSequenceEditorBlueprintLibrary.open_level_sequence(sequence)
    unreal.LevelSequenceEditorBlueprintLibrary.set_current_time(0)
    unreal.LevelSequenceEditorBlueprintLibrary.force_update()
    assert visualizer.get_editor_property("last_sequence_evaluation").get_editor_property("succeeded")
    assert assets.save_loaded_asset(sequence, only_if_is_dirty=False)
    assert assets.save_loaded_asset(config, only_if_is_dirty=False)
    assert levels.save_current_level()
    assert hashlib.sha256(source_file.read_bytes()).hexdigest() == source_hash
    report = {"level": LEVEL, "sequence": SEQUENCE, "output_config": OUTPUT,
              "source_level_unchanged_sha256": source_hash, "disabled_frozen_calls": disabled,
              "display_rate": [60, 1], "tick_resolution": [24000, 1], "playback_frames": [0, 3600],
              "time_section_frames": [0, 3601], "time_keys": [[0, 0.0], [3600, 60.0]],
              "linear_interpolation": True, "completion_mode": "KeepState",
              "audio": SOUND, "audio_frames": [0, 3600], "audio_offset_ticks": 0,
              "autoplay": True, "play_rate": 1.0, "loop_count": 0,
              "camera": camera.get_actor_label(), "camera_location": vector(camera.get_actor_location()),
              "camera_fov": 65.0, "camera_auto_activation": "Disabled", "output_resolution": [1920, 1080], "saved": True}
    (directory / "sequence-scene-setup.json").write_text(json.dumps(report, indent=2) + "\n")
    unreal.log("D13_SEQUENCE_SCENE_SAVED")


try:
    setup()
except Exception as error:
    unreal.log_error("D13_SETUP_FAILED " + str(error))
    raise
finally:
    unreal.SystemLibrary.quit_editor()
