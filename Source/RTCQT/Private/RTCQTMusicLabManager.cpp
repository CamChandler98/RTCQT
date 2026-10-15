#include "RTCQTMusicLabManager.h"
#include "RTCQTMusicLabControls.h"
#include "RTCQTStreamerBotComponent.h"
#include "RTCQTPaletteComponent.h"
#include "RTCQTVisualizer.h"
#include "RTCQTAnalyzerQuery.h"
#include "Components/AudioComponent.h"
#include "ConstantQNRT.h"
#include "Sound/SoundWave.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ARTCQTMusicLabManager::ARTCQTMusicLabManager()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("MusicAudio"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = false;
	StreamerBot = CreateDefaultSubobject<URTCQTStreamerBotComponent>(TEXT("StreamerBot"));
	Palette = CreateDefaultSubobject<URTCQTPaletteComponent>(TEXT("Palette"));
}

bool ARTCQTMusicLabManager::IsValidTrack(const FRTCQTMusicTrack& Track) const
{
	if (Track.Id.IsEmpty() || Track.Label.IsEmpty() || !Track.SoundWave || !Track.Analyzer
		|| Track.Analyzer->Sound != Track.SoundWave || Track.Channel < 0
		|| !FMath::IsFinite(Track.SoundWave->Duration) || Track.SoundWave->Duration <= 0
		|| Track.SoundWave->bLooping) return false;
	if (FMath::Abs(Track.Analyzer->DurationInSeconds - Track.SoundWave->Duration) > 0.1f) return false;
	if (!Track.Analyzer->Settings || Track.Analyzer->Settings->NumBands < 1 || Track.Analyzer->Settings->NumBands > 96) return false;
	TArray<float> Bands;
	for (const float Time : {0.f, Track.SoundWave->Duration * 0.5f})
	{
		if (!RTCQTAnalysis::QueryBandsAtTime(Track.Analyzer, Time, Track.Channel, Track.Analyzer->Settings->NumBands, Bands).bSucceeded) return false;
		for (float Band : Bands) if (!FMath::IsFinite(Band) || Band < 0 || Band > 1) return false;
	}
	return true;
}

bool ARTCQTMusicLabManager::SelectTrack(const FString& TrackId)
{
	const FRTCQTMusicTrack* Found = nullptr;
	if (Catalog) for (const auto& Track : Catalog->Tracks)
	{
		if (Track.Id != TrackId) continue;
		if (Found) { LastError = TEXT("Duplicate track ID"); return false; }
		Found = &Track;
	}
	if (!Found || !IsValidTrack(*Found)) { LastError = TEXT("Track pair is incomplete or invalid"); return false; }
	if (CurrentTrack.Id == TrackId && PlaybackState != ERTCQTMusicPlaybackState::Stopped) return true;
	return StartTrack(*Found);
}

void ARTCQTMusicLabManager::RetireAudio()
{
	if (!AudioComponent) return;
	AudioComponent->OnAudioPlaybackPercentNative.RemoveAll(this);
	AudioComponent->OnAudioFinishedNative.RemoveAll(this);
	AudioComponent->Stop();
	AudioComponent->DestroyComponent();
	AudioComponent = nullptr;
}

bool ARTCQTMusicLabManager::StartTrack(const FRTCQTMusicTrack& Track)
{
	if (!IsValidTrack(Track)) { LastError = TEXT("Track pair is incomplete or invalid"); return false; }
	RetireAudio();
	PlaybackState = ERTCQTMusicPlaybackState::Stopped;
	PlaybackPosition = 0;
	PlaybackCallbackCount = 0;
	PlaybackStartedAt = FPlatformTime::Seconds();
	PausedWallSeconds = 0;
	PausedAt = 0;
	CurrentTrack = Track;
	ActiveAnalyzer = Track.Analyzer;
	if (Visualizer) Visualizer->SetPlaybackAnalyzer(Track.Analyzer, Track.Channel);
	// New component ID prevents queued callbacks from an earlier A in A/B/A
	// being dispatched to the current playback. Only one component remains live.
	AudioComponent = NewObject<UAudioComponent>(this);
	AudioComponent->bAutoActivate = false;
	AudioComponent->bAutoDestroy = false;
	AudioComponent->bAllowSpatialization = false;
	AudioComponent->bIsUISound = true;
	AudioComponent->bCanPlayMultipleInstances = false;
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->RegisterComponent();
	AudioComponent->SetSound(Track.SoundWave);
	AudioComponent->OnAudioPlaybackPercentNative.AddUObject(this, &ARTCQTMusicLabManager::PlaybackPercent);
	AudioComponent->OnAudioFinishedNative.AddUObject(this, &ARTCQTMusicLabManager::Finished);
	PlaybackState = ERTCQTMusicPlaybackState::Playing;
	AudioComponent->Play(0);
	LastError.Reset();
	return true;
}

void ARTCQTMusicLabManager::PlaybackPercent(const UAudioComponent* Component, const USoundWave* Sound, float Percent)
{
	if (Component != AudioComponent || Sound != CurrentTrack.SoundWave
		|| PlaybackState != ERTCQTMusicPlaybackState::Playing || !FMath::IsFinite(Percent)) return;
	// Use mixer frames, never an accumulated game-frame clock. Ignore regressing
	// callbacks; each switch/restart explicitly resets the position.
	const float Position = FMath::Clamp(Percent, 0.f, 1.f) * CurrentTrack.SoundWave->Duration;
	// Recycled mixer voices can briefly publish their previous frame counter.
	// Wall time is only a plausibility bound, never the visualizer's clock.
	const double AvailableSeconds = FMath::Max(0.0, FPlatformTime::Seconds() - PlaybackStartedAt - PausedWallSeconds);
	if (Position > AvailableSeconds + 0.25) return;
	if (Position < PlaybackPosition) return;
	PlaybackPosition = Position;
	++PlaybackCallbackCount;
	if (Visualizer) Visualizer->EvaluateAtTime(FMath::Min(PlaybackPosition, ActiveAnalyzer->DurationInSeconds));
}

void ARTCQTMusicLabManager::Finished(UAudioComponent* Component)
{
	if (Component != AudioComponent) return;
	PlaybackState = ERTCQTMusicPlaybackState::Stopped;
	if (Visualizer) Visualizer->ClearVisualization();
}

bool ARTCQTMusicLabManager::Restart() { return StartTrack(CurrentTrack); }

void ARTCQTMusicLabManager::TogglePlayPause()
{
	if (PlaybackState == ERTCQTMusicPlaybackState::Stopped) { Restart(); return; }
	const bool Pause = PlaybackState == ERTCQTMusicPlaybackState::Playing;
	if (Pause) PausedAt = FPlatformTime::Seconds();
	else PausedWallSeconds += FPlatformTime::Seconds() - PausedAt;
	AudioComponent->SetPaused(Pause);
	PlaybackState = Pause ? ERTCQTMusicPlaybackState::Paused : ERTCQTMusicPlaybackState::Playing;
}

void ARTCQTMusicLabManager::BeginPlay()
{
	Super::BeginPlay();
	if (DemoCamera) if (auto* PC = UGameplayStatics::GetPlayerController(this, 0)) PC->SetViewTarget(DemoCamera);
	if (ControlsClass)
	{
		if (auto* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			Controls = CreateWidget<URTCQTMusicLabControls>(PC, ControlsClass);
			Controls->Manager = this;
			Controls->AddToViewport();
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI Mode;
			PC->SetInputMode(Mode);
		}
	}
	if (bAutoPlay && Catalog)
	{
		if (!InitialTrackId.IsEmpty()) SelectTrack(InitialTrackId);
		else for (const auto& Track : Catalog->Tracks) if (IsValidTrack(Track)) { SelectTrack(Track.Id); break; }
	}
}

void ARTCQTMusicLabManager::EndPlay(const EEndPlayReason::Type Reason)
{
	RetireAudio();
	if (Controls) Controls->RemoveFromParent();
	Super::EndPlay(Reason);
}
