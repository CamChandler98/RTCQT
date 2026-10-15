#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTCQTStreamerBotComponent.generated.h"

class IWebSocket;
class FJsonObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRTCQTBotCommandReceived,
	const FString&, Command, const FString&, Value);

/** Opt-in, local Streamer.bot connection. The first supported command is a test. */
UCLASS(ClassGroup = (RTCQT), meta = (BlueprintSpawnableComponent))
class RTCQT_API URTCQTStreamerBotComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URTCQTStreamerBotComponent();
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Chat") bool Connect();
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Chat") void Disconnect();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTCQT|Chat")
	bool bAutoConnect = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTCQT|Chat")
	FString ServerUrl = TEXT("ws://127.0.0.1:8080/");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTCQT|Chat")
	bool bShowTestMessages = true;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "RTCQT|Chat")
	FString ConnectionStatus = TEXT("Disconnected");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "RTCQT|Chat")
	FString LastError;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "RTCQT|Chat")
	FString LastCommandValue;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "RTCQT|Chat")
	int32 ReceivedCommandCount = 0;
	UPROPERTY(BlueprintAssignable, Category = "RTCQT|Chat")
	FRTCQTBotCommandReceived OnCommandReceived;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	friend class FRTCQTStreamerBotProtocolTest;
	TSharedPtr<IWebSocket> Socket;
	uint64 ConnectionGeneration = 0;
	bool bReady = false;
	bool bHelloReceived = false;
	TMap<FString, FDateTime> SeenRequests;
	void HandleMessage(const FString& Message);
	void Subscribe();
	void SendJson(const TSharedRef<FJsonObject>& Object);
	void Fail(const FString& Error);
	bool AcceptCommand(const TSharedPtr<FJsonObject>& Object);
	static FString AuthenticationResponse(const FString& Password, const FString& Salt, const FString& Challenge);
};
