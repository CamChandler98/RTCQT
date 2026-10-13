# Bulk CQT tweaks

1. Stop Play in Unreal.
2. Open `planning/scripts/update_music_cqt_settings.py` in a text editor.
3. Change the values in `DEFAULTS` near the top and save the file.
4. In Unreal, choose **Tools > Execute Python Script**, then select that file.
   PythonScriptPlugin must be enabled (Edit > Plugins > Python Editor Script Plugin).
5. Watch **Output Log** until `Music library complete` appears. Analysis runs
   asynchronously; leave the editor open until it finishes. Start Music Lab again.

All 11 settings shown in the analyzer panel are exposed. FFT sizes and the
window/spectrum/normalization choices use the enum names listed in the script.
Numeric values are checked against the installed engine's allowed ranges before
assets change. Restore the original values shown in the script and rerun to
return to the baseline.

The script applies defaults to the MusicLibrary pairs, waits for completed
analysis, checks duration and beginning/midpoint/end results, then saves. Each
library analyzer receives its own settings object. The original D07 analyzer
and D13 offline scene retain their baseline settings.

Chosen defaults are saved in the private library's `cqt_settings.json`; future
imports honor them. The existing `track_catalog.json` format and track IDs are
retained. Unchanged reruns reuse completed analysis. An interrupted batch can
be rerun; the profile and catalog are published after all entries validate.

Music Lab accepts **1–96 bands** and rebuilds the bars from the selected
analyzer's count. The line keeps its original center-to-center span; higher
counts use narrower bars. To keep similar frequency coverage, increasing
32 bands / 3.5 bands per octave to 64 bands / 7 bands per octave is a useful
starting point. Band count alone does not fix the highest represented frequency.
With mono downmix off,
the demo still queries channel 0. This changes analysis settings; the separate
visual response settings (weights, exponent, scale, emissive) remain independent.
