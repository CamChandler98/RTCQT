"""Unreal socket smoke test against streamer_bot_mock_server.py; saves no assets."""
import json
import time
import traceback
from pathlib import Path
import unreal

OUT = Path(unreal.Paths.project_saved_dir()) / "Automation/StreamerBot/transport.json"
OUT.parent.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level("/Game/Audio/PrivateTest/MusicLab/L_MusicLab")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
manager = next(actor for actor in actors if isinstance(actor, unreal.RTCQTMusicLabManager))
assert isinstance(manager.get_editor_property("streamer_bot"), unreal.RTCQTStreamerBotComponent)

receiver = unreal.RTCQTStreamerBotComponent()
receiver.set_editor_property("server_url", "ws://127.0.0.1:18880/")
receiver.set_editor_property("show_test_messages", False)
assert receiver.connect()
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
started = time.monotonic()
callback = None


def finish(success, error=None):
    report = dict(success=success, error=error,
                  received=receiver.get_editor_property("received_command_count"),
                  last_value=receiver.get_editor_property("last_command_value"),
                  status=receiver.get_editor_property("connection_status"),
                  saved_manager_has_receiver=True,
                  live_twitch_command="Requires workstation test")
    receiver.disconnect()
    unreal.unregister_slate_post_tick_callback(callback)
    OUT.write_text(json.dumps(report, indent=2))
    if success:
        unreal.log("RTCQT Streamer.bot socket smoke PASSED")
    else:
        unreal.log_error("RTCQT Streamer.bot socket smoke FAILED: " + str(error))
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)


def tick(_delta):
    try:
        count = receiver.get_editor_property("received_command_count")
        if count >= 2:
            assert count == 2, count
            assert receiver.get_editor_property("last_command_value") == "wrapped socket test"
            assert receiver.get_editor_property("connection_status") == "Ready"
            finish(True)
        elif receiver.get_editor_property("connection_status") == "Error":
            raise RuntimeError(receiver.get_editor_property("last_error"))
        elif time.monotonic() - started > 20:
            raise TimeoutError("Receiver did not accept both socket test messages")
    except Exception:
        finish(False, traceback.format_exc())


callback = unreal.register_slate_post_tick_callback(tick)
