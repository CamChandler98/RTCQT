# D12: Material and exposure plan

Prepared: 2026-10-02, America/New_York.

Status: In progress; implementation started 2026-10-02 on `codex/d12-material-exposure`.
Prerequisite: D11 is complete at `e574d06` on `codex/d11-scale-response`.

## Finish line

The existing 32 cubes use the same deterministic response for height, color,
and glow. Quiet cubes remain visible, strong cubes retain their color, and
scene exposure stays fixed as the requested timestamp changes.

Work in `C:\Users\camch\source\RTCQT` using Unreal Engine 5.8. The WSL
checkout at `/home/camc/code/projects/RTCQT` remains source-only. Start a
`codex/d12-material-exposure` branch from the verified D11 checkpoint when
implementation begins.

## Existing foundation

- One visualizer actor, one instanced static mesh component, 32 instances.
- Custom-data slot `0`: D02 response in `[0, 1]`, already used for D11 height.
- Custom-data slot `1`: normalized band index in `[0, 1]`.
- `Instance Material`: one optional component-wide override.
- `DA_ResponseTEst`: the existing private response-settings asset. Keep
  Response Exponent `2`, Noise Floor `0.1`, Input Ceiling `1`, and Z range
  `0.1` to `4` for the baseline.
- `Emissive Range` already exists in the settings class with default `0` to
  `10`; D12 connects it to the material.
- Existing scene: `/Game/Audio/PrivateTest/L_D10D11ResponseTest`. The
  visualizer's label can remain `D08 - 32 Static Bands`.
- Project settings already enable the extended exposure luminance range,
  so the exposure limits are shown as Min EV100 and Max EV100.

## Steps and ownership

### 1. Prepare the material contract — Codex

Document the two custom-data slots and these named material parameters:
`LowBandColor`, `HighBandColor`, `EmissiveMin`, and `EmissiveMax`.

Use a cyan-to-magenta band gradient as a starting palette. The colors remain
editable. Set the baseline emissive range to `0` to `10`. Separate the lit
surface color from glow so zero-response cubes can still be seen.

Use one shared dynamic material instance per visualizer actor. Reuse it
between evaluations; refresh it when the assigned parent material changes.
Snapshot and sanitize the settings on evaluation, order reversed limits,
and keep emissive limits finite and within `[0, 100]` for shader safety.
Non-finite bounds become zero; the saved baseline remains `[0, 10]`. Preserve the existing
mesh-default fallback when the optional material is unassigned.

### 2. Connect settings to the material — Codex

Update the visualizer so the shared material receives the emissive limits
from the assigned response settings. Keep the existing response in slot `0`
and band index in slot `1`.

Verify meaningful cases: response `0`, `0.5`, and `1`; changed emissive
settings; reordered and repeated times; changed material assignment; and
missing-material fallback. Confirm that material updates preserve the 32
instances and do not allocate a material per band or per evaluation.

If Unreal is open, save and close it before the native build. Build the
Development Editor target and run the affected tests plus the existing
RTCQT regression suite. Record test results and the implementation commit.

**Checkpoint 1:** code and material-parameter plumbing verified; D12 remains
in progress until the rendered material and saved scene pass.

### 3. Create the material — Unreal workstation, together

1. Open the project. In the Content Browser, create `Content/Materials`.
2. Create a Material named `M_RTCQT_Bands` and open it.
3. Set Material Domain to `Surface`, Blend Mode to `Opaque`, and Shading
   Model to `Default Lit`. Enable `Used with Instanced Static Meshes`.
4. Add two `PerInstanceCustomData` nodes, with Data Index `0` and `1`.
   Pass each through a `VertexInterpolator`, then `Saturate`. Call their
   outputs response `r` and normalized band index `b` in the documentation.
5. Add vector parameters `LowBandColor` and `HighBandColor`. Blend them
   with `Lerp`, using `b` as Alpha. This produces the band tint.
6. Multiply that tint by `Lerp(0.25, 1.0, r)` and connect it to Base Color.
   Set Metallic to `0` and Roughness to `0.65`.
7. Add scalar parameters `EmissiveMin = 0` and `EmissiveMax = 10`. Use
   `Lerp(EmissiveMin, EmissiveMax, r)`, multiply by the band tint, and
   connect the result to Emissive Color.
8. Apply and save. Require a successful material compile.

The graph uses one response value for color brightness and glow. Color
position comes from band order. The material preview alone cannot prove
per-instance variation; verify it on the actual visualizer.

Codex can author the same material through Unreal's native editor APIs and
leave the graph available for inspection. Keep this click-by-click recipe
as the manual path so Cam can follow and adjust it.

### 4. Assign the material — Cam, with Codex checking the scene

1. Open `Content/Audio/PrivateTest/L_D10D11ResponseTest`.
2. Select the existing visualizer actor in the Outliner.
3. In Details, under its assets, set `Instance Material` to
   `M_RTCQT_Bands`. Keep the existing analyzer, settings asset, mesh,
   band count, layout, and evaluation wiring.
4. Open the assigned `DA_ResponseTEst` and confirm Emissive Range is
   `0` to `10` and Response Exponent is `2`.
5. Save the material, settings, and level. Play at `16.75` using the
   existing explicit-time query.

Pass: the 32 cubes have a band-color gradient and glow correlated with
their heights; evaluation succeeds with `bands=32` and `reason=None`.
The existing printed prefix may still say D10 or D11.

If every cube has one color, check Data Index `1` and its connection to
the tint Lerp. If color varies but glow does not, check Data Index `0`
and its connection to the emissive Lerp. If the material is absent or
shows a compile fallback, inspect the compiler result and instanced-mesh
usage setting before continuing.

### 5. Lock scene exposure — Unreal workstation, together

1. Add a `Post Process Volume`, or configure the existing one if present.
2. Enable `Infinite Extent (Unbound)`.
3. Under Exposure, enable the overrides for Min EV100 and Max EV100 and
   set both to the same value. Start at `0` and adjust the shared value
   once while reviewing the reference stills.
4. Keep Exposure Compensation fixed. Check other volumes and any camera
   post-process overrides for conflicting exposure settings.
5. Use the game exposure settings for the editor viewport, then verify
   in Play as well. Keep the lighting and framing fixed between captures.
6. Tune the fixed exposure and maximum glow together if the strong frame
   loses its color. Quiet frames must still show the lit surface.
   Bloom is optional; if enabled, keep it subtle and fixed.
7. Record the final numeric exposure, emissive range, palette, and any
   bloom value. Save the level.

Equal exposure limits disable eye adaptation in the installed UE 5.8
implementation. The final exposure value is a measured scene choice,
not a different value for each timestamp.

### 6. Verify readable and repeatable frames — Codex + Cam

Use the existing D11 reference captures to start with:

| Timestamp | Measured response | Purpose |
| --- | --- | --- |
| `0.75` | mean `0.02035`, maximum `0.10380` | Quiet frame: small cubes remain visible. |
| `2.00` | mean `0.36119`, maximum `0.59600` | Moderate frame: height and color differences are readable. |
| `16.75` | mean `0.39803`, maximum `0.92922` | Strong peak within the sampled frames: colors remain distinguishable. |
| `33.25` | mean `0.42760`, maximum `0.79800` | Additional bright frame with higher average response. |

These labels describe the existing sampled frames, not the whole track's
absolute peak. Also use a controlled response `0`, `0.5`, and `1` fixture
to verify exact emissive endpoints `0`, `5`, and `10`.

Evaluate `16.75 -> 0.75 -> 2.00 -> 33.25 -> 16.75` in one active scene.
The verification helper can issue these explicit timestamp changes so
Cam does not need to rebuild the Blueprint for each capture. Watch the
floor/background for exposure adaptation during the changes, and retain
the final exposure configuration as evidence.

Require matching response/custom data and shared material parameters
when returning to `16.75`, with the same 32-instance topology. Review
the rendered stills for readability. Treat temporal rendering effects
separately from deterministic material inputs; do not promise identical
pixels solely from matching response values.

### 7. Save, reopen, and close the card — Codex

Verify the saved material assignment, response settings, and fixed
exposure in a fresh Unreal session. Retain the material asset and its
documentation, quiet/moderate/strong stills, focused logs, parameter
values, instance/material-count checks, and fresh-session evidence under
`planning/evidence/d12/`.

Keep private audio, analyzer, response settings, and test level under
ignored `Content/Audio/PrivateTest`. The original material can be tracked
under `Content/Materials`. Reproduce the private scene setup in the
documentation without committing its licensed assets.

Update D12 to `Done` only after the visual and saved-scene gates pass.
Commit the final assets, documentation, and evidence, and report the
checkpoint hash. Push when requested.

**Checkpoint 2:** D12 complete; D13 is unblocked for the sequence-driven
audio, time, and camera scene.

## Parallel work and boundaries

Material-palette choices and graph preparation can proceed while the
native code changes are prepared. Native compilation waits for Unreal
to release the project DLL. Final material assignment and exposure
validation depend on the successful build.

D12 proves the look at explicit timestamps. D13 adds the shared sequence
clock and synchronized audio; D14 proves rendered repeatability. Finish
D12's checkpoints before starting those delivery cards.
