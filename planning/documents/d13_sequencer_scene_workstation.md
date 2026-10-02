# D13 workstation check

Project: `C:\Users\camch\source\RTCQT\RTCQT.uproject`, Unreal 5.8.
Implementation checkpoint: `b58915f` on `codex/d13-sequencer-scene`.
The existing D12 level remains unchanged. D13 is Done; Cam confirmed music,
cubes, camera, and restart on 2026-10-02.

## Open the moving scene

1. In the Content Browser, open `Content/Audio/PrivateTest`.
2. Double-click `L_D13SequenceTest`.
3. Double-click `LS_RTCQT_60s` to open its Sequencer timeline.
4. Confirm the timeline shows 60 fps and frames 0 through 3600.
5. The visualizer track is `D08 - 32 Static Bands > Sequence Time Seconds`.
   Its two Linear keys are frame 0/value 0 and frame 3600/value 60.
6. The Audio track starts `SW_NoTimeToExplain_60s` at frame 0.
7. Camera Cuts uses `D08 - Layout Camera` for the full playback range.
   The camera's Auto Activate for Player is Disabled; the sequence performs
   the runtime camera handoff.

## Scrub and repeat

Use the Sequencer time field to visit these frame numbers:

| Song seconds | Sequencer frame |
| ---: | ---: |
| 0.75 | 45 |
| 2.00 | 120 |
| 11.75 | 705 |
| 16.75 | 1005 |
| 33.25 | 1995 |
| 44.25 | 2655 |

The cubes should change when the playhead moves. Visit 1005, then 45,
then return to 1005; the strong frame should return. On the visualizer's
Details panel, Sequence Time Seconds and Last Sequence Evaluation show
the assigned time and whether evaluation succeeded.

## Play and listen

1. Stop Sequencer preview if it is already running.
2. Use Unreal's main **Play** button. The saved `D13 - Sequence Player`
   automatically starts the 60-second sequence from zero at rate 1.
3. Expect the camera view to contain all 32 colored bands. The song should
   play once while the heights and glow change.
4. Listen at the opening and through the stronger section near 16.75 seconds.
   Confirm there is one song playing and the visual changes follow it.
5. Stop Play, then Play again; the sequence should restart from the opening.
6. This listening and restart check passed with Cam on 2026-10-02.
   Repeat it after any future audio/timeline changes.

If the cubes are frozen, confirm this is `L_D13SequenceTest` and that its
Level Sequence Actor assigns `LS_RTCQT_60s`. If music is absent, check the
workstation output volume and the Audio track's SoundWave assignment.
If the camera is wrong, inspect Camera Cuts and the assigned camera binding.

The sequence's end boundary at frame 3600 is exclusive: output contains
frames 0 through 3599. The float section extends through frame 3600 for
an exact 60-second diagnostic query, without adding an output frame.
`MRQ_RTCQT_1080p60_Base` stores the 1920x1080, 60 fps output baseline.
D14 adds its render format/sampling and performs the two proof renders.
