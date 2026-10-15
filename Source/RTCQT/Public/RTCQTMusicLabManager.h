#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "RTCQTMusicLabManager.generated.h"

class USoundWave;
class UConstantQNRT;
class UAudioComponent;
class ARTCQTVisualizer;
class URTCQTMusicLabControls;
class URTCQTStreamerBotComponent;

USTRUCT(BlueprintType)
struct RTCQT_API FRTCQTMusicTrack
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundWave> SoundWave;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UConstantQNRT> Analyzer;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Channel = 0;
};

/** Editor-generated projection of the importer's schema_version=1 catalog. */
UCLASS(BlueprintType)
class RTCQT_API URTCQTMusicCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FRTCQTMusicTrack> Tracks;
};

UENUM(BlueprintType)
enum class ERTCQTMusicPlaybackState : uint8 { Stopped, Playing, Paused };

UCLASS(Blueprintable)
class RTCQT_API ARTCQTMusicLabManager : public AActor
{
	GENERATED_BODY()
public:
	ARTCQTMusicLabManager();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTCQT|Chat")
	TObjectPtr<URTCQTStreamerBotComponent> StreamerBot;
	UFUNCTION(BlueprintCallable) bool SelectTrack(const FString& TrackId);
	UFUNCTION(BlueprintCallable) bool Restart();
	UFUNCTION(BlueprintCallable) void TogglePlayPause();
	bool IsValidTrack(const FRTCQTMusicTrack& Track) const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<URTCQTMusicCatalog> Catalog;
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly) TObjectPtr<ARTCQTVisualizer> Visualizer;
	UPROPERTY(EditInstanceOnly) TObjectPtr<AActor> DemoCamera;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString InitialTrackId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAutoPlay = true;
	UPROPERTY(EditAnywhere) TSubclassOf<URTCQTMusicLabControls> ControlsClass;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) FRTCQTMusicTrack CurrentTrack;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) TObjectPtr<UConstantQNRT> ActiveAnalyzer;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) TObjectPtr<UAudioComponent> AudioComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) ERTCQTMusicPlaybackState PlaybackState = ERTCQTMusicPlaybackState::Stopped;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) float PlaybackPosition = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) int32 PlaybackCallbackCount = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient) FString LastError;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	friend class FRTCQTMusicLabSwitchTest;
	double PlaybackStartedAt = 0;
	double PausedAt = 0;
	double PausedWallSeconds = 0;
	bool StartTrack(const FRTCQTMusicTrack& Track);
	void RetireAudio();
	void PlaybackPercent(const UAudioComponent* Component, const USoundWave* Sound, float Percent);
	void Finished(UAudioComponent* Component);
	UPROPERTY(Transient) TObjectPtr<URTCQTMusicLabControls> Controls;
};
