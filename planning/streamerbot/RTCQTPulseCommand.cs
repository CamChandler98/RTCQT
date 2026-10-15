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
