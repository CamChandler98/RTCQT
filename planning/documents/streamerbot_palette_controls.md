# Palette step 3: local controls and preset setup

Implemented and validated 2026-10-15, UE 5.8, in
`C:\Users\camch\source\RTCQT`. The native build, all 35 RTCQT tests,
offscreen widget rendering, and real preset asset checks passed. See
[the evidence](../evidence/viewer-interactions/palette-step3.md).

Cam owns the existing level and pulse Blueprint saves. The native panel and
dedicated preset asset are ready; the saved level assignment and human
music/visual review below are still pending. Palette WebSocket allowlisting
is step 4 of [the implementation plan](streamerbot_palette_plan.md).

## 1. Open and configure the existing level

1. Open `C:\Users\camch\source\RTCQT\RTCQT.uproject`.
2. In the Content Browser, open
   `/Game/Audio/PrivateTest/MusicLab/L_MusicLab`.
3. In the World Outliner, select the placed **Music Lab Manager** actor
   (`BP_MusicLabManager`). In Details, select its **Palette** component.
   Clear any Details search first if the component list is hidden.
4. Under **RTCQT > Palette**, set:

   | Setting | Value |
   | --- | --- |
   | Preset Asset | `/Game/Audio/PrivateTest/MusicLab/DA_MusicLabPalettes` |
   | Palette Enabled | Checked |
   | Viewer Palettes Enabled | Checked to allow future viewer requests, unchecked to lock them |

5. On the manager actor, confirm its existing **Visualizer**, **Catalog**,
   **Demo Camera**, and **Controls Class** references are still populated.
   Controls Class should remain `WBP_MusicLabControls` from this Music Lab
   folder. Recheck the pulse controller's existing manager/visualizer
   references using the pulse walkthrough.
6. Save the level with **Ctrl+S**. These placed-instance settings are the
   persistent opt-in. Changes to the checkboxes during Play are session only.

Expected: the Palette component and asset are available, and the existing
pulse graphs/settings are preserved. If native members are missing, stop
Play, open `BP_MusicLabManager` and `WBP_MusicLabControls`, choose **Compile**,
then return to the level and recheck references before saving. Their native
parents should be `RTCQTMusicLabManager` and `RTCQTMusicLabControls`.

If the preset asset is missing, close Unreal and run section 4. An empty
Preset Asset also supports the four built-in defaults, but assigning the
DataAsset allows saved artist edits.

## 2. Check the controls with real music

1. Press **Play**. The upper-left panel keeps the existing track dropdown,
   Play/Pause, and Restart controls and adds **Palettes** underneath.
2. Confirm **Enable palettes** is checked. If you left the saved feature
   disabled, checking this now previews it for this Play session only.
3. Choose **Sunset**, **Ice**, and **Ember**. Confirm four distinct color
   pairs including **Neon**, smooth half-second blends, and uninterrupted
   music. A new selection during a blend should continue from the visible
   colors. Brief green feedback confirms local selections.
4. Press **Next palette** repeatedly: Neon -> Sunset -> Ice -> Ember -> Neon.
5. Press **Reset palette**, or select **Authored colors**. Confirm a smooth
   return to the material's original colors. Next from authored colors
   starts with Neon.
6. Uncheck **Viewer palettes**. Status should read **Viewer palettes:
   locked**. Local selection, Next, and Reset should still work. This
   checkbox controls palettes only; the pulse guide's **L** key controls
   viewer pulses separately.
7. While a palette is selected, switch tracks, Restart, and Pause/Play.
   Confirm exactly one audible track, correct track/analyzer response,
   and the palette staying selected. Palettes can also blend while paused.
8. Uncheck **Enable palettes**. Colors should restore immediately and
   palette selection controls should disable. Re-enable to preview again.

Expected: palette controls never restart audio or change actor scale. The
status shows the selected target while blending and the separate viewer
gate/cooldown. An enabled viewer gate alone does not establish a receiver
connection. Chat palette messages are not accepted until step 4 is done.

## 3. Finish the combined-effects and persistence gates

After the pulse walkthrough's local effect works:

1. Start a pulse using the walkthrough's existing local control. During its
   scale animation, select a different palette. Move keyboard focus back to
   the game viewport if a UI control holds focus.
2. Confirm colors blend while the pulse changes scale, then scale returns
   to BaseScale and the selected palette remains. Check pulse cancellation
   and its existing viewer cooldown independently.
3. Stop Play and start again. Colors should begin at authored defaults;
   runtime palette selections are transient.
4. Save the intended component settings outside Play, close Unreal, reopen
   the same level, and repeat one palette and one pulse. Confirm the preset
   reference/feature opt-in survived and music still sounds correct.

Record the human results and a short combined-effects recording in
`planning/evidence/viewer-interactions/`. Automated transient fixtures and
widget screenshots do not establish these listening/scene checks.

## 4. Rerun the dedicated preset setup if needed

Save and close Unreal first. In WSL/Bash:

```bash
cd /mnt/c/Users/camch/source/RTCQT
UE='/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
"$UE" 'C:/Users/camch/source/RTCQT/RTCQT.uproject' /Engine/Maps/Entry \
  -run=pythonscript \
  '-script=C:/Users/camch/source/RTCQT/planning/scripts/setup_palette_presets.py' \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  '-abslog=C:/Users/camch/source/RTCQT/Saved/Logs/Palette-step3-setup.log'
```

Expected: `RTCQT_PALETTE_SETUP_PASSED` in the log and
`Saved/Automation/PaletteStep3/setup.json` with `passed: true`. The first run
creates `DA_MusicLabPalettes`; later runs validate and reuse it without
saving or replacing artist colors/labels. The script never loads or saves
the Music Lab level. Do not run the broader `setup_music_lab.py` for this
handoff.

Open the DataAsset in Unreal to edit **Display Name**, **Low Color**, and
**High Color**, then save it. Preserve the stable IDs `neon`, `sunset`,
`ice`, and `ember` for the planned chat contract. Stop and restart Play to
reload edits into the panel.

To intentionally replace all edited presets with the four native defaults,
repeat the command with the additional `-RTCQTPaletteResetDefaults` flag.
This explicit reset overwrites saved palette edits. If setup reports an
unowned asset or wrong class, inspect that exact asset instead of deleting
or overwriting it; setup deliberately refuses the collision.

For an isolated rerun of the edit-preservation/reset checks, substitute
`verify_palette_presets.py` in the same command. Expected marker:
`RTCQT_PALETTE_PRESETS_VERIFIED`; report:
`Saved/Automation/PaletteStep3/preset-verification.json`. It creates and
removes only its unique owned verification asset and preserves the real
preset asset, level, and pulse Blueprint.

The private Music Lab directory is ignored by Git. The setup script and
this handoff recreate the preset asset and explain its instance assignment
on another workstation; source publishing does not include the private
level, music library, or palette DataAsset.
