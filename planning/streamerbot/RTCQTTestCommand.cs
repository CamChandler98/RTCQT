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
