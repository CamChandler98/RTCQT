# RTCQT Stream Solo Project Playbook

## Purpose

This document is the source of truth for moving the surviving RTCQT ideas from
old Unreal C++ source to a reliable first stream.

The checkout is not currently a complete Unreal project. It does not contain a
`.uproject`, target files, `Content/`, maps, materials, Blueprints, or a proven
engine version. A card is not complete merely because the old source exists.

The first stream path is:

```text
licensed WAV -> ConstantQNRT -> deterministic mesh visualizer
-> Movie Render Queue -> synchronized media file -> OBS
```

Custom RTCQT analysis and live hosting are later experiments, not blockers for
the Stream MVP.

## Board Structure

### 1. Roadmap Board

**Name:** `RTCQT Stream - Roadmap`
**Level:** Epics and outcomes only

Status columns:

- `Now`
- `Next`
- `Later`
- `Done`

Required fields:

- Priority: `P0`, `P1`, `P2`
- Estimate: `S`, `M`, `L`
- Milestone: `Stream MVP`, `Polish`, `Live`

### 2. Delivery Board

**Name:** `RTCQT Stream - Delivery`
**Level:** Implementation cards only

Use the same columns and fields as the Roadmap board.

WIP rule:

- Maximum two cards in `Now`.
- Pull work from the active Roadmap epic before starting another epic.
- A blocked card gets a note within one work session and is split if possible.

## Roadmap To Delivery Map

### Epic: Stream MVP - Project and Analysis Foundation

- [ ] Create a recoverable Unreal project shell.
- [ ] Prepare a licensed test track and ConstantQNRT asset.

### Epic: Stream MVP - Deterministic Visualizer

- [ ] Build the first ConstantQNRT line visualizer.
- [ ] Lock playback and analysis to one clock.
- [ ] Add spectrum smoothing and response controls.
- [ ] Build the emissive material and exposure baseline.

### Epic: Stream MVP - Render and OBS Launch

- [ ] Build a track-length Level Sequence.
- [ ] Render a short synchronization proof.
- [ ] Produce the final synchronized media file.
- [ ] Build the OBS scene and audio routing.
- [ ] Pass a private stream rehearsal.
- [ ] Write the live-show runbook and fallback scene.

### Epic: Polish - RTCQT Visual Language

- [ ] Add circle and spiral layouts.
- [ ] Evaluate the old RTCQT processing against the NRT baseline.
- [ ] Port only processing that produces a visible improvement.

### Epic: Live - Real-Time And Interactive Delivery

- [ ] Benchmark live Unreal capture in OBS.
- [ ] Prototype Pixel Streaming on the local network.

## Definition Of Ready

A Delivery card can enter `Now` only if:

- [ ] Its objective names one observable outcome.
- [ ] Dependencies and required assets are available.
- [ ] Audio licensing or broadcast permission is known when audio is involved.
- [ ] Acceptance criteria can be checked without relying on taste alone.
- [ ] The work fits one or two focused sessions, or has been split.

## Definition Of Done

A Delivery card is `Done` only if:

- [ ] The asset, code, configuration, or document is saved in a recoverable location.
- [ ] The listed automated or manual checks pass.
- [ ] Engine version and important plugin/settings changes are recorded.
- [ ] Screenshots, logs, or rendered output are attached when the result is visual.
- [ ] Follow-up work is captured as a separate card.

## Weekly Planning Ritual

1. Put exactly one Roadmap epic in `Now`.
2. Pull one or two unblocked Delivery cards from that epic into `Now`.
3. Write a finish line that describes something observable.
4. Review storage, licensing, build, GPU, and audio-routing risks.
5. End the week by moving completed cards and recording blockers.

Example finish line:

> A 30-second licensed test track drives the same cube motion on three runs.

## Stream MVP Gate

Do not start public live-Unreal or Pixel Streaming work until all of these are
true:

- [ ] A deterministic visualizer reacts to a licensed track.
- [ ] Movie Render Queue produces a clean render.
- [ ] The final media file is synchronized at the start, middle, and end.
- [ ] OBS receives the intended video and audio exactly once.
- [ ] A private stream rehearsal completes without dropped media or bad routing.
- [ ] A fallback scene can replace the visual within a few seconds.

## Delivery Card Template

```md
## Objective
[Observable outcome]

## Epic
[Roadmap epic]

## Scope
- [Bounded task]
- [Bounded task]

## Acceptance Criteria
- [ ] [Testable condition]
- [ ] [Testable condition]

## Dependencies
- [Required card, asset, tool, or decision]

## Out Of Scope
- [Work deliberately deferred]

## Evidence
- [Log, screenshot, render, or test result to attach]
```
