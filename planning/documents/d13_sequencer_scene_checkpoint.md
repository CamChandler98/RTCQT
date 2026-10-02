# D13 Sequencer Scene Checkpoints

Date: 2026-10-02, America/New_York.
Branch: `codex/d13-sequencer-scene`.
Base: D12 complete at `3f1be90`.
Status: **In progress**.

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

Pending guarded D13 level/sequence/output authoring, real Sequencer playhead
captures at every reference, camera framing, full forward preview, fresh
session verification, and a workstation audible check. D13 stays `Now`.
