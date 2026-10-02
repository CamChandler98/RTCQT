"""Compare actual D13 sequence captures against the independent D07 band vectors."""

import argparse
import csv
import json
import math
from pathlib import Path


def validate(root, report_path):
    report = json.loads(report_path.read_text())
    for flag in ["autoplay_verified", "full_forward_preview_verified", "pause_stop_restart_verified",
                 "play_in_editor_verified", "maximum_height_fixture_restored", "final_output_frame_camera_verified"]:
        assert report[flag], flag
    assert not report["files_saved"]
    assert report["visualizer_count"] == report["active_shared_material_count"] == 1
    assert report["instance_count"] == 32 and report["fixed_ev100"] == 2.0
    assert report["playback_frames"] == [0, 3600] and report["display_rate"] == [60, 1]
    assert report["output_resolution"] == [1920, 1080]
    reference = {}
    with (root / "planning/evidence/d07/baseline.csv").open() as source:
        for row in csv.DictReader(source):
            reference[(float(row["time_seconds"]), int(row["band_index"]))] = float(row["normalized"])
    times = [16.75, 0.75, 44.25, 2.0, 11.75, 33.25, 16.75]
    plays, editors = report["play_frames"], report["editor_frames"]
    assert [row["time_seconds"] for row in plays] == times
    assert [row["time_seconds"] for row in editors] == times
    assert plays[0] == plays[-1] and editors[0] == editors[-1]
    assert len({row["shared_material"] for row in plays}) == len({row["shared_material"] for row in editors}) == 1
    assert plays[0]["shared_material"] != editors[0]["shared_material"]
    noise = report["settings"]["noise_floor"]
    ceiling = report["settings"]["input_ceiling"]
    assert report["settings"]["response_exponent"] == 2
    max_response_error = max_scale_error = 0.0
    for play, editor in zip(plays, editors):
        assert play["transforms"] == editor["transforms"]
        assert play["custom_data"] == editor["custom_data"]
        assert play["emissive_range"] == editor["emissive_range"] == [0.0, 10.0]
        assert len(play["transforms"]) == 32 and len(play["custom_data"]) == 64
        for index, transform in enumerate(play["transforms"]):
            normalized = reference[(play["time_seconds"], index)]
            expected_response = min(1.0, max(0.0, (normalized - noise) / (ceiling - noise))) ** 2
            actual_response = play["custom_data"][index * 2]
            assert math.isfinite(actual_response) and 0.0 <= actual_response <= 1.0
            assert math.isclose(actual_response, expected_response, abs_tol=1.e-6, rel_tol=1.e-6)
            assert math.isclose(play["custom_data"][index * 2 + 1], index / 31, abs_tol=1.e-6)
            assert transform["translation"] == [(index - 15.5) * 100.0, 0.0, 0.0]
            assert transform["scale"][:2] == [0.5, 0.5]
            expected_scale = 0.1 + 3.9 * expected_response
            assert math.isclose(transform["scale"][2], expected_scale, abs_tol=1.e-6, rel_tol=1.e-6)
            max_response_error = max(max_response_error, abs(actual_response - expected_response))
            max_scale_error = max(max_scale_error, abs(transform["scale"][2] - expected_scale))
    samples = report["forward_samples"]
    assert len(samples) >= 10 and samples[-1]["player_time"] >= 59.98
    assert all(samples[i]["player_time"] < samples[i + 1]["player_time"] for i in range(len(samples) - 1))
    assert all(abs(row["player_time"] - row["evaluation_time"]) < 1.e-4 for row in samples)
    assert all(row["camera"] == "D08 - Layout Camera" and row["instance_count"] == 32 for row in samples)
    playing_samples = [row for row in samples if row["player_is_playing"]]
    assert len(playing_samples) >= 10 and playing_samples[-1]["player_time"] >= 59.0
    assert all(row["camera_is_view_target"] for row in playing_samples)
    assert not samples[-1]["player_is_playing"]
    assert len({row["shared_material"] for row in samples}) == 1
    assert len(report["boundary_frames"]) == 3
    for frame in report["boundary_frames"]:
        assert len(frame["transforms"]) == 32 and len(frame["custom_data"]) == 64
        assert all(math.isfinite(value) for value in frame["custom_data"])
    assert len(report["stills"]) == 8
    for name in report["stills"]:
        path = report_path.parent / name
        assert path.exists() and path.stat().st_size > 1000
    return {"passed": True, "real_sequence_seeks": 7, "distinct_reference_times": 6,
            "bands_per_query": 32, "band_rows_checked": 224,
            "play_editor_inputs_match_exactly": True, "same_time_inputs_repeat_exactly": True,
            "forward_samples": len(samples), "playing_camera_samples": len(playing_samples), "camera_cuts_verified_in_play": True,
            "maximum_response_difference": max_response_error, "maximum_scale_difference": max_scale_error,
            "baseline": "D07 baseline.csv with independent D02 quadratic response calculation",
            "absolute_and_relative_tolerance": 1.e-6,
            "audible_confirmation": report["audible_confirmation"], "pixel_identical_rendering_claimed": False}


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=root / "Saved/Automation/D13/fresh-sequence-frames.json")
    arguments = parser.parse_args()
    print(json.dumps(validate(root, arguments.report), indent=2))
