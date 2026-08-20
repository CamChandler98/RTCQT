#!/usr/bin/env python3
"""Extract a frame-aligned segment from an uncompressed WAV file."""

from __future__ import annotations

import argparse
import wave
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="source PCM WAV")
    parser.add_argument("destination", type=Path, help="output PCM WAV")
    parser.add_argument("--start", type=float, required=True, help="start time in seconds")
    parser.add_argument("--duration", type=float, required=True, help="duration in seconds")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.start < 0.0 or args.duration <= 0.0:
        raise ValueError("start must be non-negative and duration must be positive")

    source_path = args.source.resolve()
    destination_path = args.destination.resolve()
    if source_path == destination_path:
        raise ValueError("source and destination must be different files")

    with wave.open(str(source_path), "rb") as source:
        if source.getcomptype() != "NONE":
            raise ValueError(f"expected uncompressed PCM, got {source.getcomptype()}")
        sample_rate = source.getframerate()
        start_frame = round(args.start * sample_rate)
        frame_count = round(args.duration * sample_rate)
        if start_frame + frame_count > source.getnframes():
            raise ValueError("requested segment extends past the source duration")

        parameters = source.getparams()
        source.setpos(start_frame)
        frames = source.readframes(frame_count)

    destination_path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(destination_path), "wb") as destination:
        destination.setparams(parameters)
        destination.writeframes(frames)

    print(
        f"wrote {destination_path} "
        f"({frame_count} frames, {frame_count / sample_rate:.6f} seconds)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
