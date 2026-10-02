# D13 evidence and reproduction

Authoritative workstation: `C:\Users\camch\source\RTCQT`, Unreal 5.8.
Branch: `codex/d13-sequencer-scene`; native checkpoint: `b58915f`.

D13 is Done. The native bridge passed all 25 RTCQT automation groups.
Saved-scene verification passed, and Cam confirmed music, cubes, camera,
and restart. The audible result is recorded separately in
`workstation-listening-result.json`; the original capture report retains
its Pending field from before that human check.
See `planning/documents/d13_sequencer_scene_checkpoint.md` for current gates
and `d13_sequencer_scene_workstation.md` for the GUI walkthrough.

## Private saved assets

- `/Game/Audio/PrivateTest/L_D13SequenceTest`
- `/Game/Audio/PrivateTest/LS_RTCQT_60s`
- `/Game/Audio/PrivateTest/MRQ_RTCQT_1080p60_Base`

These reference the existing licensed D07 SoundWave/analyzer and D11 settings.
They remain ignored. The original D12 level is preserved. The tracked helpers
reproduce and verify the private configuration on this workstation.

## Helper responsibilities

- `setup_d13_sequence_scene.py`: save a D13 copy, remove only its old frozen
  BeginPlay evaluator, and author the linear float track, audio, camera cut,
  Auto Play actor, and output baseline. It only replaces D13-tagged assets.
- `inspect_d13_saved_scene.py`: reopen and check saved track/output settings
  and project all 256 cube corners at maximum height into the camera view.
- `capture_d13_sequence_frames.py`: a fresh rendered editor enters Play,
  checks full forward playback and the actual camera view, moves the real
  sequence playheads through reordered/repeated reference frames, and saves
  eight stills. It saves no level or asset and restores its temporary fixture.
- `validate_d13_sequence_capture.py`: independently compare 224 captured band
  rows against the D07 vectors and D02 response calculation at tolerance 1e-6.
- `open_d13_sequence_scene.py`: leave the saved scene/timeline open for Cam's
  listening and scrubbing check, without saving any asset.

## Native launches from Bash/WSL

Close the editor before authoring or launching an isolated verification.
The Python and Sequencer plugins are enabled for these launches only.
From `/mnt/c/Users/camch/source/RTCQT`:

```bash
rtcqt_cmd='/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
rtcqt_editor='/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
rtcqt_project='C:/Users/camch/source/RTCQT/RTCQT.uproject'
"$rtcqt_cmd" "$rtcqt_project" -Unattended -NullRHI -NoSplash -NoSound \
  -EnablePlugins=PythonScriptPlugin,SequencerScripting \
  -ExecutePythonScript='C:/Users/camch/source/RTCQT/planning/scripts/setup_d13_sequence_scene.py' \
  -AbsLog='C:/Users/camch/source/RTCQT/Saved/Logs/D13-setup.log'
"$rtcqt_cmd" "$rtcqt_project" -Unattended -NullRHI -NoSplash -NoSound \
  -EnablePlugins=PythonScriptPlugin,SequencerScripting \
  -ExecutePythonScript='C:/Users/camch/source/RTCQT/planning/scripts/inspect_d13_saved_scene.py' \
  -AbsLog='C:/Users/camch/source/RTCQT/Saved/Logs/D13-inspection.log'
"$rtcqt_editor" "$rtcqt_project" -Unattended -NoSplash -Windowed -ResX=1280 -ResY=720 \
  -EnablePlugins=PythonScriptPlugin,SequencerScripting \
  -ExecutePythonScript='C:/Users/camch/source/RTCQT/planning/scripts/capture_d13_sequence_frames.py' \
  -AbsLog='C:/Users/camch/source/RTCQT/Saved/Logs/D13-capture.log'
PYTHONDONTWRITEBYTECODE=1 python3 planning/scripts/validate_d13_sequence_capture.py
```

The authoring, inspection, and capture helpers quit their isolated editor
when finished. The open helper deliberately leaves it running. Listening
uses the rendered editor with sound enabled and Unreal's main Play button.
The terminal endpoint at 60 seconds is a diagnostic; final output frame 3599
is inside the camera-cut range. Camera Cuts restores the gameplay view when
playback ends. Pixel-identical rendering and the two MRQ proof renders are
D14 work.
