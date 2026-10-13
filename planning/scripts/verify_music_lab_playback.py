"""Exercise real PIE mixer callbacks and controls; save no project assets.

Run with audio enabled. This records engine evidence, not a listening verdict.
"""
import json
import math
import time
import traceback
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
gate = ROOT / "planning/scripts/inspect_music_lab.py"
exec(compile(gate.read_text(), str(gate), "exec"), {"__name__": "music_lab_gate"})
OUT = ROOT / "Saved/Automation/MusicLab"
OUT.mkdir(parents=True, exist_ok=True)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
assert levels.load_level("/Game/Audio/PrivateTest/MusicLab/L_MusicLab")
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
report = {"samples": [], "audible_and_perceptual_alignment": "Requires workstation confirmation"}
started = time.monotonic()
test_counts = globals().get("CQT_TEST_COUNTS", [])
phase = "prepare" if test_counts else "start"
manager = None
callback = None
paused_at = 0
phase_at = started
index = 0
ids = []
combo = None
prepared = []
def completion_for(marker):
    def completed():
        marker["done"] = True
    return completed

if test_counts:
    scene = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    editor_manager = next(a for a in scene if isinstance(a, unreal.RTCQTMusicLabManager))
    template = editor_manager.get_editor_property("catalog").get_editor_property("tracks")[0]
    template_analyzer = template.get_editor_property("analyzer")
    for count in test_counts:
        analyzer = unreal.ConstantQNRT()
        settings = unreal.ConstantQNRTSettings(outer=analyzer)
        for key in ("starting_frequency", "analysis_period", "downmix_to_mono", "fft_size", "window_type",
                    "spectrum_type", "band_width_stretch", "cqt_normalization", "noise_floor_db"):
            settings.set_editor_property(key, template_analyzer.get_editor_property("settings").get_editor_property(key))
        settings.set_editor_property("num_bands", count)
        settings.set_editor_property("num_bands_per_octave", 3.5 * count / 32)
        analyzer.set_editor_property("settings", settings)
        analyzer.set_sound(template.get_editor_property("sound_wave"))
        marker = {"done": False}
        completed = completion_for(marker)
        analyzer.get_editor_property("on_analysis_complete").add_callable(completed)
        analyzer.analyze_audio()
        prepared.append((count, analyzer, marker, completed))


def walk(widget):
    yield widget
    if isinstance(widget, unreal.PanelWidget):
        for n in range(widget.get_children_count()):
            yield from walk(widget.get_child_at(n))


def sample(label):
    sound = manager.get_editor_property("current_track").get_editor_property("sound_wave")
    analyzer = manager.get_editor_property("active_analyzer")
    assert analyzer.get_editor_property("sound") == sound
    position = manager.get_editor_property("playback_position")
    callbacks = manager.get_editor_property("playback_callback_count")
    assert callbacks > 0 and position > 0 and math.isfinite(position)
    if label in ("B after switch", "A after switch back", "A restarted"):
        assert position <= time.monotonic() - phase_at + 0.5, "Stale mixer source position after restart/switch"
    components = manager.get_components_by_class(unreal.AudioComponent)
    assert len(components) == 1 and components[0].is_playing()
    visualizer = manager.get_editor_property("visualizer")
    mesh = visualizer.get_component_by_class(unreal.InstancedStaticMeshComponent)
    assert mesh.get_instance_count() == analyzer.get_editor_property("settings").get_editor_property("num_bands")
    count = mesh.get_instance_count()
    span = (mesh.get_instance_transform(count - 1, False).translation.x
            - mesh.get_instance_transform(0, False).translation.x) if count > 1 else 0
    if test_counts and report["samples"]:
        assert abs(span - report["samples"][0]["line_span"]) < 0.01
    # Compare the rendered custom-data responses against a fresh isolated query
    # at the manager's last mixer time; no game-time clock participates.
    before = list(mesh.get_editor_property("per_instance_sm_custom_data"))
    evaluation = visualizer.evaluate_at_time(position)
    assert evaluation.get_editor_property("succeeded")
    assert before == list(mesh.get_editor_property("per_instance_sm_custom_data"))
    report["samples"].append(dict(label=label, track_id=manager.get_editor_property("current_track").get_editor_property("id"),
                                  mixer_position=position, callbacks=callbacks, live_audio_components=1,
                                  analyzer_matches=True, visual_response_matches=True, band_count=count, line_span=span))


def verify_weight_output():
    visualizer = manager.get_editor_property("visualizer")
    settings = visualizer.get_editor_property("response_settings")
    saved = [(r.get_editor_property("label"), r.get_editor_property("min_hz"),
              r.get_editor_property("max_hz"), r.get_editor_property("gain"))
             for r in settings.get_editor_property("frequency_weights")]
    def rows(values):
        result = []
        for label, low, high, gain in values:
            row = unreal.RTCQTFrequencyWeight()
            for key, value in dict(label=label, min_hz=low, max_hz=high, gain=gain).items():
                row.set_editor_property(key, value)
            result.append(row)
        return result
    try:
        settings.set_editor_property("frequency_weights", rows([("Mute test", 0, 3.e38, 0)]))
        assert visualizer.evaluate_at_time(manager.get_editor_property("playback_position")).get_editor_property("succeeded")
        mesh = visualizer.get_component_by_class(unreal.InstancedStaticMeshComponent)
        data = list(mesh.get_editor_property("per_instance_sm_custom_data"))
        assert all(value == 0 for value in data[::2])
        report["frequency_weights_reach_visual_output"] = True
    finally:
        settings.set_editor_property("frequency_weights", rows(saved))
        visualizer.evaluate_at_time(manager.get_editor_property("playback_position"))


def finish(error=None):
    report["passed"] = error is None
    if error:
        report["error"] = str(error)
        unreal.log_error("MUSIC_LAB_PLAYBACK_FAILED " + str(error))
    else:
        unreal.log("MUSIC_LAB_PLAYBACK_PASSED")
    (OUT / "playback.json").write_text(json.dumps(report, indent=2))
    levels.editor_request_end_play()
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    unreal.SystemLibrary.quit_editor()


def tick(delta):
    global phase, manager, phase_at, index, ids, combo, paused_at
    try:
        now = time.monotonic()
        assert now - started < 160, "Playback test timed out (check audio callback support/device)"
        if phase == "prepare" and all(marker["done"] for _, _, marker, _ in prepared):
            proxy = unreal.RTCQTMusicCatalog()
            entries = []
            for count, analyzer, marker, completion in prepared:
                analyzer.get_editor_property("on_analysis_complete").remove_callable(completion)
                assert len(analyzer.get_normalized_channel_constant_q_at_time(30, 0)) == count
                entry = unreal.RTCQTMusicTrack()
                for key, value in dict(id="bands-" + str(count), label=str(count) + " bands",
                                       sound_wave=template.get_editor_property("sound_wave"), analyzer=analyzer, channel=0).items():
                    entry.set_editor_property(key, value)
                entries.append(entry)
            proxy.set_editor_property("tracks", entries)
            editor_manager.set_editor_property("catalog", proxy)
            editor_manager.set_editor_property("initial_track_id", "bands-" + str(test_counts[0]))
            report["temporary_analysis_counts"] = test_counts
            phase = "start"
        elif phase == "start" and now - started > 3:
            levels.editor_request_begin_play()
            phase = "acquire"
        elif phase == "acquire" and levels.is_in_play_in_editor():
            world = editor.get_game_world()
            managers = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.RTCQTMusicLabManager)
            assert len(managers) == 1
            manager = managers[0]
            assert not unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LevelSequenceActor)
            tracks = list(manager.get_editor_property("catalog").get_editor_property("tracks"))
            ids = [t.get_editor_property("id") for t in tracks]
            widgets = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.RTCQTMusicLabControls, True)
            assert len(widgets) == 1
            combo = widgets[0].get_track_dropdown()
            assert isinstance(combo, unreal.ComboBoxString)
            assert combo.get_option_count() == len(ids) == len(set(ids)), {
                "options": combo.get_option_count(), "tracks": len(ids),
                "widget_manager": str(widgets[0].get_editor_property("manager")),
                "manager_error": manager.get_editor_property("last_error")}
            report["dropdown_options"] = combo.get_option_count()
            phase, phase_at = "initial", now
        elif phase == "initial" and manager.get_editor_property("playback_position") > 2:
            sample("A beginning")
            verify_weight_output()
            audio = manager.get_editor_property("audio_component")
            assert manager.select_track(ids[0]) and manager.get_editor_property("audio_component") == audio
            assert not manager.select_track("invalid-id") and manager.get_editor_property("audio_component") == audio
            phase = "midpoint"
        elif phase == "midpoint" and manager.get_editor_property("playback_position") >= 30:
            sample("A midpoint")
            manager.toggle_play_pause()
            paused_at = manager.get_editor_property("playback_position")
            phase, phase_at = "paused", now
        elif phase == "paused" and now - phase_at > 1:
            assert manager.get_editor_property("playback_position") == paused_at
            manager.toggle_play_pause()
            assert manager.select_track(ids[1])
            assert manager.get_editor_property("playback_position") == 0
            phase, phase_at = "B", now
        elif phase == "B" and manager.get_editor_property("playback_position") > 2:
            sample("B after switch")
            assert manager.select_track(ids[0])
            phase, phase_at = "A", now
        elif phase == "A" and manager.get_editor_property("playback_position") > 2:
            sample("A after switch back")
            assert manager.restart()
            assert manager.get_editor_property("playback_position") == 0
            phase, phase_at = "restart", now
        elif phase == "restart" and manager.get_editor_property("playback_position") > 2:
            sample("A restarted")
            unreal.SystemLibrary.execute_console_command(editor.get_game_world(),
                "Shot showui filename=" + str(OUT / "controls.png") + " -nosuffix")
            index = 0
            phase = "all_tracks"
        elif phase == "all_tracks":
            if index == len(ids):
                report["all_tracks_selected"] = len(ids)
                report["pause_resume_verified"] = True
                finish()
                return
            combo.get_editor_property("on_selection_changed").broadcast(
                combo.get_option_at_index(index), unreal.SelectInfo.ON_MOUSE_CLICK)
            assert manager.get_editor_property("current_track").get_editor_property("id") == ids[index]
            phase, phase_at = "track_wait", now
        elif phase == "track_wait" and manager.get_editor_property("playback_position") > 0.2:
            sample("catalog entry " + str(index))
            index += 1
            phase = "all_tracks"
    except Exception:
        finish(traceback.format_exc())


callback = unreal.register_slate_post_tick_callback(tick)
