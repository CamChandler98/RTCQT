#!/usr/bin/env python3
"""Validate the deterministic RTCQT layout golden vectors without Unreal."""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path


ABS_TOLERANCE = 1e-6
REL_TOLERANCE = 1e-6
DEFAULT_VECTORS = (
    Path(__file__).resolve().parents[1]
    / "test-vectors"
    / "layout_golden_vectors.csv"
)
GOLDEN_ANGLE = math.pi * (3.0 - math.sqrt(5.0))


@dataclass(frozen=True)
class Vec3:
    x: float
    y: float
    z: float

    def __add__(self, other: "Vec3") -> "Vec3":
        return Vec3(self.x + other.x, self.y + other.y, self.z + other.z)

    def __sub__(self, other: "Vec3") -> "Vec3":
        return Vec3(self.x - other.x, self.y - other.y, self.z - other.z)

    def __mul__(self, scalar: float) -> "Vec3":
        return Vec3(self.x * scalar, self.y * scalar, self.z * scalar)

    def dot(self, other: "Vec3") -> float:
        return self.x * other.x + self.y * other.y + self.z * other.z

    def length(self) -> float:
        return math.sqrt(self.dot(self))

    def normalized(self) -> "Vec3":
        length = self.length()
        if not math.isfinite(length) or length <= 0.0:
            raise ValueError(f"cannot normalize vector {self}")
        return self * (1.0 / length)

    def is_finite(self) -> bool:
        return all(math.isfinite(value) for value in (self.x, self.y, self.z))


@dataclass(frozen=True)
class LayoutTransform:
    translation: Vec3
    forward: Vec3
    up: Vec3
    scale: Vec3


@dataclass(frozen=True)
class CaseInput:
    layout: str
    count: int
    spacing: float
    radius: float
    origin: Vec3
    scale: Vec3


@dataclass
class GoldenCase:
    inputs: CaseInput
    raw_inputs: tuple[str, ...]
    expected_count: int
    expected: dict[int, LayoutTransform]


def sanitize_origin_component(value: float) -> float:
    return value if math.isfinite(value) else 0.0


def sanitize_scale_component(value: float) -> float:
    return value if math.isfinite(value) and value >= 0.0 else 0.0


def sanitize_dimension(value: float) -> float:
    return value if math.isfinite(value) and value >= 0.0 else 0.0


def sanitize_inputs(inputs: CaseInput) -> CaseInput:
    return CaseInput(
        layout=inputs.layout,
        count=inputs.count,
        spacing=sanitize_dimension(inputs.spacing),
        radius=sanitize_dimension(inputs.radius),
        origin=Vec3(
            sanitize_origin_component(inputs.origin.x),
            sanitize_origin_component(inputs.origin.y),
            sanitize_origin_component(inputs.origin.z),
        ),
        scale=Vec3(
            sanitize_scale_component(inputs.scale.x),
            sanitize_scale_component(inputs.scale.y),
            sanitize_scale_component(inputs.scale.z),
        ),
    )


def line_layout(inputs: CaseInput) -> list[LayoutTransform]:
    if inputs.count <= 0:
        return []
    transforms = []
    midpoint = (inputs.count - 1) / 2.0
    for index in range(inputs.count):
        offset = Vec3((index - midpoint) * inputs.spacing, 0.0, 0.0)
        transforms.append(
            LayoutTransform(
                translation=inputs.origin + offset,
                forward=Vec3(1.0, 0.0, 0.0),
                up=Vec3(0.0, 0.0, 1.0),
                scale=inputs.scale,
            )
        )
    return transforms


def circle_layout(inputs: CaseInput) -> list[LayoutTransform]:
    if inputs.count <= 0:
        return []
    transforms = []
    for index in range(inputs.count):
        angle = math.tau * index / inputs.count
        radial = Vec3(math.cos(angle), math.sin(angle), 0.0)
        transforms.append(
            LayoutTransform(
                translation=inputs.origin + radial * inputs.radius,
                forward=radial,
                up=Vec3(0.0, 0.0, 1.0),
                scale=inputs.scale,
            )
        )
    return transforms


def fibonacci_sphere_layout(inputs: CaseInput) -> list[LayoutTransform]:
    if inputs.count <= 0:
        return []
    transforms = []
    world_up = Vec3(0.0, 0.0, 1.0)
    for index in range(inputs.count):
        z = 1.0 - 2.0 * (index + 0.5) / inputs.count
        ring_radius = math.sqrt(max(0.0, 1.0 - z * z))
        azimuth = index * GOLDEN_ANGLE
        direction = Vec3(
            math.cos(azimuth) * ring_radius,
            math.sin(azimuth) * ring_radius,
            z,
        )
        up = (world_up - direction * z).normalized()
        transforms.append(
            LayoutTransform(
                translation=inputs.origin + direction * inputs.radius,
                forward=direction,
                up=up,
                scale=inputs.scale,
            )
        )
    return transforms


def generate_layout(inputs: CaseInput) -> list[LayoutTransform]:
    sanitized = sanitize_inputs(inputs)
    if sanitized.layout == "line":
        return line_layout(sanitized)
    if sanitized.layout == "circle":
        return circle_layout(sanitized)
    if sanitized.layout == "fibonacci_sphere":
        return fibonacci_sphere_layout(sanitized)
    raise ValueError(f"unknown layout {sanitized.layout!r}")


def parse_required_float(value: str, field_name: str, case_id: str) -> float:
    if value.strip() == "":
        raise ValueError(f"{case_id}: required field {field_name!r} is empty")
    return float(value)


def parse_required_int(value: str, field_name: str, case_id: str) -> int:
    if value.strip() == "":
        raise ValueError(f"{case_id}: required field {field_name!r} is empty")
    return int(value)


def parse_expected_transform(row: dict[str, str], case_id: str) -> LayoutTransform:
    def vector(prefix: str) -> Vec3:
        return Vec3(
            parse_required_float(row[f"expected_{prefix}_x"], f"expected_{prefix}_x", case_id),
            parse_required_float(row[f"expected_{prefix}_y"], f"expected_{prefix}_y", case_id),
            parse_required_float(row[f"expected_{prefix}_z"], f"expected_{prefix}_z", case_id),
        )

    return LayoutTransform(
        translation=vector("translation"),
        forward=vector("forward"),
        up=vector("up"),
        scale=vector("scale"),
    )


def load_cases(path: Path) -> dict[str, GoldenCase]:
    cases: dict[str, GoldenCase] = {}
    input_fields = (
        "layout",
        "count",
        "spacing",
        "radius",
        "origin_x",
        "origin_y",
        "origin_z",
        "scale_x",
        "scale_y",
        "scale_z",
    )
    expected_fields = {
        "case_id",
        *input_fields,
        "expected_count",
        "index",
        "expected_translation_x",
        "expected_translation_y",
        "expected_translation_z",
        "expected_forward_x",
        "expected_forward_y",
        "expected_forward_z",
        "expected_up_x",
        "expected_up_y",
        "expected_up_z",
        "expected_scale_x",
        "expected_scale_y",
        "expected_scale_z",
    }

    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        missing = expected_fields.difference(reader.fieldnames or [])
        if missing:
            raise ValueError(
                "golden vector CSV is missing columns: " + ", ".join(sorted(missing))
            )

        for row_number, row in enumerate(reader, start=2):
            case_id = row["case_id"].strip()
            if not case_id:
                raise ValueError(f"row {row_number}: case_id is empty")
            raw_inputs = tuple(row[field].strip() for field in input_fields)
            expected_count = parse_required_int(
                row["expected_count"], "expected_count", case_id
            )

            if case_id not in cases:
                inputs = CaseInput(
                    layout=row["layout"].strip(),
                    count=parse_required_int(row["count"], "count", case_id),
                    spacing=parse_required_float(row["spacing"], "spacing", case_id),
                    radius=parse_required_float(row["radius"], "radius", case_id),
                    origin=Vec3(
                        parse_required_float(row["origin_x"], "origin_x", case_id),
                        parse_required_float(row["origin_y"], "origin_y", case_id),
                        parse_required_float(row["origin_z"], "origin_z", case_id),
                    ),
                    scale=Vec3(
                        parse_required_float(row["scale_x"], "scale_x", case_id),
                        parse_required_float(row["scale_y"], "scale_y", case_id),
                        parse_required_float(row["scale_z"], "scale_z", case_id),
                    ),
                )
                cases[case_id] = GoldenCase(
                    inputs=inputs,
                    raw_inputs=raw_inputs,
                    expected_count=expected_count,
                    expected={},
                )

            case = cases[case_id]
            if case.raw_inputs != raw_inputs:
                raise ValueError(f"{case_id}: repeated rows have different inputs")
            if case.expected_count != expected_count:
                raise ValueError(f"{case_id}: repeated rows have different expected counts")

            index_text = row["index"].strip()
            if not index_text:
                if expected_count != 0:
                    raise ValueError(f"{case_id}: blank index requires expected_count 0")
                continue

            index = int(index_text)
            if index in case.expected:
                raise ValueError(f"{case_id}: duplicate expected index {index}")
            case.expected[index] = parse_expected_transform(row, case_id)

    if not cases:
        raise ValueError("golden vector CSV contains no cases")
    return cases


def assert_close(case_id: str, field: str, actual: float, expected: float) -> None:
    if not math.isfinite(actual):
        raise AssertionError(f"{case_id}: {field} produced non-finite value {actual}")
    if not math.isclose(actual, expected, rel_tol=REL_TOLERANCE, abs_tol=ABS_TOLERANCE):
        raise AssertionError(
            f"{case_id}: {field} expected {expected:.9g}, got {actual:.9g}"
        )


def assert_vec_close(case_id: str, field: str, actual: Vec3, expected: Vec3) -> None:
    assert_close(case_id, f"{field}.x", actual.x, expected.x)
    assert_close(case_id, f"{field}.y", actual.y, expected.y)
    assert_close(case_id, f"{field}.z", actual.z, expected.z)


def validate_invariants(case_id: str, inputs: CaseInput, outputs: list[LayoutTransform]) -> None:
    sanitized = sanitize_inputs(inputs)
    for index, transform in enumerate(outputs):
        label = f"instance[{index}]"
        for field, vector in (
            ("translation", transform.translation),
            ("forward", transform.forward),
            ("up", transform.up),
            ("scale", transform.scale),
        ):
            if not vector.is_finite():
                raise AssertionError(f"{case_id}: {label}.{field} is not finite")
        assert_close(case_id, f"{label}.forward_length", transform.forward.length(), 1.0)
        assert_close(case_id, f"{label}.up_length", transform.up.length(), 1.0)
        assert_close(case_id, f"{label}.forward_dot_up", transform.forward.dot(transform.up), 0.0)

        offset = transform.translation - sanitized.origin
        if sanitized.layout == "circle":
            assert_close(case_id, f"{label}.radius", offset.length(), sanitized.radius)
            assert_close(case_id, f"{label}.plane_z", offset.z, 0.0)
        elif sanitized.layout == "fibonacci_sphere":
            assert_close(case_id, f"{label}.radius", offset.length(), sanitized.radius)

    if sanitized.layout == "line" and outputs:
        mean_x = sum(transform.translation.x for transform in outputs) / len(outputs)
        assert_close(case_id, "line_center_x", mean_x, sanitized.origin.x)
        for index in range(1, len(outputs)):
            distance = (
                outputs[index].translation - outputs[index - 1].translation
            ).length()
            assert_close(case_id, f"line_spacing[{index}]", distance, sanitized.spacing)


def validate_vectors(path: Path) -> int:
    failures: list[str] = []
    try:
        cases = load_cases(path)
    except (KeyError, TypeError, ValueError) as error:
        print("Layout vector validation: FAIL (could not load vectors)")
        print(f"- {error}")
        return 1

    output_count = 0
    for case_id, case in cases.items():
        try:
            outputs = generate_layout(case.inputs)
            repeated_outputs = generate_layout(case.inputs)
            if outputs != repeated_outputs:
                raise AssertionError(f"{case_id}: repeated evaluation changed output")
            if len(outputs) != case.expected_count:
                raise AssertionError(
                    f"{case_id}: expected {case.expected_count} outputs, got {len(outputs)}"
                )
            expected_indices = set(range(case.expected_count))
            if set(case.expected) != expected_indices:
                raise AssertionError(
                    f"{case_id}: expected indices {sorted(expected_indices)}, "
                    f"found {sorted(case.expected)}"
                )

            validate_invariants(case_id, case.inputs, outputs)
            for index, actual in enumerate(outputs):
                expected = case.expected[index]
                label = f"instance[{index}]"
                assert_vec_close(case_id, f"{label}.translation", actual.translation, expected.translation)
                assert_vec_close(case_id, f"{label}.forward", actual.forward, expected.forward)
                assert_vec_close(case_id, f"{label}.up", actual.up, expected.up)
                assert_vec_close(case_id, f"{label}.scale", actual.scale, expected.scale)
                output_count += 1
        except (AssertionError, KeyError, TypeError, ValueError) as error:
            failures.append(str(error))

    if failures:
        print(f"Layout vector validation: FAIL ({len(failures)} failure(s))")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print(
        "Layout vector validation: PASS "
        f"({len(cases)} cases, {output_count} transforms, "
        f"abs_tol={ABS_TOLERANCE:g}, rel_tol={REL_TOLERANCE:g})"
    )
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate deterministic RTCQT layout golden vectors."
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
