#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "RTCQTMusicLabManager.h"
#include "RTCQTVisualizer.h"
#include "RTCQTResponseSettings.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundWave.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTMusicLabSwitchTest, "RTCQT.MusicLab.Switching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTMusicLabSwitchTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Spawn;
	Spawn.bDeferConstruction = true;
	Spawn.ObjectFlags |= RF_Transient;
	auto* Visualizer = World->SpawnActor<ARTCQTVisualizer>(ARTCQTVisualizer::StaticClass(), FTransform::Identity, Spawn);
	auto* Settings = NewObject<URTCQTResponseSettings>(Visualizer);
	auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	FindFProperty<FObjectPropertyBase>(Visualizer->GetClass(), TEXT("ResponseSettings"))->SetObjectPropertyValue_InContainer(Visualizer, Settings);
	FindFProperty<FObjectPropertyBase>(Visualizer->GetClass(), TEXT("InstanceMesh"))->SetObjectPropertyValue_InContainer(Visualizer, Cube);
	Visualizer->FinishSpawning(FTransform::Identity);
	auto* Manager = World->SpawnActor<ARTCQTMusicLabManager>();
	Manager->bAutoPlay = false;
	Manager->Visualizer = Visualizer;
	Manager->Catalog = NewObject<URTCQTMusicCatalog>(Manager);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FRTCQTMusicTrack Track;
		Track.Id = Index == 0 ? TEXT("A") : Index == 1 ? TEXT("B") : TEXT("C");
		Track.Label = TEXT("Duplicate title");
		Track.SoundWave = NewObject<USoundWave>(Manager);
		Track.SoundWave->Duration = 60;
		Track.Analyzer = NewObject<UConstantQNRT>(Manager);
		Track.Analyzer->Sound = Track.SoundWave;
		Track.Analyzer->Settings = NewObject<UConstantQNRTSettings>(Track.Analyzer);
		const int32 Count = Index == 0 ? 16 : Index == 1 ? 64 : 32;
		Track.Analyzer->Settings->NumBands = Count;
		Track.Analyzer->Settings->NoiseFloorDb = -60;
		auto Data = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
		TArray<float> First, Last;
		First.Init(Index == 0 ? -30.f : -15.f, Count);
		Last.Init(0.f, Count);
		Data->AddFrame(0, 0, First);
		Data->AddFrame(0, 60, Last);
		Data->SetDurationInSeconds(60);
		Data->SortChronologically();
		Track.Analyzer->SetResult(Data);
		Manager->Catalog->Tracks.Add(Track);
	}
	TestTrue(TEXT("Select A"), Manager->SelectTrack(TEXT("A")));
	auto* FirstAudio = Manager->AudioComponent.Get();
	FirstAudio->OnAudioPlaybackPercentNative.Broadcast(FirstAudio, Manager->CurrentTrack.SoundWave, 0.5f);
	TestEqual(TEXT("Recycled voice's stale initial frame counter rejected"), Manager->PlaybackPosition, 0.f);
	Manager->PlaybackStartedAt = FPlatformTime::Seconds() - 30;
	FirstAudio->OnAudioPlaybackPercentNative.Broadcast(FirstAudio, Manager->CurrentTrack.SoundWave, 0.5f);
	TestEqual(TEXT("Mixer percent maps to 30 seconds"), Manager->PlaybackPosition, 30.f);
	auto* Meshes = Visualizer->FindComponentByClass<UInstancedStaticMeshComponent>();
	TestEqual(TEXT("16-band analyzer builds 16 cubes"), Meshes->GetInstanceCount(), 16);
	FTransform FirstTransform, LastTransform;
	Meshes->GetInstanceTransform(0, FirstTransform, false);
	Meshes->GetInstanceTransform(15, LastTransform, false);
	const double Span16 = LastTransform.GetTranslation().X - FirstTransform.GetTranslation().X;
	TestFalse(TEXT("Unknown ID rejected"), Manager->SelectTrack(TEXT("unknown")));
	TestEqual(TEXT("Invalid request preserves audio component"), Manager->AudioComponent.Get(), FirstAudio);
	TestEqual(TEXT("Invalid request preserves position"), Manager->PlaybackPosition, 30.f);
	TestTrue(TEXT("Same A is no-op"), Manager->SelectTrack(TEXT("A")));
	TestEqual(TEXT("No-op preserves component"), Manager->AudioComponent.Get(), FirstAudio);
	TestTrue(TEXT("Switch B"), Manager->SelectTrack(TEXT("B")));
	TestTrue(TEXT("Prior component destroyed"), !FirstAudio->IsRegistered());
	TestEqual(TEXT("Switch resets position"), Manager->PlaybackPosition, 0.f);
	TestEqual(TEXT("B analyzer matches B sound"), Manager->ActiveAnalyzer->Sound.Get(), Manager->CurrentTrack.SoundWave.Get());
	auto* Instances = Visualizer->FindComponentByClass<UInstancedStaticMeshComponent>();
	TestEqual(TEXT("Stale visual frame cleared before first callback"), Instances->GetInstanceCount(), 0);
	Manager->PlaybackStartedAt = FPlatformTime::Seconds() - 15;
	Manager->AudioComponent->OnAudioPlaybackPercentNative.Broadcast(Manager->AudioComponent, Manager->CurrentTrack.SoundWave, 0.25f);
	TestEqual(TEXT("64-band analyzer builds 64 cubes"), Instances->GetInstanceCount(), 64);
	Instances->GetInstanceTransform(0, FirstTransform, false);
	Instances->GetInstanceTransform(63, LastTransform, false);
	const double Span64 = LastTransform.GetTranslation().X - FirstTransform.GetTranslation().X;
	TestTrue(TEXT("Display center span survives 16 to 64 switch"), FMath::IsNearlyEqual(Span16, Span64, 1.e-3));
	TestTrue(TEXT("Switch back A"), Manager->SelectTrack(TEXT("A")));
	auto* CurrentAudio = Manager->AudioComponent.Get();
	FirstAudio->OnAudioPlaybackPercentNative.Broadcast(FirstAudio, Manager->CurrentTrack.SoundWave, 0.9f);
	TestEqual(TEXT("Old A callbacks cannot advance new A"), Manager->PlaybackPosition, 0.f);
	CurrentAudio->OnAudioPlaybackPercentNative.Broadcast(CurrentAudio, Manager->CurrentTrack.SoundWave, 0.5f);
	TestEqual(TEXT("New component also rejects stale mixer voice frames"), Manager->PlaybackPosition, 0.f);
	Manager->PlaybackStartedAt = FPlatformTime::Seconds() - 15;
	CurrentAudio->OnAudioPlaybackPercentNative.Broadcast(CurrentAudio, Manager->CurrentTrack.SoundWave, 0.25f);
	TestEqual(TEXT("New A audio position drives analyzer"), Manager->PlaybackPosition, 15.f);
	TestEqual(TEXT("Visualizer populated by new callback"), Instances->GetInstanceCount(), 16);
	Manager->TogglePlayPause();
	CurrentAudio->OnAudioPlaybackPercentNative.Broadcast(CurrentAudio, Manager->CurrentTrack.SoundWave, 0.5f);
	TestEqual(TEXT("Pause freezes position"), Manager->PlaybackPosition, 15.f);
	Manager->TogglePlayPause();
	TestTrue(TEXT("Restart A"), Manager->Restart());
	TestEqual(TEXT("Restart resets position"), Manager->PlaybackPosition, 0.f);
	TestTrue(TEXT("Restart uses fresh audio identity"), Manager->AudioComponent != CurrentAudio);
	Manager->Catalog->Tracks[1].Analyzer->Sound = Manager->CurrentTrack.SoundWave;
	TestFalse(TEXT("Mismatched pair rejected before stop"), Manager->SelectTrack(TEXT("B")));
	TestEqual(TEXT("A stays selected"), Manager->CurrentTrack.Id, FString(TEXT("A")));
	const FRTCQTMusicTrack Duplicate = Manager->Catalog->Tracks[0];
	Manager->Catalog->Tracks.Add(Duplicate);
	TestFalse(TEXT("Duplicate ID rejected"), Manager->SelectTrack(TEXT("A")));
	TestTrue(TEXT("32-band track also accepted"), Manager->SelectTrack(TEXT("C")));
	Manager->PlaybackStartedAt = FPlatformTime::Seconds() - 15;
	Manager->AudioComponent->OnAudioPlaybackPercentNative.Broadcast(Manager->AudioComponent, Manager->CurrentTrack.SoundWave, 0.25f);
	TestEqual(TEXT("32 cubes after 16/64/16/restart/32"), Instances->GetInstanceCount(), 32);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTFrequencyWeightTest, "RTCQT.MusicLab.FrequencyWeights",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTCQTFrequencyWeightTest::RunTest(const FString& Parameters)
{
	auto* Settings = NewObject<URTCQTResponseSettings>();
	Settings->BandWeights = {1.5f};
	FRTCQTFrequencyWeight Bass;
	Bass.MinHz = 40; Bass.MaxHz = 160; Bass.Gain = 2;
	FRTCQTFrequencyWeight Overlap;
	Overlap.MinHz = 80; Overlap.MaxHz = 160; Overlap.Gain = 0.5;
	FRTCQTFrequencyWeight Mute;
	Mute.MinHz = 160; Mute.MaxHz = 320; Mute.Gain = 0;
	Settings->FrequencyWeights = {Bass, Overlap, Mute};
	const auto A = Settings->ResolveBandWeights(16, 40, 1);
	const auto B = Settings->ResolveBandWeights(32, 40, 2);
	const auto C = Settings->ResolveBandWeights(64, 40, 4);
	TestEqual(TEXT("Index and frequency gains multiply"), A[0], 3.f);
	TestEqual(TEXT("Overlapping gains multiply"), A[1], 1.f);
	TestEqual(TEXT("Range begins inclusively at 160 Hz"), A[2], 0.f);
	TestEqual(TEXT("320 Hz upper edge excluded"), A[3], 1.f);
	TestEqual(TEXT("Same 80 Hz gain on 32 bands"), B[2], A[1]);
	TestEqual(TEXT("Same 80 Hz gain on 64 bands"), C[4], A[1]);
	TestEqual(TEXT("Starting frequency changes the mapping"), Settings->ResolveBandWeights(16, 80, 1)[0], 1.5f);
	TestTrue(TEXT("Invalid count rejected"), Settings->ResolveBandWeights(0, 40, 1).IsEmpty());
	TestTrue(TEXT("Invalid octave spacing rejected"), Settings->ResolveBandWeights(16, 40, 0).IsEmpty());
	Settings->FrequencyWeights.Reset();
	const auto Neutral = Settings->ResolveBandWeights(96, 40, 8);
	TestEqual(TEXT("Missing index weights default to one at 96 bands"), Neutral[95], 1.f);
	return true;
}
#endif
