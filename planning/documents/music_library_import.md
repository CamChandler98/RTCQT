# Music library import

Run `planning/scripts/import_music_library.py` through Unreal Editor's Python
script runner with PythonScriptPlugin enabled, or from Git Bash:

```bash
"/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" \
  'C:/Users/camch/source/RTCQT/RTCQT.uproject' \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  -EnablePlugins=PythonScriptPlugin \
  -ExecutePythonScript='C:/Users/camch/source/RTCQT/planning/scripts/import_music_library.py'
```

The importer recursively selects WAVs in `Music`; MP3s are ignored. Each source
relative path gets a stable ID and distinct pair, including full/instrumental
versions and the 60-second fixture. New pairs live under
`/Game/Audio/PrivateTest/MusicLibrary`, following the private asset ignore policy.
It copies the saved D07 analyzer, then applies the saved library CQT defaults
when present (otherwise it uses D07's existing settings). Edit and run
`update_music_cqt_settings.py` to bulk tune the library. Reference
assets and scenes are untouched. Existing destination assets must carry the
importer's ownership metadata or the entire batch stops before importing.
Changed WAV bytes trigger reimport and analysis; unchanged pairs are reused.
Removed sources leave their assets on disk but leave the next catalog.

`Content/Audio/PrivateTest/MusicLibrary/track_catalog.json` contains stable IDs,
labels, relative source paths, source SHA-256, duration, SoundWave/analyzer object
paths, channel 0, and the inspected settings. A future dropdown can display
`label`, persist `id`, and load the object paths. This JSON is an editor-side
catalog; a packaged application will need explicit staging or a runtime asset
conversion. It is published atomically only after all tracks validate.

Analysis is asynchronous: the importer waits for `OnAnalysisComplete` before
saving the analyzer, checks its full duration against the SoundWave, and records
the analyzed source hash. Each pair must return the configured number of finite normalized values in [0, 1] at the start,
midpoint, and near the end. Results are saved to
`Saved/Automation/MusicLibrary/result.json`. Failures may leave completed pairs;
rerun after fixing the reported issue. No automatic deletions occur.

Live settings inspected on 2026-10-12: 40 Hz, 32 bands, 3.5 bands/octave,
0.01-second period, mono downmix, FFT 8192, Blackman, Power Spectrum,
bandwidth stretch 1, Equal Energy normalization, noise floor -60 dB.
The initial run's band-count-only check missed copied 60-second analysis data.
The corrected importer invalidates that old fingerprint and reanalyzes all
owned pairs once; subsequent unchanged runs reuse the completed results.
