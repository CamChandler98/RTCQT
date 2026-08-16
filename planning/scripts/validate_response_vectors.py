#!/usr/bin/env python3
"""Validate the deterministic RTCQT response golden vectors without Unreal."""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


ABS_TOLERANCE = 1e-6
REL_TOLERANCE = 1e-6
DEFAULT_VECTORS = (
    Path(__file__).resolve().parents[1]
    / "test-vectors"
    / "response_golden_vectors.csv"
)


@dataclass(frozen=True)
class ResponseStages:
    sanitized_input: float
    normalized: float
    weighted: float
    response: float
    output: float


def clamp(value: float, minimum: float, maximum: float) -> float:
    return min(max(value, minimum), maximum)


def parse_required(value: str, field_name: str, case_id: str) -> float:
    if value.strip() == "":
        raise ValueError(f"{case_id}: required field {field_name!r} is empty")
    return float(value)


def parse_optional(value: str) -> Optional[float]:
    return None if value.strip() == "" else float(value)


def process_response(
    input_value: float,
    noise_floor: float,
    input_ceiling: float,
    band_weight: Optional[float],
    response_exponent: Optional[float],
    output_min: Optional[float],
    output_max: Optional[float],
) -> ResponseStages:
    sanitized_input = input_value if math.isfinite(input_value) else 0.0

    sanitized_floor = noise_floor if math.isfinite(noise_floor) else 0.0
    sanitized_floor = clamp(sanitized_floor, 0.0, 1.0)

    sanitized_ceiling = input_ceiling if math.isfinite(input_ceiling) else 1.0
    sanitized_ceiling = clamp(sanitized_ceiling, 0.0, 1.0)

    if sanitized_ceiling <= sanitized_floor:
        normalized = 0.0
    else:
        normalized = clamp(
            (sanitized_input - sanitized_floor)
            / (sanitized_ceiling - sanitized_floor),
            0.0,
            1.0,
        )

    if band_weight is None:
        sanitized_weight = 1.0
    elif not math.isfinite(band_weight) or band_weight < 0.0:
        sanitized_weight = 0.0
    else:
        sanitized_weight = band_weight

    if (
        response_exponent is None
        or not math.isfinite(response_exponent)
        or response_exponent <= 0.0
    ):
        sanitized_exponent = 1.0
    else:
        sanitized_exponent = response_exponent

    weighted = clamp(normalized * sanitized_weight, 0.0, 1.0)
    response = clamp(math.pow(weighted, sanitized_exponent), 0.0, 1.0)

    sanitized_output_min = (
        output_min if output_min is not None and math.isfinite(output_min) else 0.0
    )
    sanitized_output_max = (
        output_max if output_max is not None and math.isfinite(output_max) else 1.0
    )
    ordered_min = min(sanitized_output_min, sanitized_output_max)
    ordered_max = max(sanitized_output_min, sanitized_output_max)
    output = ordered_min + (ordered_max - ordered_min) * response

    return ResponseStages(
        sanitized_input=sanitized_input,
        normalized=normalized,
        weighted=weighted,
        response=response,
        output=output,
    )


def assert_close(case_id: str, stage: str, actual: float, expected: float) -> None:
    if not math.isfinite(actual):
        raise AssertionError(f"{case_id}: {stage} produced non-finite value {actual}")
    if not math.isclose(
        actual,
        expected,
        rel_tol=REL_TOLERANCE,
        abs_tol=ABS_TOLERANCE,
    ):
        raise AssertionError(
            f"{case_id}: {stage} expected {expected:.9g}, got {actual:.9g}"
        )


def validate_vectors(path: Path) -> int:
    failures: list[str] = []
    seen_case_ids: set[str] = set()
    count = 0

    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        required_columns = {
            "case_id",
            "input",
            "noise_floor",
            "input_ceiling",
            "band_weight",
            "response_exponent",
            "output_min",
            "output_max",
            "expected_sanitized_input",
            "expected_normalized",
            "expected_weighted",
            "expected_response",
            "expected_output",
        }
        missing_columns = required_columns.difference(reader.fieldnames or [])
        if missing_columns:
            raise ValueError(
                "golden vector CSV is missing columns: "
                + ", ".join(sorted(missing_columns))
            )

        for row in reader:
            case_id = row["case_id"].strip()
            count += 1

            if not case_id:
                failures.append(f"row {count}: case_id is empty")
                continue
            if case_id in seen_case_ids:
                failures.append(f"{case_id}: duplicate case_id")
                continue
            seen_case_ids.add(case_id)

            try:
                stages = process_response(
                    input_value=parse_required(row["input"], "input", case_id),
                    noise_floor=parse_required(
                        row["noise_floor"], "noise_floor", case_id
                    ),
                    input_ceiling=parse_required(
                        row["input_ceiling"], "input_ceiling", case_id
                    ),
                    band_weight=parse_optional(row["band_weight"]),
                    response_exponent=parse_optional(row["response_exponent"]),
                    output_min=parse_optional(row["output_min"]),
                    output_max=parse_optional(row["output_max"]),
                )

                expected = ResponseStages(
                    sanitized_input=parse_required(
                        row["expected_sanitized_input"],
                        "expected_sanitized_input",
                        case_id,
                    ),
                    normalized=parse_required(
                        row["expected_normalized"], "expected_normalized", case_id
                    ),
                    weighted=parse_required(
                        row["expected_weighted"], "expected_weighted", case_id
                    ),
                    response=parse_required(
                        row["expected_response"], "expected_response", case_id
                    ),
                    output=parse_required(
                        row["expected_output"], "expected_output", case_id
                    ),
                )

                assert_close(
                    case_id,
                    "sanitized_input",
                    stages.sanitized_input,
                    expected.sanitized_input,
                )
                assert_close(
                    case_id, "normalized", stages.normalized, expected.normalized
                )
                assert_close(case_id, "weighted", stages.weighted, expected.weighted)
                assert_close(case_id, "response", stages.response, expected.response)
                assert_close(case_id, "output", stages.output, expected.output)
            except (AssertionError, KeyError, TypeError, ValueError) as error:
                failures.append(str(error))

    if count == 0:
        failures.append("golden vector CSV contains no cases")

    if failures:
        print(f"Response vector validation: FAIL ({len(failures)} failure(s))")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print(
        "Response vector validation: PASS "
        f"({count} cases, abs_tol={ABS_TOLERANCE:g}, rel_tol={REL_TOLERANCE:g})"
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate deterministic RTCQT response golden vectors."
    )
    parser.add_argument(
        "vectors",
        nargs="?",
        type=Path,
        default=DEFAULT_VECTORS,
        help=f"golden-vector CSV (default: {DEFAULT_VECTORS})",
    )
    args = parser.parse_args()
    return validate_vectors(args.vectors.resolve())


if __name__ == "__main__":
    raise SystemExit(main())
