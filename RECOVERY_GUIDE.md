# RTCQT recovery and streaming roadmap

For a focused current-Unreal rebuild plan based on the UE 5.8 compiler errors,
see `UE5_VISUALIZER_REBUILD_GUIDE.md`.

This repository is the source code for the `SynRTCQT` Unreal C++ module. It is
not a complete Unreal project: the `.uproject`, `Content/` assets, maps,
Blueprints, and project configuration are not present in Git.

The recovery strategy is therefore:

1. Prove the old module builds in a fresh project.
2. Recreate one minimal audio-reactive scene.
3. Make it deterministic with pre-analyzed audio and render it offline.
4. Put that result on stream.
5. Only then turn it into an interactive online room.

## What survived

- Constant-Q transform (CQT) analysis and post-processing.
- `ASpectrumManager`, which accepts float audio samples through the
  Blueprint-callable `AnalyzeAudio` function and broadcasts spectrum updates.
- `AMeshController` and `ASoundMesh`, which can create and update rows, circles,
  spheres, and spirals of reactive meshes.
- Blueprint-exposed analyzer and processing settings.
- A save-game object for spectrum settings and some UMG control code.

What did not survive is the glue that obtains audio data and calls
`AnalyzeAudio`, plus all visual assets and Blueprint wiring. The old history
contains a class named `RealTimeCQTManager`, but it was deliberately removed in
the January 2, 2024 refactor. Do not begin recovery from that deleted class.

## Is non-realtime visualization possible?

Yes. There are two useful meanings of "non-realtime," and both are good fits.

### 1. Pre-analyzed audio with live playback

Unreal's Audio Synesthesia plugin provides a `ConstantQNRT` analyzer. It
analyzes an imported `SoundWave` ahead of time and lets a Blueprint query the
frequency-band values at a particular playback time. This is the shortest path
to a reliable first visualization because analysis cannot fall behind during
playback.

For the first recovery milestone, use this engine-provided NRT analyzer rather
than immediately adapting the custom C++ analyzer. Once the scene works, the
custom RTCQT processing can be brought back where it provides a visible benefit.

### 2. Fully offline video rendering

Drive the scene from the pre-analyzed track in Sequencer, then render it with
Movie Render Queue. This can render more slowly than playback and produce a
clean image sequence/video. Add the original music in the final edit or OBS so
audio stays perfectly aligned.

For the streaming idea, this is the recommended first release: play the
finished visual video on stream before at
tempting to host Unreal continuously.

## Choose an engine version

The code was written between April 2023 and January 2024 and uses UE5-era Audio
Synesthesia and Signal Processing APIs. No project descriptor survived, so the
exact engine version cannot be proven.

Recommended recovery order:

1. Try **Unreal Engine 5.2** with Visual Studio 2022. It is the closest likely
   compatibility target for code developed through mid-2023.
2. If the Launcher does not offer 5.2, or if current tooling makes it painful,
   use a current stable UE version and treat compilation errors as a small port.
3. Do not use a Preview engine build for recovery.

Keep the first successful engine version recorded in the `.uproject` and add it
to this guide.

## Install prerequisites (Windows)

Unreal development is best recovered on Windows because that is also the
simplest eventual OBS and Pixel Streaming host.

1. Install the Epic Games Launcher.
2. In **Unreal Engine > Library**, add and install the chosen engine version.
   Include Starter Content if you want placeholder meshes and materials.
3. Install Visual Studio with **Game development with C++**, a Windows 10/11
   SDK, and the Unreal tooling offered by the installer.
4. Clone this repository. Preserve it as the source of truth; do not paste the
   only copy into an untracked test project.

## Recreate the project shell

The module name must remain `SynRTCQT` because its exported classes use the
`SYNRTCQT_API` macro.

1. Launch Unreal and create a **Games > Blank > C++** project named
   `SynRTCQT`. Starter Content is optional.
2. Close Unreal and Visual Studio.
3. In the new project, locate `Source/SynRTCQT/`.
4. Keep the generated target files in `Source/`, but replace the contents of
   `Source/SynRTCQT/` with the contents of this repository.
5. Ensure the resulting paths look like:

   ```text
   SynRTCQT.uproject
   Source/
     SynRTCQT.Target.cs
     SynRTCQTEditor.Target.cs
     SynRTCQT/
       SynRTCQT.Build.cs
       SynRTCQT.cpp
       SynRTCQT.h
       SpectrumManager.cpp
       AnalyzerComponents/
       CoreDSP/
       MeshController/
       ...
   ```

6. Open the `.uproject`, agree to rebuild, and build the **Development Editor**
   target.

Do not add old Marketplace audio-analyzer plugins yet. Their references in
`SynRTCQT.Build.cs` are commented out and the current analyzer is based on
Unreal's own audio modules.

## Expected first-build triage

The first build may expose engine-version drift. Fix one first error at a time,
not the cascade shown in Visual Studio's Error List.

Likely issues:

- Enable the **Audio Synesthesia** plugin in the Unreal Plugin browser.
- `AudioSynesthesiaEditor` is currently listed as a public runtime dependency.
  If it blocks a non-editor or packaged build, make it editor-only.
- `AMeshController` uses `FPropertyEditorModule` inside `#if WITH_EDITOR`; a
  newer engine may require an editor-only `PropertyEditor` dependency and the
  corresponding header, or that cosmetic Details-panel section can be removed.
- Internal DSP APIs such as `FPseudoConstantQ` may have changed between UE
  versions. Record the exact compiler error before adapting it.

The first checkpoint is simply: the editor opens and `SpectrumManager` and
`MeshController` appear as placeable C++ actors.

## Minimal non-realtime proof of concept

Do this before rebuilding the custom UI or the old visual design.

1. Enable **Audio Synesthesia** and **Movie Render Queue**; restart the editor.
2. Import a short WAV file that you own or have permission to broadcast.
3. Create a **ConstantQNRT** analyzer asset for that `SoundWave` and configure a
   modest band count (start with 32 or 48).
4. Create a Blueprint actor named `BP_NRTVisualizer` containing instanced cubes
   or another cheap mesh.
5. At construction/begin play, create one mesh per frequency band.
6. Start the `SoundWave` and retain its playback start time.
7. On Tick, calculate the song time and query the ConstantQNRT values at that
   time. Map each band value to mesh Z scale and/or material emissive strength.
8. Add a camera and light, press Play, and confirm the same portion of the song
   produces the same motion on repeated runs.

Keep audio playback and analysis on one clock. Avoid separately starting the
song and a free-running animation timer, because they will drift.

### Then reconnect this repo's visual code

Once the NRT Blueprint works:

1. Replace the Blueprint's mesh creation with `AMeshController`, or copy its
   useful layouts into an instanced-mesh Blueprint/C++ component.
2. Bind spectrum indices to `UpdateMeshZ`, `UpdateMeshBrightness`, or
   `UpdateMeshColorHSV`.
3. Compare the built-in ConstantQNRT look against `ASpectrumManager` processing.
4. Only port custom RTCQT features that make the output materially better.

This avoids making the custom DSP build a prerequisite for validating the art
and stream concept.

## Make the first stream-ready visual

1. Put the camera and visualizer in a Level Sequence whose duration exactly
   matches the track.
2. Use Movie Render Queue to render a PNG/EXR sequence or a supported video
   format. An image sequence is safer because an interrupted render can resume
   and individual bad frames can be replaced.
3. Combine the rendered frames with the original WAV in an editor or `ffmpeg`.
4. Check sync at the beginning, middle, and end.
5. Add the finished video as an OBS Media Source and test it privately.

This gives viewers synchronized music and visuals while Unreal can be closed.
It is cheap to host, stable during a live show, and easy to loop.

## Online room roadmap

Treat the room as a later delivery layer, not part of DSP recovery.

### Stage A: shared broadcast (recommended first)

Everyone watches the same pre-rendered piece through the platform where you
already stream. Chat is the room. This validates whether viewers enjoy the idea
without infrastructure work.

### Stage B: one interactive browser room

Package one Unreal instance and use **Pixel Streaming**. Unreal renders and
plays audio once; browsers receive it over WebRTC. This matches a shared room
better than shipping the Unreal project to every viewer. Start on the local
network, then solve public hosting, HTTPS, STUN/TURN, access control, and GPU
hosting.

By default, Pixel Streaming viewers can share input. For a listening room,
disable general viewer control or expose only curated interactions such as
color voting, camera selection, and reactions.

### Stage C: true multiplayer space

Only build replicated avatars, movement, voice/chat, queues, moderation, and
per-user state after Stage B proves those features are wanted. This is a much
larger project than sharing one synchronized audiovisual experience.

## Definition of done for recovery

- [ ] A fresh C++ project builds the `SynRTCQT` module.
- [ ] One short, licensed WAV has a ConstantQNRT asset.
- [ ] A minimal mesh scene reacts deterministically to the track.
- [ ] A Level Sequence renders through Movie Render Queue.
- [ ] The rendered video and original audio remain synchronized.
- [ ] The result has completed a private OBS test.
- [ ] Only after those pass: test Pixel Streaming on the local network.

## Useful official documentation

- [Install Unreal Engine](https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine)
- [Set up Visual Studio for Unreal C++](https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-in-unreal-engine)
- [Audio Synesthesia and ConstantQNRT](https://dev.epicgames.com/documentation/unreal-engine/audio-synesthesia-in-unreal-engine)
- [Movie Render Pipeline](https://dev.epicgames.com/documentation/unreal-engine/movie-render-pipeline-in-unreal-engine)
- [Pixel Streaming](https://dev.epicgames.com/documentation/unreal-engine/pixel-streaming-in-unreal-engine)
- [Pixel Streaming quick start](https://dev.epicgames.com/documentation/unreal-engine/getting-started-with-pixel-streaming-in-unreal-engine)
