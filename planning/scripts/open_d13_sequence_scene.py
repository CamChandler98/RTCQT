"""Open the saved D13 scene for Cam's listening/scrubbing check; save nothing."""

import traceback
import unreal

try:
    unreal.EditorPythonScripting.set_keep_python_script_alive(True)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    assert levels.load_level("/Game/Audio/PrivateTest/L_D13SequenceTest")
    sequence = assets.load_asset("/Game/Audio/PrivateTest/LS_RTCQT_60s")
    assert unreal.LevelSequenceEditorBlueprintLibrary.open_level_sequence(sequence)
    unreal.LevelSequenceEditorBlueprintLibrary.set_global_position(
        unreal.MovieSceneSequencePlaybackParams(time=16.75,
            position_type=unreal.MovieScenePositionType.TIME,
            update_method=unreal.UpdatePositionMethod.JUMP))
    unreal.LevelSequenceEditorBlueprintLibrary.force_update()
    scene = actors.get_all_level_actors()
    visualizer = next(a for a in scene if isinstance(a, unreal.RTCQTVisualizer))
    camera = next(a for a in scene if isinstance(a, unreal.CameraActor))
    assert visualizer.get_editor_property("last_sequence_evaluation").get_editor_property("succeeded")
    unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(
        camera.get_actor_location(), camera.get_actor_rotation())
    actors.set_selected_level_actors([visualizer])
    unreal.log("D13_READY_FOR_WORKSTATION_LISTENING_CHECK")
except Exception:
    unreal.log_error("D13_OPEN_FAILED " + traceback.format_exc())
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()
    raise
