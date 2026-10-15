#if WITH_DEV_AUTOMATION_TESTS
#include "RTCQTStreamerBotComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTStreamerBotProtocolTest, "RTCQT.StreamerBot.Protocol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTStreamerBotProtocolTest::RunTest(const FString& Parameters)
{
	auto* Receiver = NewObject<URTCQTStreamerBotComponent>();
	Receiver->bShowTestMessages = false;
	TestFalse(TEXT("Networking is opt-in"), Receiver->bAutoConnect);
	TestEqual(TEXT("SHA256 authentication matches independent reference"),
		Receiver->AuthenticationResponse(TEXT("test-password"), TEXT("test-salt"), TEXT("test-challenge")),
		FString(TEXT("zBIloly70ANx+T9nUkQVuTCqTUuHtfrqEdn8XDQ3m/s=")));
	auto Encode = [](const TSharedRef<FJsonObject>& Object)
	{
		FString Text;
		FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
		return Text;
	};
	auto Payload = []()
	{
		auto Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("schema_version"), 1);
		Object->SetStringField(TEXT("topic"), TEXT("rtcqt.visuals"));
		Object->SetStringField(TEXT("command"), TEXT("test"));
		Object->SetStringField(TEXT("value"), TEXT("hello from Streamer.bot"));
		Object->SetStringField(TEXT("request_id"), FGuid::NewGuid().ToString());
		Object->SetStringField(TEXT("issued_at"), FDateTime::UtcNow().ToIso8601());
		return Object;
	};
	const auto First = Payload();
	Receiver->HandleMessage(Encode(First));
	TestEqual(TEXT("Ignore data before subscription"), Receiver->ReceivedCommandCount, 0);
	Receiver->HandleMessage(TEXT("{\"request\":\"Hello\",\"info\":{}}"));
	TestEqual(TEXT("Hello starts subscription"), Receiver->ConnectionStatus, FString(TEXT("Subscribing")));
	Receiver->HandleMessage(TEXT("{\"id\":\"rtcqt-subscribe\",\"status\":\"ok\"}"));
	TestTrue(TEXT("Successful acknowledgement makes receiver ready"), Receiver->bReady);
	Receiver->HandleMessage(Encode(First));
	Receiver->HandleMessage(Encode(First));
	TestEqual(TEXT("Duplicate request fires once"), Receiver->ReceivedCommandCount, 1);
	auto Event = MakeShared<FJsonObject>();
	Event->SetStringField(TEXT("source"), TEXT("General"));
	Event->SetStringField(TEXT("type"), TEXT("Custom"));
	auto Envelope = MakeShared<FJsonObject>();
	Envelope->SetObjectField(TEXT("event"), Event);
	Envelope->SetObjectField(TEXT("data"), Payload());
	Receiver->HandleMessage(Encode(Envelope));
	auto Wrapped = MakeShared<FJsonObject>();
	Wrapped->SetStringField(TEXT("data"), Encode(Payload()));
	Envelope->SetObjectField(TEXT("data"), Wrapped);
	Receiver->HandleMessage(Encode(Envelope));
	Envelope->SetStringField(TEXT("data"), Encode(Payload()));
	Receiver->HandleMessage(Encode(Envelope));
	TestEqual(TEXT("Object and string custom envelopes accepted"), Receiver->ReceivedCommandCount, 4);
	Event->SetStringField(TEXT("source"), TEXT("Twitch"));
	Envelope->SetStringField(TEXT("data"), Encode(Payload()));
	Receiver->HandleMessage(Encode(Envelope));
	TestEqual(TEXT("Unrelated events cannot inject commands"), Receiver->ReceivedCommandCount, 4);
	for (const int32 Variant : {0, 1, 2, 3, 4, 5, 6})
	{
		auto Invalid = Payload();
		switch (Variant)
		{
		case 0: Invalid->SetStringField(TEXT("topic"), TEXT("other")); break;
		case 1: Invalid->SetStringField(TEXT("command"), TEXT("exec")); break;
		case 2: Invalid->SetStringField(TEXT("issued_at"), (FDateTime::UtcNow() - FTimespan::FromSeconds(20)).ToIso8601()); break;
		case 3: Invalid->SetStringField(TEXT("issued_at"), (FDateTime::UtcNow() + FTimespan::FromSeconds(20)).ToIso8601()); break;
		case 4: Invalid->SetStringField(TEXT("value"), TEXT("fake\nlog line")); break;
		case 5: Invalid->SetNumberField(TEXT("schema_version"), 1.5); break;
		case 6: Invalid->RemoveField(TEXT("request_id")); break;
		}
		Receiver->HandleMessage(Encode(Invalid));
	}
	Receiver->HandleMessage(TEXT("not JSON"));
	TestEqual(TEXT("Invalid commands preserve accepted count"), Receiver->ReceivedCommandCount, 4);
	TestEqual(TEXT("Invalid commands preserve displayed value"), Receiver->LastCommandValue, FString(TEXT("hello from Streamer.bot")));

	auto Pulse = Payload();
	Pulse->SetStringField(TEXT("command"), TEXT("pulse"));
	Pulse->SetStringField(TEXT("value"), TEXT("gentle"));
	Receiver->HandleMessage(Encode(Pulse));
	Receiver->HandleMessage(Encode(Pulse));
	TestEqual(TEXT("Pulse command accepted once despite duplicate"), Receiver->ReceivedCommandCount, 5);
	TestEqual(TEXT("Pulse preset recorded"), Receiver->LastCommandValue, FString(TEXT("gentle")));

	auto InvalidPulse = Payload();
	InvalidPulse->SetStringField(TEXT("command"), TEXT("pulse"));
	InvalidPulse->SetStringField(TEXT("value"), TEXT("aggressive"));
	Receiver->HandleMessage(Encode(InvalidPulse));
	TestEqual(TEXT("Invalid pulse preset rejected"), Receiver->ReceivedCommandCount, 5);
	const uint64 OldGeneration = Receiver->ConnectionGeneration;
	Receiver->Disconnect();
	TestFalse(TEXT("Disconnect removes readiness"), Receiver->bReady);
	TestTrue(TEXT("Disconnect invalidates queued callbacks"), Receiver->ConnectionGeneration > OldGeneration);
	Receiver->HandleMessage(Encode(Payload()));
	TestEqual(TEXT("Disconnected receiver ignores messages"), Receiver->ReceivedCommandCount, 5);
	return true;
}
#endif
