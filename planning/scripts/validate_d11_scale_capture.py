#!/usr/bin/env python3
"""Check applied D11 transforms against independent D07 numeric references."""

import argparse
import csv
import json
import math
from pathlib import Path


FIELDS = (
    "query_index", "time_seconds", "band_index", "analyzer_normalized",
    "response", "local_x", "local_y", "local_z", "scale_x", "scale_y",
    "scale_z", "normalized_band_index",
)
ORDER = (16.75, 0.75, 44.25, 16.75, 2.0, 11.75, 33.25, 16.75)
TOLERANCE = 1e-6


def validate(capture_path, reference_path):
    with reference_path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != ["time_seconds", "band_index", "raw_db", "normalized"]:
            raise ValueError("Unexpected D07 reference columns")
        reference = {}
        for row in reader:
            key = (float(row["time_seconds"]), int(row["band_index"]))
            value = float(row["normalized"])
            if key in reference or not 0 <= key[1] < 32 or not math.isfinite(value) or not 0 <= value <= 1:
                raise ValueError(f"Invalid or duplicated D07 key: {key}")
            reference[key] = value
        if set(reference) != {(time, band) for time in set(ORDER) for band in range(32)}:
            raise ValueError("D07 reference must have all six timestamps and 32 bands")

    maxima = {field: 0.0 for field in FIELDS[3:]}
    seen = set()
    repeated = {}
    with capture_path.open(newline="", encoding="utf-8-sig") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != list(FIELDS):
            raise ValueError("Unexpected D11 capture columns")
        for row in reader:
            query, band = int(row["query_index"]), int(row["band_index"])
            time = float(row["time_seconds"])
            if not 0 <= query < len(ORDER) or not 0 <= band < 32 or time != ORDER[query] or (query, band) in seen:
                raise ValueError(f"Invalid query key: {(query, time, band)}")
            seen.add((query, band))
            values = {field: float(row[field]) for field in FIELDS[3:]}
            if not all(math.isfinite(value) for value in values.values()):
                raise ValueError(f"Non-finite capture: {(query, band)}")

            # D02 defaults, evaluated independently of the Unreal processor.
            normalized = min(max((reference[time, band] - 0.1) / 0.9, 0.0), 1.0)
            response = normalized ** 2.0
            expected = {
                "analyzer_normalized": reference[time, band],
                "response": response,
                "local_x": (band - 15.5) * 100,
                "local_y": 0.0,
                "local_z": 0.0,
                "scale_x": 0.5,
                "scale_y": 0.5,
                "scale_z": 0.1 + 3.9 * response,
                "normalized_band_index": band / 31.0,
            }
            for field, target in expected.items():
                actual = values[field]
                maxima[field] = max(maxima[field], abs(actual - target))
                if not math.isclose(actual, target, abs_tol=TOLERANCE, rel_tol=TOLERANCE):
                    raise ValueError(f"Query {query} band {band} {field}: {actual} != {target}")
            key = time, band
            if key in repeated and values != repeated[key]:
                raise ValueError(f"Repeated timestamp changed captured values: {key}")
            repeated[key] = values
    if seen != {(query, band) for query in range(len(ORDER)) for band in range(32)}:
        raise ValueError("Capture must contain all 256 ordered timestamp-band rows")
    return {
        "status": "PASS",
        "d07_reference_pairs": len(reference),
        "queries": len(ORDER),
        "captured_rows": len(seen),
        "query_order_seconds": list(ORDER),
        "absolute_tolerance": TOLERANCE,
        "relative_tolerance": TOLERANCE,
        "maximum_absolute_differences": maxima,
        "repeated_capture_values_match_exactly": True,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", type=Path, default=Path("planning/evidence/d11/checkpoint1-captured-transforms.csv"))
    parser.add_argument("--reference", type=Path, default=Path("planning/evidence/d07/baseline.csv"))
    args = parser.parse_args()
    try:
        result = validate(args.capture, args.reference)
    except (OSError, KeyError, TypeError, ValueError) as error:
        parser.exit(1, f"D11 capture validation FAIL: {error}\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
