# First !pulse command into Unreal

Workstation: `C:\Users\camch\source\RTCQT\RTCQT.uproject`, UE 5.8.
Prepared on 2026-10-14 against the current Music Lab and Streamer.bot receiver.
The first-command guide recorded Streamer.bot 1.0.7; menu labels below follow
that setup and the current official documentation.

The finished path is Twitch chat -> Streamer.bot command -> C# action -> local
WebSocket -> RTCQT receiver -> Blueprint pulse controller -> Music Lab visualizer.

Typing `!pulse` will briefly enlarge the visualizer by up to 15%, then smoothly
return it to its normal size over 1.2 seconds while the music keeps playing.
This first version is a size accent. The glow accent from the broader
[interaction plan](viewer_chat_interactions_plan.md) can follow after this passes.

**This is an implementation walkthrough.** The current receiver accepts only
`test`, and the pulse controller described here does not exist yet. Writing
this guide did not change the runtime source or create Blueprint assets.
Complete the steps in order; changing only the Streamer.bot command will not
produce a visual effect.

## 1. Prove the existing connection and save a starting point

1. Open `C:\Users\camch\source\RTCQT\RTCQT.uproject`.
2. Open `/Game/Audio/PrivateTest/MusicLab/L_MusicLab`.
3. Follow [First Streamer.bot command into Unreal](streamerbot_first_command.md)
   if `!rtcqttest` is not already working.
4. Press **Play**, then type `!rtcqttest` in Twitch website chat.
5. Require the cyan message and this Unreal Output Log entry:

   ```text
   RTCQT chat received: test | hello from Streamer.bot
   ```

6. Listen to the music, switch tracks, pause/resume, and restart. Confirm one
   audible track and that the bars still follow the selected music.
7. Stop PIE, save the level, and close Unreal before editing/building C++.
8. Keep a copy of `Source/RTCQT/Private/RTCQTStreamerBotComponent.cpp` and
   `Source/RTCQT/Private/Tests/RTCQTStreamerBotTests.cpp` outside `Source`.
   For example, use `Saved/Backups/PulseSetup/`. These files already contain
   local first-command work; preserve it.

**Gate:** the test command reaches Unreal and the Music Lab playback controls
work. Fix that baseline before adding pulse behavior.

## 2. Teach the receiver to accept the pulse payload

Open:

`C:\Users\camch\source\RTCQT\Source\RTCQT\Private\RTCQTStreamerBotComponent.cpp`

Find `URTCQTStreamerBotComponent::AcceptCommand` near the end of the file.
Replace **that function only** with:

```cpp
bool URTCQTStreamerBotComponent::AcceptCommand(const TSharedPtr<FJsonObject>& Object)
{
    double Version = 0;
    FString Topic, Command, Value, Id, Timestamp;
    FDateTime IssuedAt;
    const FDateTime Now = FDateTime::UtcNow();
    if (!Object->TryGetNumberField(TEXT("schema_version"), Version) || Version != 1
        || !Object->TryGetStringField(TEXT("topic"), Topic) || Topic != TEXT("rtcqt.visuals")
        || !Object->TryGetStringField(TEXT("command"), Command)
        || (Command != TEXT("test") && Command != TEXT("pulse"))
        || !Object->TryGetStringField(TEXT("value"), Value) || Value.IsEmpty() || Value.Len() > 160
        || !Object->TryGetStringField(TEXT("request_id"), Id) || Id.IsEmpty() || Id.Len() > 128
        || !Object->TryGetStringField(TEXT("issued_at"), Timestamp) || !FDateTime::ParseIso8601(*Timestamp, IssuedAt)
        || (Now - IssuedAt).GetTotalSeconds() > 10 || (IssuedAt - Now).GetTotalSeconds() > 2) return false;
    if (Command == TEXT("pulse") && Value != TEXT("gentle")) return false;
    for (const TCHAR Character : Value) if (Character < 32) return false;
    for (auto It = SeenRequests.CreateIterator(); It; ++It)
        if ((Now - It.Value()).GetTotalSeconds() > 10) It.RemoveCurrent();
    if (SeenRequests.Contains(Id) || SeenRequests.Num() >= 256) return false;
    SeenRequests.Add(Id, Now);
    LastCommandValue = Value;
    ++ReceivedCommandCount;
    UE_LOG(LogRTCQT, Log, TEXT("RTCQT chat received: %s | %s"), *Command, *Value);
    if (Command == TEXT("test") && bShowTestMessages && GEngine)
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4.f, FColor::Cyan,
            FString::Printf(TEXT("RTCQT chat received: %s"), *Value));
    OnCommandReceived.Broadcast(Command, Value);
    return true;
}
```

The allowed pulse is exactly `command: pulse`, `value: gentle`. The existing
schema, topic, freshness, and duplicate checks still run. The cyan diagnostic
continues to belong to `test`; the controller will report accepted pulses.

Add protocol coverage in:

`C:\Users\camch\source\RTCQT\Source\RTCQT\Private\Tests\RTCQTStreamerBotTests.cpp`

Find the assertion named `Invalid commands preserve displayed value`. Immediately
**after** it, and before `const uint64 OldGeneration`, insert:

```cpp
    auto Pulse = Payload();
    Pulse->SetStringField(TEXT("command"), TEXT("pulse"));
    Pulse->SetStringField(TEXT("value"), TEXT("gentle"));
    Receiver->HandleMessage(Encode(Pulse));
    Receiver->HandleMessage(Encode(Pulse));
    TestEqual(TEXT("Pulse accepted once despite duplicate"), Receiver->ReceivedCommandCount, 5);
    TestEqual(TEXT("Pulse preset recorded"), Receiver->LastCommandValue, FString(TEXT("gentle")));
    auto InvalidPulse = Payload();
    InvalidPulse->SetStringField(TEXT("command"), TEXT("pulse"));
    InvalidPulse->SetStringField(TEXT("value"), TEXT("huge"));
    Receiver->HandleMessage(Encode(InvalidPulse));
    TestEqual(TEXT("Unknown pulse preset ignored"), Receiver->ReceivedCommandCount, 5);
```

At the end of that test, change only the expected count in the assertion named
`Disconnected receiver ignores messages` from `4` to `5`. The earlier expected
counts remain as they are.

Save both files. No header change is needed for this version; Blueprint already
has access to **On Command Received**.

## 3. Build and check the native change

Keep Unreal closed. In **Ubuntu/WSL Bash**, run:

```bash
cd /mnt/c/Users/camch/source/RTCQT

'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.exe' \
  RTCQTEditor Win64 Development \
  '-Project=C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -WaitMutex -NoHotReloadFromIDE \
  '-Log=C:\Users\camch\source\RTCQT\Saved\Logs\Pulse-build.log'
```

Require a successful build before continuing. Then run the existing RTCQT suite:

```bash
'/mnt/c/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' \
  'C:\Users\camch\source\RTCQT\RTCQT.uproject' \
  -unattended -nop4 -nosplash -NullRHI -NoSound \
  '-ExecCmds=Automation RunTests RTCQT.' \
  '-TestExit=Automation Test Queue Empty' \
  '-ReportExportPath=C:\Users\camch\source\RTCQT\Saved\Automation\Pulse' \
  '-abslog=C:\Users\camch\source\RTCQT\Saved\Logs\Pulse-tests.log'
```

Open `Saved/Automation/Pulse/index.json`. Require `RTCQT.StreamerBot.Protocol`
and the other RTCQT tests to succeed, with zero failed or unrun tests. Check
the report contents rather than relying only on the process exit code.
This checks the receiver; the Blueprint effect still needs the checks below.

**Gate:** native build and suite pass. If the compiler reports duplicate
definitions, check that you replaced `AcceptCommand` instead of adding a second
copy. If the disconnect assertion fails with actual `5`, check its expected
count from Step 2. Reopen the editor only after fixing failures.

## 4. Create the Blueprint pulse controller

1. Reopen the project and `L_MusicLab`.
2. In the Content Browser, create the folder `/Game/RTCQT/Interactions`.
3. Right-click in that folder -> **Blueprint Class** -> **Actor**.
4. Name it `BP_RTCQTPulseController` and open it.
5. Under **Class Defaults**, set **Actor Tick > Start with Tick Enabled** on
   and **Tick Interval** to `0`.
6. Set **Input > Auto Receive Input** to **Player 0** for local preview keys.
7. In **My Blueprint > Variables**, add the following. Compile when needed to
   expose a variable's default value.

| Variable | Type | Default / setup |
| --- | --- | --- |
| `Manager` | RTCQTMusicLabManager **Object Reference** | None; enable **Instance Editable** |
| `Visualizer` | RTCQTVisualizer **Object Reference** | None; filled from Manager on BeginPlay |
| `BaseScale` | Vector | `(1, 1, 1)`; replaced with the authored scale on BeginPlay |
| `Initialized` | Boolean | false |
| `PulseActive` | Boolean | false |
| `PulseFromChat` | Boolean | false |
| `PulseStartedAt` | Float | `0` |
| `LastAcceptedAt` | Float | `-1000` |
| `ActiveAudio` | Audio Component **Object Reference** | None |
| `LastConnectionStatus` | String | empty |
| `ViewerControlsEnabled` | Boolean | true; enable **Instance Editable** |
| `PulseStrength` | Float | `0.15` |

For object types, choose **Object Reference**, rather than Class Reference.
The negative `LastAcceptedAt` lets the first pulse work immediately.

This actor scales the visualizer as a whole. The native visualizer continues
to calculate every band's music response inside that transform. The row's
spacing also expands slightly; keep the whole row in the camera at peak size.

### Reading the Blueprint instructions

White wires control execution. Colored wires carry values. Drag a variable
into the graph and choose **Get** to read it or **Set** to assign it. Drag off
an object reference to find functions/properties belonging to that object.

A **Branch** is an if/else. In a function, finish an early-exit path with a
**Return Node**. Use the execution version of **Is Valid** before reading an
object's fields. Combining a validity check and a field read with Boolean AND
does not guarantee a safe evaluation order.

Compile and save after each function below. Empty placeholder functions can
be created first if you need their call nodes before wiring their bodies.

## 5. Add cancel and viewer-lock functions

In **My Blueprint > Functions**, click **+** and create `CancelPulse`, with
no inputs or outputs. Wire these operations in order:

1. Set `PulseActive` to false.
2. Set `PulseFromChat` to false.
3. Set `ActiveAudio` to None; leave the object input unconnected on its Set node.
4. Use **Is Valid** on `Visualizer`.
5. On the valid path, call **Set Actor Scale 3D**, Target = `Visualizer`,
   New Scale 3D = `BaseScale`. Then return. The invalid path simply returns.

Leave `LastAcceptedAt` alone. Canceling a pulse should not remove its cooldown.

Create `SetViewerControlsEnabled`, with one Boolean input named `Enabled`:

1. Set the `ViewerControlsEnabled` variable to `Enabled`.
2. Branch on `Enabled`. False -> call `CancelPulse`; True -> continue.
3. Print a short status with **Print String**: `RTCQT viewer pulses locked`
   when false, or `RTCQT viewer pulses enabled` when true. You can use a
   **Select String** node, with Pick A = `Enabled`, A = the enabled message,
   and B = the locked message, feeding Print String's In String pin.

The lock controls incoming chat pulses. Local preview remains available.

## 6. Initialize the controller and bind the receiver

In the Event Graph, create **Event BeginPlay** and wire:

```text
Event BeginPlay
  -> Is Valid Manager
  -> Set Visualizer (value = Manager.Visualizer)
  -> Is Valid Visualizer
  -> Is Valid (object = Manager.StreamerBot)
  -> Set BaseScale (value = Get Actor Scale 3D, Target = Visualizer)
  -> Set LastConnectionStatus (value = Manager.StreamerBot.ConnectionStatus)
  -> Bind Event to On Command Received (Target = Manager.StreamerBot)
  -> Set Initialized = true
  -> Print String "RTCQT pulse controller ready"
```

On each invalid path, print `RTCQT pulse setup error: missing reference` in
red and stop that execution path. Leave `Initialized` false.

To create the delegate handler, drag from the Bind node's red **Event** pin
and choose **Add Custom Event**. Name the generated event `HandleChatCommand`.
It must have the delegate's two String inputs, **Command** and **Value**.

From `HandleChatCommand`, compare the strings with **Equal (String)**:

```text
(Command == "pulse") AND (Value == "gentle")
  -> Branch
       True -> TryPulse (FromChat = true)
       False -> end
```

Create `TryPulse` as described next, or create its empty signature now and
return to finish it. Bind once on BeginPlay. Do not bind from Tick or from
each incoming command.

**Gate:** the Blueprint compiles. A missing Manager assignment is expected
until you place the controller in Step 9.

## 7. Add the shared TryPulse function

Create `TryPulse` with Boolean input `FromChat` and Boolean output `Accepted`.
Set `Accepted` false on every rejection Return Node.

Build a sequence of validity checks and Branch nodes using this table in
order. A failed check prints the listed message and returns false. Enable
**Print to Log** on these Print String nodes so troubleshooting has a record.

| Continue only when... | Rejection message |
| --- | --- |
| `Initialized` is true | `RTCQT pulse ignored: controller not initialized` |
| `Manager` is valid, then `Visualizer` is valid, then `Manager.StreamerBot` is valid | `RTCQT pulse ignored: missing reference` |
| `FromChat` is false OR `ViewerControlsEnabled` is true | `RTCQT pulse ignored: viewer controls locked` |
| `FromChat` is false OR `Manager.StreamerBot.ConnectionStatus` equals `Ready` | `RTCQT pulse ignored: receiver not ready` |
| `Manager.PlaybackState` equals the enum value **Playing** | `RTCQT pulse ignored: music is not playing` |
| `Manager.AudioComponent` is valid | `RTCQT pulse ignored: missing audio component` |
| `PulseActive` is false | `RTCQT pulse ignored: already active` |
| `Now - LastAcceptedAt` is greater than or equal to `5.0` | `RTCQT pulse ignored: cooldown` |

For the playback-state comparison, drag from **Get Playback State**, create
an enum equality node, and select **Playing**. This is not a String comparison.

For `Now`, create a function **Local Variable** of type Float. Set it from
**Get Real Time Seconds** once before the cooldown check. Find local variables
in My Blueprint while this function is open.

On the accepted path, wire these Set nodes in order:

```text
Set PulseStartedAt = Now
  -> Set LastAcceptedAt = Now
  -> Set ActiveAudio = Manager.AudioComponent
  -> Set PulseFromChat = FromChat
  -> Set PulseActive = true
  -> Print String "RTCQT pulse accepted"
  -> Return Accepted = true
```

Set the accepted Print String to green, Duration `1.2`, with **Print to Log**
enabled. This message means an effect was accepted, while the receiver's
`RTCQT chat received` line means only that a valid payload arrived.

Both chat and the local preview use this function. The Unreal five-second
limit also applies to the broadcaster, who bypasses Streamer.bot cooldowns.
An active pulse is ignored rather than restarted, stacked, or queued.

## 8. Add the pulse animation and cancellation checks

Create a function `UpdatePulse`, with no inputs or outputs. Add Float local
variables `Elapsed`, `T`, and `Accent`, and a String local variable `NewStatus`.
In the Event Graph, connect **Event Tick** directly to `UpdatePulse`.

### Check the connection and playback first

Wire the following sequence inside `UpdatePulse`:

1. If `Initialized` is false, return.
2. Validate `Manager`, then `Visualizer`, then `Manager.StreamerBot`.
   If any is invalid, call `CancelPulse`, set `Initialized` false, and return.
3. Set `NewStatus = Manager.StreamerBot.ConnectionStatus`.
4. Branch on `LastConnectionStatus == "Ready" AND NewStatus != "Ready"`.
   True -> `CancelPulse`. Both paths then continue to the next step.
5. Set `LastConnectionStatus = NewStatus`.
6. If `PulseActive` is false, return.
7. If any of the following is true, call `CancelPulse` and return:
   - `Manager.PlaybackState` is not **Playing**.
   - `Manager.AudioComponent` is not equal to `ActiveAudio` using **Not Equal (Object)**.
   - `PulseFromChat` is true AND `ViewerControlsEnabled` is false.
   - `PulseFromChat` is true AND `NewStatus` is not `Ready`.

The Music Lab replaces its audio component on every successful track switch
and restart. Comparing that reference catches both operations, including a
restart of the same song. Pause and natural track completion are caught by
Playback State. Cancellation takes effect on the next controller tick.

### Calculate one bounded accent

After those checks:

1. Set `Elapsed = Get Real Time Seconds - PulseStartedAt`.
2. If `Elapsed >= 1.2`, call `CancelPulse` and return. This restores exactly
   `BaseScale`, including after a long frame.
3. Otherwise, calculate `Accent` with these two paths:

| Branch | Set T | Set Accent |
| --- | --- | --- |
| `Elapsed <= 0.1` | Clamp `Elapsed / 0.1` to `[0, 1]` | `T * T * (3 - 2 * T)` |
| `Elapsed > 0.1` | Clamp `(Elapsed - 0.1) / 1.1` to `[0, 1]` | `1 - T * T * (3 - 2 * T)` |

4. Connect both paths to **Set Actor Scale 3D**:
   - Target: `Visualizer`.
   - New Scale 3D: `BaseScale * (1 + Clamp(PulseStrength, 0, 0.15) * Accent)`.
   - Use **Vector multiplied by Float** for the final multiplication.

Use ordinary Multiply, Subtract, Add, Divide, and **Clamp (Float)** nodes for
these formulas. For example, `T * T * (3 - 2 * T)` is two multiplications for
`T * T` and `2 * T`, one subtraction from `3`, then one final multiplication.
Wire the first path's result into **Set Accent**, and do the same on the
second path before continuing to the scale-setting node.

Expected envelope, independent of the song's position:

| Seconds since accepted | Accent | Scale multiplier with strength 0.15 |
| --- | --- | --- |
| `0.00` | `0.0` | `1.000` |
| `0.05` | `0.5` | `1.075` |
| `0.10` | `1.0` | `1.150` |
| `0.65` | `0.5` | `1.075` |
| `1.20` | `0.0` | `1.000` |

Always multiply the saved `BaseScale`. Reading the current Actor Scale and
multiplying it again each tick would accumulate growth. The native band's
analyzed scale keeps updating underneath this actor transform, so returning
to `BaseScale` reveals the music's current frame.

Use **Get Real Time Seconds** for the accent and cooldown. Continue letting
the existing audio-mixer callbacks drive analysis; the pulse controller does
not call Evaluate At Time or advance the song clock.

Finally, add **Event EndPlay** -> `CancelPulse` in the Event Graph. Compile
and save the Blueprint.

## 9. Place the controller and prove a local pulse

1. Drag one `BP_RTCQTPulseController` into `L_MusicLab`.
2. Name its Outliner entry **RTCQT Pulse Controller**.
3. In its Details panel, set **Manager** to **Music Lab Manager**, using the
   dropdown or eyedropper. Leave Viewer Controls Enabled on.
4. Save the level. Keep exactly one pulse controller in the scene.
5. Reopen the controller Blueprint and add keyboard events in its Event Graph:

   ```text
   P / Pressed -> TryPulse (FromChat = false)
   L / Pressed -> SetViewerControlsEnabled (Enabled = NOT ViewerControlsEnabled)
   O / Pressed -> CancelPulse
   ```

6. Compile and save. Press **Play**, using **Selected Viewport**, and click in
   the game viewport so keyboard input reaches the controller. Use Pressed,
   rather than Released, for these three keys.
7. Require `RTCQT pulse controller ready` and audible music.
8. Press **P** once. Require `RTCQT pulse accepted`, one gentle expansion,
   and a smooth return after about 1.2 seconds.
9. Press P again during the pulse, then again before five seconds have elapsed
   since acceptance. Both should be ignored. After five seconds, P should work.
10. Trigger a pulse and click Pause, Restart, or another track while it is
    still active. Each should restore normal actor scale promptly.
11. Pause playback and press P. It should be ignored. Resume, wait for the
    cooldown, and try again.

**Gate:** local pulse, return to baseline, cooldown, and playback cancellation
work before introducing Twitch. If P does nothing, check viewport focus, the
controller's **Auto Receive Input = Player 0**, and the rejection messages.
If the accepted message appears but scale stays enlarged, check Event Tick,
the 1.2-second exit, and CancelPulse's scale assignment.

To inspect values during PIE, choose the **playing** controller in the
Blueprint **Debug Filter**, and inspect the playing visualizer in the Outliner.
At the end of a pulse, its Actor Scale must equal the captured BaseScale;
the individual bars will still move with the music.

## 10. Check the Streamer.bot server and Unreal endpoint

In Streamer.bot, open **Servers/Clients > WebSocket Server**:

| Setting | Value |
| --- | --- |
| Address | `127.0.0.1` |
| Port | Your existing port, usually `8080` |
| Endpoint | `/` |
| Auto Start | On |

Start the server if needed. Reuse the authentication setting that passed
`!rtcqttest`. When enabled, Unreal reads the password from:

`C:\Users\camch\source\RTCQT\Saved\Config\StreamerBot.ini`

```ini
[RTCQT.StreamerBot]
Password=the exact local WebSocket password
```

In Unreal, stop PIE. Select **Music Lab Manager > StreamerBot**:

1. Enable **Auto Connect**.
2. Set **Server Url** to `ws://127.0.0.1:8080/`, substituting your actual port.
3. Save the level.
4. Play and require Connection Status **Ready**.

The existing ready log still suggests `!rtcqttest`; that remains a useful
diagnostic after the receiver edit. After a server interruption, restart PIE
to reconnect. The current receiver does not automatically retry.

## 11. Create the !pulse chat command

In Streamer.bot, open **Commands**, right-click the list, and choose **Add**:

| Setting | Value |
| --- | --- |
| Name | `RTCQT Pulse` |
| Enabled | On |
| Mode | `Normal` in the first guide's UI; `Basic` in the documentation |
| Location | `Exact` |
| Commands | `!pulse` |
| Source | `Twitch Message` |
| Case Sensitive | Off |
| Global Cooldown | `5` seconds |
| User Cooldown | `15` seconds |

Keep Ignore Bot Account enabled. Test from Twitch website chat. If Ignore
Internal Messages is enabled, Streamer.bot's internal chat may not trigger it.
Default permissions can allow everyone; use an ordinary viewer account to
verify any restrictions or cooldowns because the broadcaster bypasses both.

This command accepts the bare `!pulse`, including casing such as `!PULSE`.
With Exact matching, `!pulse huge`, `!pulseplease`, and a sentence containing
`!pulse` do not match. It may not appear in the internal chat's `!`
autocomplete, so type the entire command.

Save the command. It needs the linked action in the next step before it does
anything.

## 12. Create the action and paste the C# sender

1. Open **Actions & Queues > Actions**.
2. Add an enabled action named `RTCQT - Send Pulse to Unreal`.
3. In its **Triggers** panel, right-click ->
   **Core > Commands > Command Triggered**.
4. Select `RTCQT Pulse` and save the trigger.
5. In **Sub-Actions**, right-click -> **Core > C# > Execute C# Code**.
6. Replace the sample with the contents of
   [`planning/streamerbot/RTCQTPulseCommand.cs`](../streamerbot/RTCQTPulseCommand.cs):

```csharp
using System;

public class CPHInline
{
    public bool Execute()
    {
        string json = "{\"schema_version\":1,\"topic\":\"rtcqt.visuals\","
            + "\"command\":\"pulse\",\"value\":\"gentle\","
            + "\"request_id\":\"" + Guid.NewGuid().ToString("N") + "\","
            + "\"issued_at\":\"" + DateTime.UtcNow.ToString("o") + "\"}";

        CPH.LogInfo("RTCQT: sending pulse to Unreal");
        CPH.WebsocketBroadcastJson(json);
        return true;
    }
}
```

7. Click **Compile**, require success, then save the sub-action and action.
   You can enable Precompile on Application Start for subsequent sessions.
8. Leave the original `RTCQT Test` command and test action available.

The sender creates a fresh request ID and UTC timestamp each time. The fixed
`gentle` value matches the receiver's allowed preset. It carries no pulse
strength argument; strength is bounded inside Unreal.

**Gate:** type `!pulse` in Twitch website chat. Streamer.bot Action History
should show this action, and its log should include:

```text
RTCQT: sending pulse to Unreal
```

This proves the command/action connection. Continue to prove reception and
the actual effect.

## 13. Prove the full chat-to-effect path

1. Keep Streamer.bot's server running.
2. In Unreal, play `L_MusicLab` and require audible music, receiver **Ready**,
   and `RTCQT pulse controller ready`.
3. If you used local preview, wait at least five seconds after its last
   acceptance. If you locked viewer pulses with L, press L to enable them.
4. Type `!pulse` in Twitch website chat.

Expected Unreal Output Log entry from the receiver:

```text
RTCQT chat received: pulse | gentle
```

Expected controller message:

```text
RTCQT pulse accepted
```

Expect one 1.2-second expansion and return while the selected music continues.
During PIE, the receiver's **Received Command Count** should increase once and
**Last Command Value** should become `gentle`.

**Pass:** one allowed chat command produces one accepted pulse. A received
payload without the accepted-controller message is not an effect pass.

## 14. Check cooldowns, recovery, and saved settings

Use this checklist after the first successful pulse:

| Check | Expected result |
| --- | --- |
| Broadcaster sends several pulses less than five seconds apart | Unreal accepts at most one in five seconds; no stacking or delayed pulses |
| Viewer A sends twice within 15 seconds | Second command is blocked by that viewer's Streamer.bot cooldown |
| Viewer B sends within five seconds of A's accepted command | Streamer.bot's global cooldown blocks it |
| Viewer B sends after five seconds, with B's own cooldown clear | One pulse is allowed |
| `!PULSE` | Same result as `!pulse`, when cooldowns allow |
| `!pulse huge` or `!pulseplease` | No command match or effect |
| Pause music, then send `!pulse` | Transport may receive it; effect is ignored and never queued for resume |
| Switch tracks or restart during a pulse | Scale returns to BaseScale; new playback remains correct |
| Press L during a pulse, then send a chat pulse | Active pulse cancels; chat effects stay locked; P remains a local preview |
| Press O during a pulse | Pulse cancels immediately; existing cooldown stays in effect |
| Stop Streamer.bot's server during a pulse | Pulse cancels when the receiver detects disconnect; music continues |
| Restart server, stop PIE, and Play again | Receiver reaches Ready; fresh commands work; no old pulse replays |
| Close/reopen both apps, reload Music Lab, and Play | One controller, correct Manager reference, saved endpoint, and normal starting scale |

A command accepted by Streamer.bot while Unreal ignores the effect can still
consume Streamer.bot's cooldown. Wait for that account's cooldown before the
next test. The Unreal cooldown is five seconds between accepted effects; the
per-viewer 15-second restriction is enforced in Streamer.bot.

After reopening, listen again to track switching, pause/resume, and restart.
Confirm the pulse feels gentle and the row fits the camera at full expansion.
If needed, lower Pulse Strength to `0.08`; the guide caps the maximum at `0.15`.

**Final gate:** confirm by eye and ear that chat triggers one accent, it returns
cleanly to the current music response, and playback/restart still work after
reopening. Record that confirmation separately from the automated test report.

## 15. Save a recoverable setup

1. Stop PIE and save the controller Blueprint and Music Lab level.
2. In Streamer.bot, use **Add to Export** for the pulse command and action.
   Save an export containing both so the command/action link can be recovered.
3. Keep the C# file, source changes, controller Blueprint, and guide together
   in version control when you are ready to publish the implementation.
4. Back up the Music Lab level separately. Its location under
   `Content/Audio/PrivateTest/` is Git-ignored, even though the controller
   Blueprint under `Content/RTCQT/Interactions/` can be tracked normally.
5. Retain a short recording of a pulse and the relevant build/test and runtime
   logs. The broader plan uses `planning/evidence/viewer-interactions/` for
   shareable evidence; `Saved/Automation/Pulse/` holds the local test report.

For a quick rollback, disable `RTCQT Pulse` in Streamer.bot, stop PIE, and
remove the placed pulse controller from Music Lab. Its source receiver change
can remain without producing an effect, or you can restore your two source
backups with the editor closed and rebuild.

## If something does not match

| Symptom | First checks |
| --- | --- |
| No Streamer.bot action runs | Command enabled, Exact text, Twitch Message source, correct Command Triggered link, website chat, account cooldown |
| Streamer.bot sends; Unreal receives nothing | PIE running, receiver Ready, endpoint/password, successful native rebuild and editor reopen; retry `!rtcqttest` |
| Test arrives; pulse does not | Step 2 allowlist compiled, `command = pulse`, `value = gentle`, fresh timestamp and request ID |
| Pulse received but no accepted message | One controller placed, correct Manager, BeginPlay binding, Viewer Controls Enabled, playback Playing, no active pulse/cooldown |
| Accepted message but no visible movement | Valid Visualizer, Event Tick -> UpdatePulse, nonzero Pulse Strength, Set Actor Scale 3D target and formula |
| Effect grows every frame or never settles | Use saved BaseScale each tick; check elapsed-time calculation and CancelPulse's exact scale restore |
| One command gives multiple effects or prints | Duplicate controllers or repeated event binding; keep one controller and bind only on BeginPlay |
| Music restarts or analysis freezes when pulsing | Remove any pulse-graph calls to Restart, Play, Set Playback Analyzer, or Evaluate At Time; scale only the visualizer actor |
| Reconnection does not recover automatically | Restart PIE after the Streamer.bot server is running |

## Verification status

This guide's source locations, existing Blueprint-accessible properties, and
UE 5.8 time/scale functions were checked while writing it. The Streamer.bot
settings and broadcast method were checked against the official references
below. The companion C# sender compiled in a standalone harness with stubbed
CPH logging/broadcast methods; two executions produced valid pulse JSON with
distinct request IDs and fresh UTC timestamps. Its in-guide copy and local
document links were also checked. Compile it inside Streamer.bot in Step 12
to verify that application's actual C# environment.

Native pulse changes and Blueprint construction remain steps for you to
execute; no completed Unreal pulse, Twitch delivery, or human playback pass
is claimed by this document.

## References

- [Existing first-command guide](streamerbot_first_command.md)
- [Viewer interaction plan](viewer_chat_interactions_plan.md)
- [Streamer.bot Commands](https://docs.streamer.bot/guide/core/commands)
- [Command Triggered](https://docs.streamer.bot/api/triggers/core/commands/command-triggered)
- [WebsocketBroadcastJson](https://docs.streamer.bot/api/csharp/methods/core/websocket/websocket-broadcast-json)
- [WebSocket configuration](https://docs.streamer.bot/api/websocket/guide/configuration)
- [Unreal world time functions](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UWorld)
