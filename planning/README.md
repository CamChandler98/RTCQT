# RTCQT stream planning

This folder turns the RTCQT recovery work into a small two-board Kanban system,
following the structure used in the D2RSO Python planning folder.

## Files

- `csv/rtcqt_stream_roadmap_import.csv`: epic-level outcomes for a Roadmap board.
- `csv/rtcqt_stream_delivery_import.csv`: implementation-ready cards for a Delivery board.
- `documents/stream_roadmap_to_daily.md`: board rules, milestone map, and definitions of ready and done.

## Suggested GitHub Projects setup

Create two projects and import the matching CSV into each:

1. `RTCQT Stream - Roadmap`
2. `RTCQT Stream - Delivery`

Use these fields on both boards:

- Status: `Now`, `Next`, `Later`, `Done`
- Priority: `P0`, `P1`, `P2`
- Estimate: `S`, `M`, `L`
- Milestone: `Stream MVP`, `Polish`, `Live`

Keep no more than two Delivery cards in `Now`. The CSV starts with two cards
there: creating a recoverable Unreal project shell and preparing the first
licensed test track.

## First finish line

The first release is intentionally a pre-rendered, synchronized visual played
through OBS. It proves the viewer experience without requiring Unreal to remain
healthy during a live show. Live Unreal capture, custom RTCQT DSP, and Pixel
Streaming stay in `Later` until that path has passed a private stream test.
