"""Independently compare D12 saved-scene captures with the retained D11 baseline."""

import argparse
import csv
import json
import math
from pathlib import Path


def validate(root, report_path):
    report = json.loads(report_path.read_text())
    assert report["play_in_editor_verified"]
    assert report["visualizer_count"] == 1 and report["instance_count"] == 32
    assert report["active_shared_material_count"] == 1
    assert report["response_exponent"] == 2
    assert report["min_ev100"] == report["max_ev100"]
    assert report["global_eye_adaptation_disabled"] and not report["files_saved"]
    baseline = {}
    with (root / "planning/evidence/d11/checkpoint1-captured-transforms.csv").open() as source:
        for row in csv.DictReader(source):
            baseline[(float(row["time_seconds"]), int(row["band_index"]))] = row
    plays = report["play_frames"]
    editors = report["editor_frames"]
    assert [frame["time_seconds"] for frame in plays] == [16.75, 0.75, 2.0, 33.25, 16.75]
    assert len(editors) == len(plays)
    assert plays[0] == plays[-1] and editors[0] == editors[-1]
    assert len({frame["shared_material"] for frame in plays}) == 1
    assert len({frame["shared_material"] for frame in editors}) == 1
    assert plays[0]["shared_material"] != editors[0]["shared_material"]
    max_response_error = max_scale_error = 0.0
    for play, editor in zip(plays, editors):
        assert play["custom_data"] == editor["custom_data"]
        assert play["transforms"] == editor["transforms"]
        assert play["emissive_range"] == editor["emissive_range"] == [0, 10]
        assert len(play["transforms"]) == 32 and len(play["custom_data"]) == 64
        for index, transform in enumerate(play["transforms"]):
            ref = baseline[(play["time_seconds"], index)]
            response = play["custom_data"][index * 2]
            assert 0 <= response <= 1
            assert math.isclose(play["custom_data"][index * 2 + 1], index / 31, abs_tol=1e-6)
            max_response_error = max(max_response_error, abs(response - float(ref["response"])))
            assert math.isclose(response, float(ref["response"]), abs_tol=1e-6, rel_tol=1e-6)
            for actual, key in zip(transform["translation"], ["local_x", "local_y", "local_z"]):
                assert math.isclose(actual, float(ref[key]), abs_tol=1e-6, rel_tol=1e-6)
            for actual, key in zip(transform["scale"], ["scale_x", "scale_y", "scale_z"]):
                assert math.isclose(actual, float(ref[key]), abs_tol=1e-6, rel_tol=1e-6)
                max_scale_error = max(max_scale_error, abs(actual - float(ref[key])))
            assert 0 <= 10 * response <= 10
    assert report["controlled_endpoints"] == {"response": [0, 0.5, 1], "emissive": [0, 5, 10], "song_data": False}
    assert len(report["stills"]) == 6
    for name in report["stills"]:
        assert (report_path.parent / name).is_file()
    return {"passed": True, "queries": 5, "bands_per_query": 32, "band_rows_checked": 160,
            "play_editor_inputs_match_exactly": True, "same_time_inputs_repeat_exactly": True,
            "maximum_response_difference": max_response_error, "maximum_scale_difference": max_scale_error,
            "baseline": "D11 checkpoint1-captured-transforms.csv", "absolute_and_relative_tolerance": 1e-6}


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=root / "Saved/Automation/D12/fresh-material-frames.json")
    args = parser.parse_args()
    print(json.dumps(validate(root, args.report), indent=2))


if __name__ == "__main__":
    main()
