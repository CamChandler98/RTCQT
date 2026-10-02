# D13 Sequencer Scene Checkpoints

Date: 2026-10-02, America/New_York.
Branch: `codex/d13-sequencer-scene`.
Base: D12 complete at `3f1be90`.
Status: **Done**. Cam confirmed the final audible preview and restart on
2026-10-02; all native, saved-scene, visual, and listening gates pass.
Native implementation checkpoint: `b58915f`.

## Checkpoint 1: native time bridge

The Development Editor build and all 25 RTCQT automation groups pass with
zero test errors/warnings. `SequenceTimeSeconds` uses the native setter
`SetSequenceTimeSeconds`, which calls `EvaluateAtTime` and records transient
`LastSequenceEvaluation` diagnostics. No actor tick or audio clock is added.

Three real Level Sequence player tests verify all six reference times,
reordered and repeated seeks, independent 32-band response/height values,
subframes, start/end boundaries, pause, Keep State at stop, explicit restart,
one shared material, unchanged topology, and failure/clear/recovery.
Transient fixtures use explicit bindings and initialize their game world
for Play; the persistent saved scene is verified separately.

The original 22 D08-D12 groups remain green. The native actor remains free
of editor or MovieScene dependencies; only editor development tests link
the three sequence modules.

Evidence: `planning/evidence/d13/checkpoint1-build-log.txt`,
`checkpoint1-automation.json`, `checkpoint1-automation-log.txt`, and
`checkpoint1-verification.json`.

## Checkpoint 2: saved scene

The saved timeline, visual gates, and workstation listening result all pass.
D13 is `Done`; D14 is the next delivery card.

### Saved scene and timeline

The guarded authoring helper creates only the D13 copy and D13-tagged assets:

- `/Game/Audio/PrivateTest/L_D13SequenceTest`
- `/Game/Audio/PrivateTest/LS_RTCQT_60s`
- `/Game/Audio/PrivateTest/MRQ_RTCQT_1080p60_Base`

The copied Level Blueprint's old BeginPlay -> frozen 16.75 evaluator is
disconnected. Unrelated graph nodes remain. The original D12 level's SHA256
remains `221cded94bd00efae8f07838936a874c5d36db47818ea8ebff52573fc5d89b17`.

The sequence uses display rate 60/1, tick resolution 24000/1, subframe
evaluation, Tick clock, play rate 1, and playback frames `[0, 3600)`.
One existing-actor float track animates `SequenceTimeSeconds` with exactly
two Linear keys: frame 0/value 0 and frame 3600/value 60. Extrapolation is
constant and the time section uses Keep State. That section extends to
exclusive frame 3601 so the exact 60-second diagnostic can evaluate; the
sequence output still contains exactly frames 0 through 3599.

The existing analyzer SoundWave starts at frame 0, offset 0, with both audio
looping/repeating disabled. Audio and Camera Cuts cover `[0, 3600)`.
One `D13 - Sequence Player` assigns the sequence, enables Auto Play, and
uses start offset 0 with no random start or looping.

`D08 - Layout Camera` keeps its oblique transform, uses FOV 65 and constrained
16:9 aspect, and has no exposure overrides. Its old Auto Activate for Player
setting is Disabled so Camera Cuts performs the runtime camera handoff.
The unbound D12 exposure volume keeps Min EV100 = Max EV100 = 2. The minimal
output config stores 1920x1080 and 60/1 fps with sequence-governed range.
D14 will choose render format/sampling and perform the two proof renders.

### Fresh playback, seeking, and framing

A separate read-only editor inspects the persisted bindings, keys, sections,
audio assignment, playback settings, output baseline, and disabled frozen
BeginPlay branch. Camera projection checks all 256 cube corners for the
32 instances at maximum Z scale 4. Maximum absolute NDC is
0.6810010119269847 horizontally and 0.14276418729587745 vertically, both
below the required 0.95 limit.

A fresh rendered editor loads the saved D13 level and enters main Play.
Auto Play advances through the full 60 seconds. Thirteen timeline samples
match evaluated time within 1e-4 seconds; all twelve samples while playing
also verify the actual player-controller view target is the bound camera.
Camera Cuts restores the gameplay view when the exclusive endpoint is
reached. A separate seek checks that final output frame 3599 uses the camera.

The runtime player then seeks frames `1005 -> 45 -> 2655 -> 120 -> 705 ->
1995 -> 1005`, and checks frame 0, frame 3599, and exact time 60. Pause, stop
with retained visual state, and an explicit restart at zero pass. The editor
Sequencer playhead performs the same seven reference seeks. Both worlds
produce exactly matching response inputs and transforms; each world owns
one shared MID, with one instanced component and 32 instances. Returning to
1005 reproduces exactly the earlier data in each world.

The independent validator checks 224 band rows against D07's normalized
vectors using D02's quadratic response calculation. At absolute/relative
tolerance 1e-6, maximum response difference is 2.955406541715888e-08 and
maximum Z-scale difference is 2.1136459515957995e-07.

Eight fresh 1920x1080 camera stills were captured and visually reviewed:
all six reference times, a repeated 16.75 frame, and an explicitly synthetic
all-maximum-height fixture. All 32 colored bands fit the frame. Quiet bands
retain visible surfaces; strong and later frames show distinct heights and
color. The repeated strong frame restores the same layout and appearance.
The temporary maximum fixture is restored to real sequence evaluation.
No asset or level is saved during capture; all seven monitored private/source
asset hashes remain unchanged. Pixel-identical temporal rendering is not
claimed; D14 owns that separate render comparison.

Native source fingerprints match checkpoint 1, so the already passing
25-group suite remains applicable. The final authoring, inspection, and
capture sessions finish successfully with no errors, ensures, or fatals.
Authoring records one deprecated `set_current_time` call warning. The rendered
session records Unreal's startup `r.MotionVectorSimulation` render-thread
warning; both are retained in the focused log. All automation groups have
zero test warnings/errors.

### Retained evidence and workstation result

Under `planning/evidence/d13/`:

- `checkpoint2-sequence-scene-setup.json`
- `checkpoint2-saved-scene-inspection.json`
- `checkpoint2-fresh-sequence-frames.json`
- `checkpoint2-capture-validation.json`
- `checkpoint2-native-log.txt` and `checkpoint2-verification.json`
- `frame-quiet.png`, `frame-moderate.png`, `frame-onset.png`
- `frame-strong-first.png`, `frame-bright.png`, `frame-late.png`
- `frame-strong-repeat.png`, `frame-maximum-height.png`
- `workstation-listening-result.json`, Passed

The level, sequence, output baseline, audio, analyzer, and settings remain in
the ignored private fixture folder. Tracked native authoring/inspection/
capture/validation helpers and the evidence README document reproduction.

Cam used the saved D13 scene and confirmed: "Yes—music, cubes, camera, and
restart work." This directly satisfies the audible forward-preview and
restart gate. The isolated capture report retains its original Pending
listening field; the subsequent human result is recorded separately in
`workstation-listening-result.json` and the final verification receipt.
The exact GUI walkthrough is in
[d13_sequencer_scene_workstation.md](d13_sequencer_scene_workstation.md).
Checkpoint 2 and D13 are complete. D14 remains `Next`.
