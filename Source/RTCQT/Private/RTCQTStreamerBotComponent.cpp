#include "RTCQTStreamerBotComponent.h"

#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "IWebSocket.h"
#include "Misc/Base64.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "RTCQTLog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "WebSocketsModule.h"
THIRD_PARTY_INCLUDES_START
#include <openssl/sha.h>
THIRD_PARTY_INCLUDES_END

namespace
{
bool ReadJson(const FString& Text, TSharedPtr<FJsonObject>& Object)
{
	return Text.Len() <= 8192 && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object)
		&& Object.IsValid();
}

FString HashBase64(const FString& Text)
{
	const FTCHARToUTF8 Bytes(*Text);
	uint8 Hash[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char*>(Bytes.Get()), Bytes.Length(), Hash);
	return FBase64::Encode(Hash, sizeof(Hash));
}
}

URTCQTStreamerBotComponent::URTCQTStreamerBotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URTCQTStreamerBotComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoConnect) Connect();
}

void URTCQTStreamerBotComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	Disconnect();
	Super::EndPlay(Reason);
}

void URTCQTStreamerBotComponent::Disconnect()
{
	++ConnectionGeneration;
	bReady = false;
	bHelloReceived = false;
	if (Socket)
	{
		Socket->OnConnected().Clear();
		Socket->OnConnectionError().Clear();
		Socket->OnClosed().Clear();
		Socket->OnMessage().Clear();
		Socket->Close();
		Socket.Reset();
	}
	ConnectionStatus = TEXT("Disconnected");
}

bool URTCQTStreamerBotComponent::Connect()
{
	Disconnect();
	LastError.Reset();
	// This prototype connects only to the workstation's local bot server.
	if (!ServerUrl.StartsWith(TEXT("ws://127.0.0.1:")) && !ServerUrl.StartsWith(TEXT("ws://localhost:")))
	{
		Fail(TEXT("Use a local ws://127.0.0.1:PORT/ Streamer.bot endpoint."));
		return false;
	}
	ConnectionStatus = TEXT("Connecting");
	const uint64 Generation = ConnectionGeneration;
	const TWeakObjectPtr<URTCQTStreamerBotComponent> Weak(this);
	Socket = FWebSocketsModule::Get().CreateWebSocket(ServerUrl);
	Socket->SetTextMessageMemoryLimit(8192);
	Socket->OnConnected().AddLambda([Weak, Generation]()
	{
		AsyncTask(ENamedThreads::GameThread, [Weak, Generation]()
		{
			if (auto* Self = Weak.Get(); Self && Self->ConnectionGeneration == Generation)
			{
				Self->ConnectionStatus = TEXT("Waiting for Hello");
				UE_LOG(LogRTCQT, Log, TEXT("Streamer.bot connected; waiting for Hello."));
			}
		});
	});
	Socket->OnConnectionError().AddLambda([Weak, Generation](const FString&)
	{
		AsyncTask(ENamedThreads::GameThread, [Weak, Generation]()
		{
			if (auto* Self = Weak.Get(); Self && Self->ConnectionGeneration == Generation)
				Self->Fail(TEXT("Connection failed. Check Streamer.bot server/port, then stop and Play again."));
		});
	});
	Socket->OnClosed().AddLambda([Weak, Generation](int32, const FString&, bool)
	{
		AsyncTask(ENamedThreads::GameThread, [Weak, Generation]()
		{
			if (auto* Self = Weak.Get(); Self && Self->ConnectionGeneration == Generation)
				Self->Fail(TEXT("Connection closed. Check Streamer.bot, then stop and Play again."));
		});
	});
	Socket->OnMessage().AddLambda([Weak, Generation](const FString& Message)
	{
		AsyncTask(ENamedThreads::GameThread, [Weak, Generation, Message]()
		{
			if (auto* Self = Weak.Get(); Self && Self->ConnectionGeneration == Generation)
				Self->HandleMessage(Message);
		});
	});
	Socket->Connect();
	return true;
}

void URTCQTStreamerBotComponent::Fail(const FString& Error)
{
	Disconnect();
	ConnectionStatus = TEXT("Error");
	LastError = Error;
	UE_LOG(LogRTCQT, Warning, TEXT("Streamer.bot: %s"), *Error);
}

void URTCQTStreamerBotComponent::SendJson(const TSharedRef<FJsonObject>& Object)
{
	if (!Socket || !Socket->IsConnected()) return;
	FString Text;
	FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
	Socket->Send(Text);
}

FString URTCQTStreamerBotComponent::AuthenticationResponse(const FString& Password, const FString& Salt, const FString& Challenge)
{
	return HashBase64(HashBase64(Password + Salt) + Challenge);
}

void URTCQTStreamerBotComponent::Subscribe()
{
	ConnectionStatus = TEXT("Subscribing");
	auto Request = MakeShared<FJsonObject>();
	Request->SetStringField(TEXT("request"), TEXT("Subscribe"));
	Request->SetStringField(TEXT("id"), TEXT("rtcqt-subscribe"));
	auto Events = MakeShared<FJsonObject>();
	Events->SetArrayField(TEXT("General"), {MakeShared<FJsonValueString>(TEXT("Custom"))});
	Request->SetObjectField(TEXT("events"), Events);
	SendJson(Request);
}

void URTCQTStreamerBotComponent::HandleMessage(const FString& Message)
{
	check(IsInGameThread());
	TSharedPtr<FJsonObject> Root;
	if (!ReadJson(Message, Root)) return;
	FString Request, Id, Status;
	Root->TryGetStringField(TEXT("request"), Request);
	Root->TryGetStringField(TEXT("id"), Id);
	Root->TryGetStringField(TEXT("status"), Status);
	if (Request == TEXT("Hello"))
	{
		if (bHelloReceived) return;
		bHelloReceived = true;
		const TSharedPtr<FJsonObject>* Auth;
		if (!Root->TryGetObjectField(TEXT("authentication"), Auth)) { Subscribe(); return; }
		FString Salt, Challenge, Password;
		FConfigFile PrivateConfig;
		PrivateConfig.Read(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Config/StreamerBot.ini")));
		PrivateConfig.GetString(TEXT("RTCQT.StreamerBot"), TEXT("Password"), Password);
		if (Password.IsEmpty() || !(*Auth)->TryGetStringField(TEXT("salt"), Salt)
			|| !(*Auth)->TryGetStringField(TEXT("challenge"), Challenge))
		{
			Fail(TEXT("Authentication needs Password in Saved/Config/StreamerBot.ini [RTCQT.StreamerBot]."));
			return;
		}
		auto Reply = MakeShared<FJsonObject>();
		Reply->SetStringField(TEXT("request"), TEXT("Authenticate"));
		Reply->SetStringField(TEXT("id"), TEXT("rtcqt-auth"));
		Reply->SetStringField(TEXT("authentication"), AuthenticationResponse(Password, Salt, Challenge));
		ConnectionStatus = TEXT("Authenticating");
		SendJson(Reply);
		return;
	}
	if (Id == TEXT("rtcqt-auth") && ConnectionStatus == TEXT("Authenticating"))
	{
		if (Status == TEXT("ok")) Subscribe();
		else Fail(TEXT("Authentication failed. Match the local WebSocket password, then stop and Play again."));
		return;
	}
	if (Id == TEXT("rtcqt-subscribe") && ConnectionStatus == TEXT("Subscribing"))
	{
		if (Status != TEXT("ok")) { Fail(TEXT("Custom-event subscription failed.")); return; }
		bReady = true;
		ConnectionStatus = TEXT("Ready");
		UE_LOG(LogRTCQT, Log, TEXT("Streamer.bot ready; type !rtcqttest in Twitch chat."));
		return;
	}
	if (!bReady) return;
	const TSharedPtr<FJsonObject>* Event;
	if (Root->TryGetObjectField(TEXT("event"), Event))
	{
		FString Source, Type;
		if (!(*Event)->TryGetStringField(TEXT("source"), Source) || Source != TEXT("General")
			|| !(*Event)->TryGetStringField(TEXT("type"), Type) || Type != TEXT("Custom")) return;
		const TSharedPtr<FJsonObject>* Data;
		FString Encoded;
		if (Root->TryGetStringField(TEXT("data"), Encoded))
		{
			if (!ReadJson(Encoded, Root)) return;
		}
		else if (Root->TryGetObjectField(TEXT("data"), Data))
		{
			TSharedPtr<FJsonObject> Payload = *Data;
			if (Payload->TryGetStringField(TEXT("data"), Encoded))
			{
				if (!ReadJson(Encoded, Payload)) return;
			}
			Root = Payload;
		}
		else return;
	}
	AcceptCommand(Root);
}

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