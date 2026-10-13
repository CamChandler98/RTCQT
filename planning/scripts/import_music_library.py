"""Run in Unreal Editor Python: import WAVs and publish a private dropdown catalog.

Uses library defaults, falling back to D07 without changing its assets. MP3s are
ignored. Existing owned pairs are reused; changed source files are reimported.
"""
from pathlib import Path
import hashlib
import json
import math
import re
import time
import traceback
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
DEST = "/Game/Audio/PrivateTest/MusicLibrary"
OWNER = "RTCQT.MusicLibrary.v1"
PROPERTIES = ("starting_frequency", "num_bands", "num_bands_per_octave",
              "analysis_period", "downmix_to_mono", "fft_size", "window_type",
              "spectrum_type", "band_width_stretch", "cqt_normalization", "noise_floor_db")
PROFILE = ROOT / "Content/Audio/PrivateTest/MusicLibrary/cqt_settings.json"


def settings_values(reference):
    profile = globals().get("CQT_OVERRIDE")
    if profile is None and PROFILE.exists():
        profile = json.loads(PROFILE.read_text(encoding="utf-8"))
    if profile is None:
        return {key: reference.get_editor_property(key) for key in PROPERTIES}, None
    assert set(profile) == set(PROPERTIES), "CQT defaults must contain all 11 settings"
    values = {key: profile[key] for key in PROPERTIES}
    for key, low, high in (("starting_frequency", 20, 20000), ("num_bands_per_octave", 1, 24),
                           ("analysis_period", 0.01, 1), ("band_width_stretch", 0.01, 2),
                           ("noise_floor_db", -120, 0)):
        assert isinstance(values[key], (int, float)) and not isinstance(values[key], bool)
        assert math.isfinite(values[key]) and low <= values[key] <= high, key
    assert type(values["num_bands"]) is int and 1 <= values["num_bands"] <= 96
    assert type(values["downmix_to_mono"]) is bool
    for key, cls in (("fft_size", unreal.ConstantQFFTSizeEnum), ("window_type", unreal.FFTWindowType),
                     ("spectrum_type", unreal.AudioSpectrumType), ("cqt_normalization", unreal.ConstantQNormalizationEnum)):
        assert isinstance(values[key], str) and hasattr(cls, values[key]), "Invalid enum: " + key
        values[key] = getattr(cls, values[key])
    # Read back Unreal's float32 conversion so fingerprints survive reloads.
    temporary = unreal.ConstantQNRTSettings()
    for key, value in values.items():
        temporary.set_editor_property(key, value)
    return {key: temporary.get_editor_property(key) for key in PROPERTIES}, profile


def run():
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    reference = assets.load_asset("/Game/Audio/PrivateTest/CQNRT_NoTimeToExplain_32")
    assert isinstance(reference, unreal.ConstantQNRT), "Missing D07 reference analyzer"
    settings = reference.get_editor_property("settings")
    values, profile = settings_values(settings)
    unreal.log("Music library reference settings: " + str(values))
    files = sorted(p for p in (ROOT / "Music").rglob("*")
                   if p.is_file() and p.suffix.lower() == ".wav")
    assert files, "No Music WAV files found"
    plan = []
    for source in files:
        relative = source.relative_to(ROOT / "Music").as_posix()
        identity = hashlib.sha256(relative.encode()).hexdigest()[:12]
        name = re.sub(r"[^A-Za-z0-9_]", "_", str(Path(relative).with_suffix("")))[:100]
        slug = name + "_" + identity
        paths = [DEST + "/SW_" + slug, DEST + "/CQNRT_" + slug]
        for path, cls in zip(paths, (unreal.SoundWave, unreal.ConstantQNRT)):
            if assets.does_asset_exist(path):
                obj = assets.load_asset(path)
                assert isinstance(obj, cls) and assets.get_metadata_tag(obj, "RTCQT.Owner") == OWNER, path
        plan.append((source, relative, identity, paths))
    rows = []
    counts = {"imported": 0, "reused": 0, "analyzed": 0}
    for source, relative, identity, (sound_path, analyzer_path) in plan:
        digest = hashlib.sha256(source.read_bytes()).hexdigest()
        sound = assets.load_asset(sound_path) if assets.does_asset_exist(sound_path) else None
        changed = sound is None or assets.get_metadata_tag(sound, "RTCQT.SourceSHA256") != digest
        if changed:
            task = unreal.AssetImportTask()
            for key, value in dict(filename=str(source), destination_path=DEST,
                                   destination_name=sound_path.rsplit("/", 1)[1],
                                   automated=True, replace_existing=True, save=False).items():
                task.set_editor_property(key, value)
            tools.import_asset_tasks([task])
            sound = assets.load_asset(sound_path)
            assert isinstance(sound, unreal.SoundWave), relative
            assets.set_metadata_tag(sound, "RTCQT.Owner", OWNER)
            assets.set_metadata_tag(sound, "RTCQT.SourceSHA256", digest)
            assert assets.save_loaded_asset(sound)
            counts["imported"] += 1
        else:
            counts["reused"] += 1
        analyzer = assets.load_asset(analyzer_path) if assets.does_asset_exist(analyzer_path) else None
        fingerprint = hashlib.sha256(("analysis_complete_v2:" + str(values)).encode()).hexdigest()
        analyze = changed or analyzer is None
        if analyzer is None:
            # Duplicate preserves the complete verified settings and factory defaults.
            analyzer = assets.duplicate_asset(reference.get_path_name().split(".")[0], analyzer_path)
            assert isinstance(analyzer, unreal.ConstantQNRT)
            assets.set_metadata_tag(analyzer, "RTCQT.Owner", OWNER)
        analyze |= assets.get_metadata_tag(analyzer, "RTCQT.SettingsSHA256") != fingerprint
        analyze |= assets.get_metadata_tag(analyzer, "RTCQT.SourceSHA256") != digest
        analyze |= analyzer.get_editor_property("sound") != sound
        current_settings = analyzer.get_editor_property("settings")
        analyze |= current_settings is None or any(current_settings.get_editor_property(k) != v for k, v in values.items())
        analyze |= profile is not None and (current_settings is None or current_settings.get_outer() != analyzer)
        if analyze:
            if profile is not None:
                own_settings = unreal.ConstantQNRTSettings(outer=analyzer)
                for key, value in values.items():
                    own_settings.set_editor_property(key, value)
                analyzer.set_editor_property("settings", own_settings)
            else:
                analyzer.set_editor_property("settings", settings)
            analyzer.set_sound(sound)
            complete = {"done": False}
            def on_complete():
                complete["done"] = True
            delegate = analyzer.get_editor_property("on_analysis_complete")
            delegate.add_callable(on_complete)
            analyzer.analyze_audio()
            deadline = time.monotonic() + 300
            while not complete["done"]:
                assert time.monotonic() < deadline, "Analysis timed out: " + relative
                yield
            delegate.remove_callable(on_complete)
            counts["analyzed"] += 1
        duration = float(sound.get_editor_property("duration"))
        assert duration > 0 and math.isfinite(duration)
        assert abs(float(analyzer.get_editor_property("duration_in_seconds")) - duration) < 0.1, relative
        for position in (0.0, duration / 2, max(0.0, duration - 0.05)):
            spectrum = analyzer.get_normalized_channel_constant_q_at_time(position, 0)
            assert len(spectrum) == values["num_bands"] and all(math.isfinite(v) and 0 <= v <= 1 for v in spectrum), relative
        if analyze:
            assets.set_metadata_tag(analyzer, "RTCQT.SettingsSHA256", fingerprint)
            assets.set_metadata_tag(analyzer, "RTCQT.SourceSHA256", digest)
            assert assets.save_loaded_asset(analyzer)
        rows.append(dict(id=identity, label=relative.removesuffix(".wav"),
                         source=relative, source_sha256=digest, duration_seconds=duration,
                         sound_wave=sound.get_path_name(), analyzer=analyzer.get_path_name(), channel=0))
    catalog = ROOT / "Content/Audio/PrivateTest/MusicLibrary/track_catalog.json"
    catalog.parent.mkdir(parents=True, exist_ok=True)
    payload = dict(schema_version=1, reference_analyzer=reference.get_path_name(),
                   settings={k: str(v) if not isinstance(v, (int, float, bool, str)) else v
                             for k, v in values.items()}, tracks=rows)
    temporary = catalog.with_suffix(".json.tmp")
    temporary.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    temporary.replace(catalog)
    if profile is not None:
        profile_temp = PROFILE.with_suffix(".json.tmp")
        profile_temp.write_text(json.dumps(profile, indent=2) + "\n", encoding="utf-8")
        profile_temp.replace(PROFILE)
    report = ROOT / "Saved/Automation/MusicLibrary/result.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(dict(tracks=len(rows), **counts), indent=2), encoding="utf-8")
    unreal.log("Music library complete: " + str(counts))


unreal.EditorPythonScripting.set_keep_python_script_alive(True)
job = run()
callback = None


def tick(delta):
    # Asset save dialogs pump Slate even in unattended mode.
    if job.gi_running:
        return
    try:
        next(job)
        return
    except StopIteration:
        pass
    except Exception:
        unreal.log_error("Music library import FAILED: " + traceback.format_exc())
        error = ROOT / "Saved/Automation/MusicLibrary/failure.txt"
        error.parent.mkdir(parents=True, exist_ok=True)
        error.write_text(traceback.format_exc(), encoding="utf-8")
        (error.parent / "result.json").write_text(json.dumps({"error": traceback.format_exc()}), encoding="utf-8")
    unreal.unregister_slate_post_tick_callback(callback)
    unreal.EditorPythonScripting.set_keep_python_script_alive(False)
    if "-executepythonscript=" in unreal.SystemLibrary.get_command_line().lower():
        unreal.SystemLibrary.quit_editor()


callback = unreal.register_slate_post_tick_callback(tick)
