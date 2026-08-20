#!/usr/bin/env python3
"""Inspect a PCM WAV and propose deterministic visualizer reference times."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import wave
from pathlib import Path

import numpy as np


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("wav", type=Path, help="PCM WAV file to inspect")
    parser.add_argument("--window", type=float, default=1.0, help="analysis window in seconds")
    parser.add_argument("--hop", type=float, default=0.25, help="analysis hop in seconds")
    parser.add_argument("--candidates", type=int, default=5, help="candidates per category")
    return parser.parse_args()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_pcm16(path: Path) -> tuple[np.ndarray, dict[str, int | float | str]]:
    with wave.open(str(path), "rb") as source:
        channels = source.getnchannels()
        sample_width = source.getsampwidth()
        sample_rate = source.getframerate()
        frame_count = source.getnframes()
        compression = source.getcomptype()
        frames = source.readframes(frame_count)

    if compression != "NONE":
        raise ValueError(f"expected uncompressed PCM, got {compression}")
    if sample_width != 2:
        raise ValueError(f"expected 16-bit PCM, got {sample_width * 8}-bit")
    if channels <= 0 or sample_rate <= 0 or frame_count <= 0:
        raise ValueError("WAV header contains a non-positive channel, rate, or frame count")

    samples = np.frombuffer(frames, dtype="<i2").reshape(-1, channels)
    mono = samples.astype(np.float64).mean(axis=1) / 32768.0
    metadata: dict[str, int | float | str] = {
        "channels": channels,
        "sample_width_bits": sample_width * 8,
        "sample_rate_hz": sample_rate,
        "frame_count": frame_count,
        "duration_seconds": frame_count / sample_rate,
        "compression": compression,
    }
    return mono, metadata


def power_db(value: float) -> float:
    return 10.0 * math.log10(max(value, 1.0e-20))


def spaced_candidates(
    rows: list[dict[str, float]],
    key: str,
    count: int,
    descending: bool,
    minimum_separation: float = 2.0,
) -> list[dict[str, float]]:
    ordered = sorted(rows, key=lambda row: row[key], reverse=descending)
    selected: list[dict[str, float]] = []
    for row in ordered:
        if all(abs(row["time_seconds"] - prior["time_seconds"]) >= minimum_separation for prior in selected):
            selected.append(row)
        if len(selected) == count:
            break
    return selected


def analyze(
    mono: np.ndarray,
    sample_rate: int,
    window_seconds: float,
    hop_seconds: float,
) -> list[dict[str, float]]:
    window_size = max(1, round(window_seconds * sample_rate))
    hop_size = max(1, round(hop_seconds * sample_rate))
    taper = np.hanning(window_size)
    frequencies = np.fft.rfftfreq(window_size, d=1.0 / sample_rate)
    masks = {
        "bass": (frequencies >= 20.0) & (frequencies < 250.0),
        "mid": (frequencies >= 250.0) & (frequencies < 4000.0),
        "high": (frequencies >= 4000.0) & (frequencies <= 16000.0),
    }

    rows: list[dict[str, float]] = []
    previous_rms_db = -120.0
    for start in range(0, max(1, mono.size - window_size + 1), hop_size):
        frame = mono[start : start + window_size]
        if frame.size != window_size:
            break
        rms_power = float(np.mean(np.square(frame)))
        rms_db = power_db(rms_power)
        spectrum_power = np.square(np.abs(np.fft.rfft(frame * taper)))
        band_power = {name: float(np.sum(spectrum_power[mask])) for name, mask in masks.items()}
        total_power = max(sum(band_power.values()), 1.0e-20)
        shares = {name: value / total_power for name, value in band_power.items()}
        time_seconds = (start + window_size / 2.0) / sample_rate
        rows.append(
            {
                "time_seconds": time_seconds,
                "rms_dbfs": rms_db,
                "onset_gain_db": rms_db - previous_rms_db,
                "bass_share": shares["bass"],
                "mid_share": shares["mid"],
                "high_share": shares["high"],
                "balanced_score": min(shares.values()),
            }
        )
        previous_rms_db = rms_db
    return rows


def compact(row: dict[str, float]) -> dict[str, float]:
    return {key: round(value, 6) for key, value in row.items()}


def main() -> int:
    args = parse_args()
    if args.window <= 0.0 or args.hop <= 0.0 or args.candidates <= 0:
        raise ValueError("window, hop, and candidates must be positive")

    path = args.wav.resolve()
    mono, metadata = load_pcm16(path)
    rows = analyze(mono, int(metadata["sample_rate_hz"]), args.window, args.hop)
    audible_rows = [row for row in rows if row["rms_dbfs"] >= -50.0]
    if not audible_rows:
        raise ValueError("no analysis window exceeded -50 dBFS")

    report = {
        "path": str(path),
        "sha256": sha256(path),
        "metadata": metadata,
        "analysis": {
            "window_seconds": args.window,
            "hop_seconds": args.hop,
            "audible_threshold_dbfs": -50.0,
            "bands_hz": {"bass": [20, 250], "mid": [250, 4000], "high": [4000, 16000]},
        },
        "candidates": {
            "first_strong_onsets": [compact(row) for row in spaced_candidates(audible_rows, "onset_gain_db", args.candidates, True)],
            "quiet": [compact(row) for row in spaced_candidates(audible_rows, "rms_dbfs", args.candidates, False)],
            "bass_dominant": [compact(row) for row in spaced_candidates(audible_rows, "bass_share", args.candidates, True)],
            "mid_dominant": [compact(row) for row in spaced_candidates(audible_rows, "mid_share", args.candidates, True)],
            "high_dominant": [compact(row) for row in spaced_candidates(audible_rows, "high_share", args.candidates, True)],
            "full_spectrum_peak": [compact(row) for row in spaced_candidates(audible_rows, "rms_dbfs", args.candidates, True)],
            "most_balanced": [compact(row) for row in spaced_candidates(audible_rows, "balanced_score", args.candidates, True)],
        },
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
