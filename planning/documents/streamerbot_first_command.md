# First Streamer.bot command into Unreal

Workstation: `C:\Users\camch\source\RTCQT\RTCQT.uproject`, UE 5.8.
Streamer.bot 1.0.7 was running when this guide was prepared on 2026-10-13.

The test path is Twitch chat -> Streamer.bot command -> C# action -> local
WebSocket -> RTCQT receiver -> Unreal Output Log and a cyan on-screen message.
The receiver accepts the `test` command. Palette, shape, and pulse effects
remain separate implementation work.

## 1. Start the local Streamer.bot server

In Streamer.bot, open **Servers/Clients > WebSocket Server**. Set:

| Setting | First-test value |
| --- | --- |
| Address | `127.0.0.1` |
| Port | `8080`, or another available port |
| Endpoint | `/` |
| Auto Start | On |

Click **Start Server** if it is stopped. The Unreal endpoint for those values
is `ws://127.0.0.1:8080/`. This is Streamer.bot's own server, configured in
Streamer.bot; no OBS server configuration is needed for this connection.

The receiver supports either authentication setting. If you enabled WebSocket
authentication, create this ignored local file:

`C:\Users\camch\source\RTCQT\Saved\Config\StreamerBot.ini`

```ini
[RTCQT.StreamerBot]
Password=the exact password from Streamer.bot's WebSocket settings
```

Use the local WebSocket password. Leave Twitch login tokens in Streamer.bot.
Ensure the file is named `StreamerBot.ini`, without a hidden `.txt` suffix.
When authentication is disabled, no password file is required.

**Gate:** the server starts successfully. If the port is already in use,
choose a different port and use that same port in Unreal.

## 2. Create the chat command

1. Open **Commands**.
2. Right-click the command list and choose **Add**.
3. Configure the following, then save:

| Setting | Value |
| --- | --- |
| Name | `RTCQT Test` |
| Enabled | On |
| Mode | `Normal` (called `Basic` in some documentation) |
| Location | `Exact` |
| Commands | `!rtcqttest` |
| Source | `Twitch Message` |
| Case Sensitive | Off |
| Global Cooldown | `2` seconds |
| User Cooldown | `5` seconds |

Ignore Bot Account can stay enabled. Test from your broadcaster or an ordinary
viewer account using Twitch's website chat. Streamer.bot's internal chat may
be ignored by the command's Ignore Internal Messages setting. The broadcaster
is exempt from Streamer.bot cooldowns; use a viewer account to test cooldowns.

At the end of this step, you have defined the command text. Action History
will show a run after you create and link its action in Step 3. Also,
Streamer.bot chat's `!` autocomplete lists commands with Location set to
Start; this Exact command can be tested by typing the full `!rtcqttest`.
The Include toggle controls the Get Commands sub-action, rather than linking
the command to an action.

## 3. Create the action and link the command

1. Open **Actions & Queues > Actions**.
2. Right-click the action list, choose **Add**, and name it
   `RTCQT - Send Test to Unreal`. Keep it enabled.
3. With that action selected, right-click its **Triggers** panel.
4. Choose **Core > Commands > Command Triggered**.
5. Select `RTCQT Test`, then save the trigger.
6. Right-click the **Sub-Actions** panel.
7. Choose **Core > C# > Execute C# Code** (the group may display as C# Code).
8. Replace the sample code with the contents of
   `planning/streamerbot/RTCQTTestCommand.cs`:

```csharp
using System;

public class CPHInline
{
    public bool Execute()
    {
        string json = "{\"schema_version\":1,\"topic\":\"rtcqt.visuals\","
            + "\"command\":\"test\",\"value\":\"hello from Streamer.bot\","
            + "\"request_id\":\"" + Guid.NewGuid().ToString("N") + "\","
            + "\"issued_at\":\"" + DateTime.UtcNow.ToString("o") + "\"}";

        CPH.LogInfo("RTCQT: sending test to Unreal");
        CPH.WebsocketBroadcastJson(json);
        return true;
    }
}
```

9. Click **Compile**, require a successful result, then save the sub-action.
   Precompile on Application Start can be enabled for subsequent sessions.

Creating a command defines what text to match. The Command Triggered trigger
connects that match to this action. The C# sub-action sends the test payload.

**Gate:** type `!rtcqttest` in Twitch website chat. The action should appear
in Streamer.bot's action history; its log should include
`RTCQT: sending test to Unreal`. This proves the command/action connection.
It does not yet prove Unreal received the event. Save the action and command
to an export after the complete test passes.

## 4. Enable the receiver in Music Lab

1. Reopen `C:\Users\camch\source\RTCQT\RTCQT.uproject` after the native build.
2. Open `/Game/Audio/PrivateTest/MusicLab/L_MusicLab`.
3. Select **Music Lab Manager** in the World Outliner.
4. In the Details component list, select **StreamerBot**.
5. Under **RTCQT > Chat**, enable **Auto Connect**.
6. Set **Server Url** to `ws://127.0.0.1:8080/`, matching your server's port.
   Keep **Show Test Messages** enabled.
7. Save the Music Lab level so Auto Connect and the endpoint survive reopening.
8. Open **Window > Developer Tools > Output Log** and filter for `Streamer.bot`.
   The menu location can vary; the editor's Output Log drawer is also usable.
9. Press **Play**.

Expected log messages:

```text
Streamer.bot connected; waiting for Hello.
Streamer.bot ready; type !rtcqttest in Twitch chat.
```

**Gate:** reach the second message. If the component is missing, confirm the
native build finished and reopen the editor. If ConnectionStatus is Error,
inspect its LastError or the Output Log; check the port and, when enabled,
the local password file. Stop PIE and press Play again after correcting it.

The component connects only when Auto Connect is enabled. Stop PIE closes
its socket. This first receiver requires a new Play session or its Connect
function after a server interruption; automatic retry is future work.

## 5. Prove chat arrives in Unreal

While Unreal is playing and Streamer.bot reports the server is running,
type `!rtcqttest` in Twitch website chat.

Expected Unreal Output Log entry:

```text
RTCQT chat received: test | hello from Streamer.bot
```

A cyan `RTCQT chat received: hello from Streamer.bot` message should also
appear for four seconds. This display is a connection diagnostic.

During PIE, you can inspect the playing manager's StreamerBot component:
Connection Status should be Ready, Received Command Count should increase,
and Last Command Value should be `hello from Streamer.bot`. Select the PIE
instance when inspecting runtime values.

**Pass:** one accepted command produces one received event. Repeat after
the cooldown, then stop and restart PIE and repeat. Verify the dropdown,
music, pause, and restart still work by listening and looking at the scene.

If Streamer.bot runs the action but Unreal receives nothing, first check
that PIE is running and the receiver reached Ready. Match the server endpoint,
check that the C# sub-action compiled and saved, and inspect both logs.
Malformed, duplicate, unrelated, or more-than-ten-second-old payloads are
ignored. The test action creates a fresh ID and timestamp every execution.

## Following this test

The receiver exposes Blueprint-assignable **On Command Received**, carrying
Command and Value, for the future interaction controller. Extend its allowed
commands alongside the actual palette/pulse/shape implementations. Do not
rename this test action to `!pulse` and expect an effect to appear.

## Verification recorded on 2026-10-13

The Development Editor build succeeded. All 28 RTCQT automation tests passed;
the existing Music Lab switching test retains its default-material warning.
The new protocol test checks authentication against an independent SHA256
reference, subscription readiness, envelope variants, deduplication, stale and
invalid payload rejection, and disconnect behavior.

A fresh Unreal process loaded the saved Music Lab and found the inherited
StreamerBot component. It then connected to a local mock server, subscribed,
and received two distinct test messages over a real socket while ignoring the
duplicate and expired messages. Evidence is in
`Saved/Automation/StreamerBot/Tests/index.json` and
`Saved/Automation/StreamerBot/transport.json`. This mock test is independent
of your Streamer.bot action and Twitch account; Step 5 supplies that final
workstation proof, including the on-screen display and listening check.

## References

- [Commands](https://docs.streamer.bot/guide/core/commands)
- [Command Triggered](https://docs.streamer.bot/api/triggers/core/commands/command-triggered)
- [Execute C# Code](https://docs.streamer.bot/api/sub-actions/core/csharp/execute-csharp-code)
- [WebsocketBroadcastJson](https://docs.streamer.bot/api/csharp/methods/core/websocket/websocket-broadcast-json)
- [WebSocket configuration](https://docs.streamer.bot/api/websocket/guide/configuration)
- [Authentication](https://docs.streamer.bot/api/websocket/guide/authentication)
