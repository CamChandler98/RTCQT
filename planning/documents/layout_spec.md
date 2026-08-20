# RTCQT Deterministic Layout Contract

## Status

This document completes delivery card D03. It defines the pure layout behavior
used by the offline visualizer described in
`planning/documents/offline_visualizer_architecture.md`.

The contract replaces the legacy actor-spawning layouts with deterministic
functions that return one transform description per band. The functions do not
read actor or world state, spawn objects, or depend on evaluation history.

## Shared Contract

Each layout accepts:

- `Count`: signed integer number of requested instances.
- `Origin`: layout-space translation in Unreal units.
- `BaseScale`: per-axis instance scale.
- A layout dimension: `Spacing` for line or `Radius` for circle and sphere.

Each result contains:

- `Translation`.
- `Forward`, the direction of the instance's local positive X axis.
- `Up`, the direction of the instance's local positive Z axis.
- `Scale`.

`Forward` and `Up` are finite, unit length, and perpendicular. Unreal code may
construct a rotation with `FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat()`.
Quaternion component signs are not part of this contract because `q` and `-q`
represent the same rotation.

### Coordinates and ordering

- All results are expressed in the visualizer's local layout space.
- Positive X is forward, positive Y is right, and positive Z is up.
- The caller may apply the owning component or actor transform afterward.
- Result index `i` always corresponds to input band index `i`.
- Layout functions never reorder instances based on position or frequency.

### Input sanitization

- `Count <= 0` returns an empty array.
- Each non-finite `Origin` component becomes `0`.
- Each finite, non-negative `BaseScale` component is retained. A negative or
  non-finite scale component becomes `0`.
- A finite, non-negative spacing or radius is retained. A negative or
  non-finite dimension becomes `0`.
- Sanitization never changes a positive count, so a positive count always
  returns exactly that many finite results, even when a dimension becomes zero.

Zero scale and zero layout dimensions are intentional safe states. They may
produce invisible or coincident instances, but never reduce the output count.

## Line

The line is centered on `Origin` and extends along positive and negative X.
For `Count = n`, index `i`, and sanitized spacing `s`:

```text
offsetX = (i - (n - 1) / 2) * s
translation = origin + (offsetX, 0, 0)
forward = (1, 0, 0)
up = (0, 0, 1)
```

`Spacing` is center-to-center distance. Consequently:

- Count `1` is exactly at `Origin`.
- Even counts straddle `Origin` without placing an instance on it.
- Odd counts include one instance at `Origin`.

This intentionally replaces the legacy behavior that started at one spacing
unit in front of the actor and accidentally used actor state instead of the
function's spacing argument.

## Circle

The circle lies in the XY plane and is centered on `Origin`. Index `0` begins
on positive X. Indices proceed from positive X toward positive Y, which is
counterclockwise when viewed from positive Z toward the origin.

For sanitized radius `r`:

```text
angle = 2 * pi * i / n
radial = (cos(angle), sin(angle), 0)
translation = origin + r * radial
forward = radial
up = (0, 0, 1)
```

Instances face outward. Radius is measured from the origin to each instance
center and is independent of count, mesh size, and spacing. This replaces the
legacy `Radius + Count * Padding` behavior.

At zero radius all translations equal `Origin`, while orientation and index
ordering remain defined by their angles.

## Fibonacci Sphere

The sphere uses midpoint sampling so no instance lies exactly on a pole. Let:

```text
goldenAngle = pi * (3 - sqrt(5))
k = i + 0.5
z = 1 - 2 * k / n
ringRadius = sqrt(max(0, 1 - z * z))
azimuth = i * goldenAngle
direction = (
    cos(azimuth) * ringRadius,
    sin(azimuth) * ringRadius,
    z
)
translation = origin + sanitizedRadius * direction
forward = direction
```

The up direction is the normalized projection of positive Z onto the tangent
plane at `direction`:

```text
upCandidate = (0, 0, 1) - z * direction
up = normalize(upCandidate)
```

Midpoint sampling guarantees a non-zero `upCandidate` for every positive
finite integer count. Count `1` produces one instance on positive X with
positive Z as up.

At zero radius all translations equal `Origin`; the deterministic sphere
directions and orientations remain unchanged.

## Determinism Requirements

- The result depends only on the current explicit inputs.
- A positive count returns exactly one finite result per input band.
- Evaluation order and previous layout calls do not affect a result.
- Repeated calls with equivalent inputs return equivalent results within the
  documented floating-point tolerance.
- Actor location, rotation, scale, world state, frame rate, and song time do
  not affect layout generation.
- Layout changes do not preserve or reuse stale transforms.

## Golden Test Vectors

`planning/test-vectors/layout_golden_vectors.csv` is the machine-readable
source of expected translations, forward/up axes, scales, counts, and ordering.
It covers counts 0, 1, 2, representative full layouts, translated origins,
non-uniform scale, negative counts, and invalid dimensions.

The comparison tolerance is:

```text
absolute tolerance = 0.000001
relative tolerance = 0.000001
```

Run the reference validation without Unreal:

```bash
python3 planning/scripts/validate_layout_vectors.py
```

The validator is a reference model, not production Unreal code. D08 and D19
must implement this contract in C++ and run these vectors against the C++
implementation.

## Explicitly Deferred

- Spiral and helical layouts.
- Layouts derived from actor bounds or world queries.
- Collision-aware placement.
- Per-layout artistic offsets based on mesh dimensions.
- Stateful animation, interpolation, or layout morphing.

Any future mode must define its own coordinate, ordering, dimension, and
orientation rules without changing these three contracts.
