# D13: Sequencer, audio, and camera plan

Prepared: 2026-10-02, America/New_York.
Status: In progress; implementation began on 2026-10-02.
Starting checkpoint: D12 complete at `3f1be90` on `codex/d12-material-exposure`.

## Finish line and prerequisites

Scrubbing one Level Sequence changes all 32 cubes to the correct song frame.
Playing that sequence runs the existing 60-second instrumental, the visualizer,
and the camera together. Repeating or reversing a seek reproduces the same
response, transforms, and material inputs.

Required delivery cards D05, D11, and D12 are all `Done`. The existing D07/D10
analyzer and exact-time evaluator provide the foundation. Work in
`C:\Users\camch\source\RTCQT` using Unreal 5.8; the WSL source-only checkout
is not the Unreal workstation. Start `codex/d13-sequencer-scene` from the
verified D12 checkpoint when implementation begins.

## Selected baseline

| Setting | Planned value |
| --- | --- |
| Song range | Fixture seconds 0 to 60 |
| Sequence display/output frame rate | 60/1 fps |
| Sequence tick resolution | 24,000/1 ticks per second |
| Evaluation | With subframes; preserve seek and later render sampling |
| Playback range | Frames `[0, 3600)`; exactly 3,600 output frames |
| Last included output frame | 3599, at 59.983333333 seconds |
| Output target | 1920 x 1080, camera aspect 16:9 |
| Play rate | 1.0 |
| Automatic looping | Off for verification |
| Response Exponent | 2 |
| Emissive Range | 0 to 10 |
| Fixed exposure | Min EV100 = Max EV100 = 2 |

The end of the sequence is an exclusive boundary. A time key at frame 3600
sets the interpolation endpoint to 60 seconds; it does not create an extra
output frame. The exact analyzer endpoint at 60 is also tested separately.

Sequencer owns the frame position. The visualizer consumes its assigned
seconds value and retains the existing stateless response pipeline. Audio
starts on the same timeline; audible playback position is not queried as the
visual clock.

## Step 1: Add the native time bridge — Codex

Add an animatable float property `SequenceTimeSeconds` to the existing
visualizer and a matching native setter `SetSequenceTimeSeconds`.
The setter delegates the supplied value to `EvaluateAtTime`; it does not
accumulate delta time, advance a separate clock, or duplicate the query/math.
Keep evaluation diagnostics available for verification without logging each
playback frame.

Use the built-in float property track with a native setter. The installed
UE 5.8 property registry checks for `Set<PropertyName>` and routes that
property through its setter rather than the direct-offset optimization.
This source check supports the proposed route; its actual editor/runtime
behavior must pass the implementation gate.

Verify a real MovieScene/LevelSequence float track invokes the setter and
changes the rendered instance data. Merely changing the property value in
Details or observing that its numeric value animates is insufficient.

Test forward/reverse/isolated evaluation, fractional-frame evaluation,
repeated times, start/end boundaries, pause/restart behavior, and failure
recovery. Preserve 32 instances and one shared material. Keep editor-test
dependencies separate from the actor's runtime requirements where possible.

## Step 2: Build and verify the bridge — Codex

If Unreal is open, save and close it before the native build. Build the
Development Editor target, run the new integration tests and existing RTCQT
regressions, and retain the results.

**Checkpoint 1:** tested time bridge committed, with its hash reported.
D13 remains in progress until the saved timeline, visible scrubbing, audio
preview, and fresh-session gates also pass.

While the bridge is prepared, the frame/timestamp table and asset setup
recipe can be prepared independently. Native compilation requires the
editor to release the DLL. Final track wiring depends on the successful build.

## Step 3: Create the D13 scene and sequence — Unreal workstation, together

1. Open the existing `Content/Audio/PrivateTest/L_D10D11ResponseTest` level.
2. Duplicate/save a D13 working copy as
   `Content/Audio/PrivateTest/L_D13SequenceTest`. Preserve the verified D12
   level. The copy keeps the visualizer, ground, lighting, material, response
   settings, camera, and fixed-exposure volume.
3. In the D13 copy only, remove or disable the old Level Blueprint's
   BeginPlay call that forces a frozen `16.75` evaluation. Sequencer will
   drive every frame instead. Preserve unrelated Blueprint logic.
4. Create a Level Sequence named `LS_RTCQT_60s` in the same PrivateTest
   folder, then open it in Sequencer.
5. Set its display rate to `60 fps`, tick resolution to `24000`, and
   playback range to start `0`, end `3600`. Use subframe evaluation.
6. Place one Level Sequence Actor, label it `D13 - Sequence Player`, and
   assign `LS_RTCQT_60s`. Use play rate `1`, no looping, and start offset `0`.
   Enable Auto Play for the saved-level Play check.

Codex can create and verify these assets with native Unreal editor APIs.
Keep this manual recipe so Cam can inspect and adjust the timeline.

## Step 4: Wire the visualizer track — Unreal workstation, together

1. Drag the existing visualizer from the Outliner into Sequencer. Bind it
   as the existing actor, not a spawned duplicate.
2. Under that actor's tracks, add `Sequence Time Seconds`.
3. Add exactly these keys, with **Linear** interpolation:

   | Display frame | Property value, seconds |
   | ---: | ---: |
   | 0 | 0 |
   | 3600 | 60 |

4. Extend the section across the full playback range. Use constant
   pre/post extrapolation; do not use an eased curve, cycle, or offset loop.
5. Use Keep State for the song-time section so stopping can retain the
   last evaluated frame. A restart explicitly returns the sequence to zero.
   Verify the actual stop/restart and editor-close behavior rather than
   assuming an untested restoration path.
6. Scrub to frame `1005`. Expect a `16.75`-second evaluation, 32 instances,
   and the verified D12 strong-frame response/material inputs.

If the seconds value moves but the cubes remain frozen, stop here and fix
the setter/evaluation path. Changing time interpolation to Auto/Cubic would
desynchronize visual time from the audio even if both endpoints are correct.

## Step 5: Add synchronized preview audio — Unreal workstation, together

1. Add an Audio track to `LS_RTCQT_60s`.
2. Add the existing `SW_NoTimeToExplain_60s` SoundWave used by the analyzer.
3. Start its section at display frame `0`, with sound start offset `0`,
   and cover the 60-second sequence range. Use the intended volume without
   adding a second audio-playback path in the level.
4. Play forward at rate `1.0`. The opening, 11.75-second onset, and
   16.75-second bass section should agree with the visual changes.
5. Check audible playback on the workstation as well as the sequence's
   audio-section settings. If native automation cannot confirm the audible
   path, Cam supplies that short listening check before closing the card.

Audio may behave differently while dragging or seeking than during forward
playback. The acceptance gate is synchronized forward preview with correct
section offset and deterministic visual seeking. D15 later muxes the
approved source audio exactly once into the delivery media.

## Step 6: Bind the camera and output baseline — Unreal workstation, together

1. Reuse `D08 - Layout Camera` as the initial camera. Bind it to the
   sequence and add a Camera Cuts section covering frames `[0, 3600)`.
2. View through that camera. Set its aspect to `16:9`; refine its framing
   so all 32 bands, including the maximum configured height, fit with a
   small margin. Keep the frame static for the baseline.
3. Confirm the camera has no conflicting exposure overrides and the
   unbound volume still fixes EV100 at `2`.
4. Save a minimal output configuration named `MRQ_RTCQT_1080p60_Base` in
   PrivateTest with output resolution `1920 x 1080` and frame rate `60/1`.
   Leave its range governed by the sequence. Resolution belongs to output
   settings, not to the Level Sequence asset itself.
5. Capture reference stills at exactly `1920 x 1080`. D14 extends this
   output baseline with render format, sampling, and the two proof renders.

Test framing against both the actual reference frames and a temporary
all-maximum-height fixture. Restore real sequence evaluation afterward;
do not save synthetic custom data as the timeline result.

## Step 7: Verify every reference timestamp — Codex + Cam

All six D07 timestamps land on exact output frames at 60 fps:

| Fixture seconds | Display frame |
| ---: | ---: |
| 0.750 | 45 |
| 2.000 | 120 |
| 11.750 | 705 |
| 16.750 | 1005 |
| 33.250 | 1995 |
| 44.250 | 2655 |

Seek in the sequence to frames `1005 -> 45 -> 2655 -> 120 -> 705 -> 1995
-> 1005`. The verification must move Sequencer's playhead and allow its
track to drive the actor; directly calling `EvaluateAtTime` cannot substitute
for this gate.

Compare the resulting response, transforms, and emissive parameters to
independent direct/reference evaluations. Confirm the repeated 1005 frame
restores matching inputs, with one visualizer, one instanced component,
32 instances, and one active shared material. Save the six reference stills.

Also check frame `0`, final output frame `3599`, exact time `60`, a pause,
a backward seek, and a restart. Full forward playback should cover the
60-second fixture with matching audio and camera cuts. Record sequence
settings, track keys, audio placement, and evaluation results.

## Step 8: Save, reopen, and close D13 — Codex

Save the D13 level, sequence, and output baseline. Reopen them in a fresh
Unreal session and repeat key seeks plus the saved-level Play check. Do not
mark D13 complete based only on a successful C++ build or asset creation.

Retain the build/test results, sequence/output settings, track and audio
metadata, reference stills, numeric comparisons, fresh-session evidence,
and workstation audio-preview result under `planning/evidence/d13/`.
The scene, sequence, and output config reference private licensed fixtures
and remain under ignored `Content/Audio/PrivateTest`. Track source, native
authoring/verification scripts, asset reproduction instructions, and evidence.

**Checkpoint 2:** saved sequence, visual seeking, camera framing, and audible
preview verified; D13 marked `Done` and the final checkpoint hash reported.
Push when requested. D14 then owns the repeated 30-second MRQ proof renders;
D15 produces the synchronized media file and D16 places it in OBS.
