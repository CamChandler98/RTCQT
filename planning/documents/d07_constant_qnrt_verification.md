# D07 ConstantQNRT Verification

## Result

D07 is complete. The 60-second fixture supplies 32 raw and normalized values
at all six reference timestamps. Every value is finite. Normalized values are
within `[0, 1]` and vary across both bands and timestamps. Changing the query
order and restarting Unreal preserve all 192 pairs exactly at the logged
precision (up to nine fractional digits; acceptance tolerance `1e-6`).

Verification was completed on 2026-09-22 in America/New_York. Run timestamps
below are UTC; the last run occurred after midnight UTC.

This verifies the pre-analyzed input. The runtime visualizer still contains
the D06 skeletons; D08-D12 implement the visual output.

## Assets and settings

- Engine: Unreal 5.8.0, changelist 55116800.
- Project: `C:\Users\camch\source\RTCQT\RTCQT.uproject`.
- SoundWave: `/Game/Audio/PrivateTest/SW_NoTimeToExplain_60s`.
- Analyzer: `/Game/Audio/PrivateTest/CQNRT_NoTimeToExplain_32`.
- Probe Blueprint: `/Game/Audio/PrivateTest/BP_D07ConstantQProbe`.
- Probe level: `/Game/Audio/PrivateTest/L_D07Probe`.
- Fixture: first 60 seconds of Good Kid, No Time To Explain (Instrumental),
  stereo, 44,100 Hz, 16-bit PCM. Provenance and extraction are in
  [the audio manifest](test_audio_manifest.md).
- Fixture SHA-256, rechecked during closeout:
  `818bc64d830301e32fc929bfa19fb4799b23295e22bdde4e0108826554eb8a23`.

| Analyzer setting | Value |
| --- | --- |
| Starting frequency | 40 Hz |
| Bands | 32 |
| Bands per octave | 3.5 |
| Analysis period | 0.01 seconds |
| Downmix to mono | Enabled |
| Query channel | 0 |
| FFT size | 8192 |
| Window | Blackman |
| Spectrum | Power Spectrum |
| Bandwidth stretch | 1.0 |
| CQT normalization | Equal Energy |
| Noise floor | -60 dB |

Saved screenshots show the
[analyzer settings](../evidence/d07/constant_qnrt_settings.png.png) and
[SoundWave import details](../evidence/d07/soundwave_details.png).
The user reported the final manual checks were good after being asked to
restart, rerun the probe, and preview the SoundWave separately. Playback is a
user-reported check; the assistant's captured evidence verifies analysis and
repeatability, not audible output.

## Query contract for D09 and D10

Both query methods take explicit seconds and channel 0:

- `GetChannelConstantQAtTime` returns the raw decibel-scaled spectrum.
- `GetNormalizedChannelConstantQAtTime` returns the intended `[0, 1]` input
  for the response processor.

The installed UE 5.8 implementation interpolates the spectrum at the requested
time and normalizes with the channel's maximum over the analyzed asset:

```text
range = channel_analysis_max_db - settings_noise_floor_db
normalized = clamp((raw_db - settings_noise_floor_db) / range, 0, 1)
```

When the range is too small or negative, Unreal returns zeroes. These are
normalized control values derived from decibels, not linear physical amplitude
or power. Pass the normalized query output directly to D09. D09's existing
noise-floor, weight, exponent, and clamp operations remain unchanged.
The raw spectrum's decibel reference is not the audio manifest's dBFS meter;
positive raw values do not by themselves indicate audio clipping.

The installed source is
`Engine/Plugins/Runtime/AudioSynesthesia/Source/AudioSynesthesia/Private/ConstantQNRT.cpp`.
The [runtime API contract](unreal_runtime_api_spec.md) already selects the
normalized method for D10.

## Captured runs

| Run | UTC start | Query order in seconds | Maximum raw / normalized difference from baseline |
| --- | --- | --- | --- |
| Baseline | 2026-09-22 21:32:08.684 | 0.75, 2, 11.75, 16.75, 33.25, 44.25 | 0 / 0 |
| Reordered | 2026-09-22 21:40:50.354 | 44.25, 33.25, 2, 0.75, 16.75, 11.75 | 0 / 0 |
| Reopened editor | 2026-09-23 00:03:39.899 | 44.25, 33.25, 2, 0.75, 16.75, 11.75 | 0 / 0 |

Each run contains 192 rows keyed by `(time_seconds, band_index)`, with each
index 0-31 present exactly once per timestamp. Raw and normalized summary
counts are both 32 at every timestamp. No related Blueprint/query errors were
found around the accepted runs. The reopened run comes from a new editor log
whose engine initialization completed at 2026-09-22 21:45:17.899 UTC, after the
baseline and reordered runs.

The reordered run includes both forward and backward jumps. Comparisons match
rows by timestamp and band, independently of emission order. CSV values retain
the logged precision; float comparisons found zero difference in every pair.
The quiet 0.75-second point has normalized values from 0 to 0.389965475;
the bass-heavy 16.75-second point ranges from 0.162777379 to 0.967562735.
The manifest's separate FFT measurements provide qualitative reference cues,
not exact numeric expectations for ConstantQNRT.

Evidence:

- [Baseline CSV](../evidence/d07/baseline.csv) and [log](../evidence/d07/baseline-log.txt).
- [Reordered CSV](../evidence/d07/reordered.csv) and [log](../evidence/d07/reordered-log.txt).
- [Reopened CSV](../evidence/d07/reopened.csv) and [log](../evidence/d07/reopened-log.txt).
- [Fresh editor startup](../evidence/d07/reopened-editor-startup.txt).
- [Machine-readable verification and fingerprints](../evidence/d07/verification.json).

## Rebuild the probe

The source audio and the four private Unreal assets remain under the existing
ignored storage policy. The tracked recipe and evidence allow reconstruction.
A fresh clone needs the private WAV restored using the audio manifest.

1. Import that fixture into `Content/Audio/PrivateTest` as
   `SW_NoTimeToExplain_60s`. Create `CQNRT_NoTimeToExplain_32`, assign the
   SoundWave, apply the settings above, wait for analysis, and save.
2. Create an Actor Blueprint named `BP_D07ConstantQProbe`. Add `Analyzer`, a
   ConstantQNRT object reference whose default is the analyzer asset. Add
   `ReferenceTimes`, a float array with the six baseline timestamps above.
3. Create both query nodes from the Analyzer getter. Connect the outer
   `For Each Loop` Array Element to both queries' `In Seconds`; both channels
   are 0. The outer loop Array input is ReferenceTimes.
4. Connect BeginPlay to that outer loop, then its Loop Body to a summary
   Print String. The summary's Format Text is:
   `time={Time} bands={Count} normalized_bands={NormalizedCount}`.
   Time comes from the outer Array Element. Count and NormalizedCount come
   from separate Length nodes on the respective query arrays.
5. Connect the summary Print String's execution output to a second, inner
   For Each Loop. Its Array is the raw query output. Its Array Index is the
   band index. Feed that SAME inner index into a Get (a copy) node on the
   normalized query array. The outer loop's index must not select bands.
6. Convert the inner Array Element (raw value) and the matching normalized
   Get result through separate To Text (Float) nodes. Set Maximum Fractional
   Digits to 9 and disable Use Grouping on both.
7. The inner Loop Body executes a Print String using this Format Text:
   `D07 time={Time} band={Band} raw_db={Raw} normalized={Normalized}`.
   Time comes from the outer Array Element; Band from the inner Array Index;
   Raw and Normalized from their respective text conversion outputs. Disable
   Print to Screen and enable Print to Log on this per-band Print String.
8. Save `L_D07Probe` with one instance of the probe Actor. Compile, save, and
   Play once, then stop. The query nodes are pure: their data connections
   evaluate when the Print String nodes need the values.
9. Capture a baseline, change ReferenceTimes to the reordered sequence in the
   table, and run again. Save All, fully close and reopen Unreal, open the
   probe level, and run again. Capture startup evidence as well as the last
   run. Preview the SoundWave separately to check audible playback.
10. Parse the `D07` log lines into the four CSV columns shown in the evidence.
    Require exactly the six timestamps and 32 unique bands per timestamp,
    finite values, normalized values in `[0, 1]`, and matching raw/normalized
    arrays. Compare each reordered/reopened row against the baseline by
    `(time_seconds, band_index)` with an absolute tolerance of `1e-6`.

## Next delivery card

D08 is the next implementation checkpoint: create 32 static mesh instances
using the line-layout contract. D09 is unblocked and may be pulled when useful.
D10 still waits for D08 before integrating these verified queries with the
visualizer actor.
