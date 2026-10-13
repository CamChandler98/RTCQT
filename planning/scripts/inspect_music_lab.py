"""Read-only prerequisite gate for the Music Lab."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir()).resolve()
catalog = json.loads((root / "Content/Audio/PrivateTest/MusicLibrary/track_catalog.json").read_text())
assert catalog["schema_version"] == 1 and len(catalog["tracks"]) >= 2
ids = set()
rows = []
for row in catalog["tracks"]:
    assert row["id"] and row["label"] and row["id"] not in ids
    ids.add(row["id"])
    sound = unreal.load_asset(row["sound_wave"])
    analyzer = unreal.load_asset(row["analyzer"])
    assert isinstance(sound, unreal.SoundWave) and isinstance(analyzer, unreal.ConstantQNRT), row["id"]
    assert analyzer.get_editor_property("sound") == sound, row["id"]
    duration = sound.get_editor_property("duration")
    assert abs(analyzer.get_editor_property("duration_in_seconds") - duration) < 0.1, row["id"]
    for position in (0, duration / 2, duration - 0.05):
        assert len(analyzer.get_normalized_channel_constant_q_at_time(position, row["channel"])) == analyzer.get_editor_property("settings").get_editor_property("num_bands")
    rows.append(dict(id=row["id"], sound=sound.get_path_name(), analyzer=analyzer.get_path_name()))
sequence = unreal.load_asset("/Game/Audio/PrivateTest/LS_RTCQT_60s")
tracks = [track.get_class().get_name() for track in sequence.get_tracks()]
bindings = [{"name": str(binding.get_name()), "tracks": [t.get_class().get_name() for t in binding.get_tracks()]}
            for binding in sequence.get_bindings()]
report = root / "Saved/Automation/MusicLab/catalog_gate.json"
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps(dict(valid_pairs=rows, sequence_tracks=tracks, bindings=bindings), indent=2))
unreal.log("Music Lab gate PASSED: " + str(len(rows)) + " valid pairs; sequence " + str(tracks))
