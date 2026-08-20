# D05 Test Audio Manifest

## Selection

- **Track:** Good Kid — *No Time To Explain (Instrumental)*
- **Test fixture:** first 60.000 seconds of the instrumental WAV
- **Local fixture:** `Music/ContentIdSafe/test/no-time-to-explain-instrumental-00m00s-01m00s.wav`
- **Recoverable source:** `Music/ContentIdSafe/instrumental/01-no-time-to-explain.wav`
- **Intended use:** deterministic RTCQT development, private rehearsal, and the
  visualizer's stream/VOD soundtrack
- **Selected:** 2026-08-19

`Music/` is intentionally ignored by Git. The copyrighted audio is not part of
the source repository and does not require Git LFS. The manifest, extraction
recipe, technical fingerprints, and official recovery sources are tracked.

## Provenance and Permission

The user extracted the instrumental from the Good Kid creator album archive
into the local `Music/ContentIdSafe/` folder. Good Kid's official artist channel
publishes the statement [All Good Kid songs are Content ID free + DMCA
free](https://www.youtube.com/watch?v=D01fXyTr13U), and the band's official
[Creators Hub](https://goodkidofficial.com/creators/) is the recovery location
for the creator files. The official [Bandcamp track
page](https://goodkid.bandcamp.com/track/no-time-to-explain-2) identifies the
song, release, download format, and copyright status.

This is permission for the stated creator/stream use, not a transfer of
copyright and not permission to redistribute the audio file. Bandcamp labels
the work "all rights reserved." Keep the source and fixture private, use them
only with the visualizer output, and retain this evidence. Recheck the official
creator policy before the first public broadcast if its terms or the stream's
use change.

Conservative on-stream/VOD credit:

> Music: “No Time To Explain” (Instrumental) — Good Kid — used under Good
> Kid's Content ID/DMCA-free creator policy — goodkidofficial.com/creators

## Technical Identity

| Field | Full source | 60-second fixture |
| --- | ---: | ---: |
| Container/encoding | RIFF WAVE, PCM | RIFF WAVE, PCM |
| Channels | 2 (stereo) | 2 (stereo) |
| Sample rate | 44,100 Hz | 44,100 Hz |
| Bit depth | 16-bit | 16-bit |
| Frame count | 6,876,416 | 2,646,000 |
| Duration | 155.927800 s | 60.000000 s |
| File size | 27,725,842 bytes | 10,584,044 bytes |
| SHA-256 | `3d0bc05481420e8459674e2e101420021fa865f43d1edb7b1cada22111598213` | `818bc64d830301e32fc929bfa19fb4799b23295e22bdde4e0108826554eb8a23` |

The native 44.1 kHz/16-bit PCM file is accepted without resampling. It matches
the format advertised on Bandcamp and avoids introducing an unnecessary
conversion before Unreal performs ConstantQNRT analysis.

## Deterministic Extraction

From the repository root:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 planning/scripts/extract_wav_segment.py \
  Music/ContentIdSafe/instrumental/01-no-time-to-explain.wav \
  Music/ContentIdSafe/test/no-time-to-explain-instrumental-00m00s-01m00s.wav \
  --start 0 --duration 60
```

After extraction, verify that the fixture's SHA-256 matches the table. If the
creator archive must be recovered, download it again from the official Creators
Hub, restore the named full source, verify its source hash, and rerun the recipe.

## Reference Timestamps

The values below come from 1.0-second Hann windows on 0.25-second hops. Band
shares use bass 20–250 Hz, mid 250–4,000 Hz, and high 4,000–16,000 Hz. These
signal signatures provide repeatable checkpoints for D07 and later visual
comparison.

| Fixture time | Recognizable signal event | Measured signature | Expected visualizer check |
| ---: | --- | --- | --- |
| 00:00.750 | Quiet, high-weighted opening texture | -30.90 dBFS; high 42.33%; bass 0.02% | Low overall scale with upper bands visibly present |
| 00:02.000 | Mid-dominant opening material | -11.63 dBFS; mid 81.97%; bass 11.94% | Middle bands clearly outweigh bass |
| 00:11.750 | Strong section onset | +3.36 dB hop-to-hop gain; -13.01 dBFS | Attack/release response rises sharply and repeatably |
| 00:16.750 | Bass-dominant passage | -9.32 dBFS; bass 86.06%; mid 13.39% | Low-frequency instances dominate the layout |
| 00:33.250 | First-minute level peak | -8.74 dBFS; bass 78.45%; mid 18.45% | Dense, high-energy frame with no non-finite transforms |
| 00:44.250 | Later broad-band comparison point | -9.45 dBFS; high 4.71%; mid 33.69% | More upper-band activity than the 16.75-second bass checkpoint |

The analysis command is:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 planning/scripts/analyze_test_audio.py \
  Music/ContentIdSafe/test/no-time-to-explain-instrumental-00m00s-01m00s.wav
```

## Unreal Handoff

D07 should import the **60-second fixture**, not the full album source, into
`Content/Audio/`. Preserve its native sample rate and record Unreal's import
settings, the ConstantQNRT band count, and raw query results at every timestamp
above. If Unreal transforms the source during import, identify the imported
SoundWave by both its asset path and this fixture hash.
