#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "RTCQTResponseSettings.h"
#include "RTCQTVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Compilation/MovieSceneCompiledDataManager.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InstancedStaticMeshDelegates.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Sound/SoundWave.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace RTCQTSequenceTests
{
struct FSequenceFixture
{
	UWorld* World;
	ARTCQTVisualizer* Actor;
	UConstantQNRT* Analyzer;
	URTCQTResponseSettings* Settings;
	UInstancedStaticMeshComponent* Instances;
	ULevelSequence* Sequence;
	ULevelSequencePlayer* Player;
	FMovieSceneObjectBindingID BindingId;
	int32 TopologyChanges = 0;
	FDelegateHandle TopologyHandle;

	FSequenceFixture()
	{
		const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
			.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
			ERHIFeatureLevel::Num, &Values);
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		FActorSpawnParameters Spawn;
		Spawn.bDeferConstruction = true;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<ARTCQTVisualizer>(ARTCQTVisualizer::StaticClass(), FTransform::Identity, Spawn);
		Analyzer = NewObject<UConstantQNRT>(Actor);
		Analyzer->Sound = NewObject<USoundWave>(Analyzer);
		Analyzer->Settings->NoiseFloorDb = -60.0f;
		auto Data = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
		TArray<float> First, Last;
		for (int32 Index = 0; Index < 32; ++Index)
		{
			First.Add(-60.0f + 60.0f * Index / 31.0f);
			Last.Add(-60.0f * Index / 31.0f);
		}
		Data->AddFrame(0, 0.0f, First);
		Data->AddFrame(0, 60.0f, Last);
		Data->SetDurationInSeconds(60.0f);
		Data->SortChronologically();
		Analyzer->SetResult(Data);
		Settings = NewObject<URTCQTResponseSettings>(Actor);
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		SetAsset(TEXT("ConstantQNRT"), Analyzer);
		SetAsset(TEXT("ResponseSettings"), Settings);
		SetAsset(TEXT("InstanceMesh"), Cube);
		SetAsset(TEXT("InstanceMaterial"), Cube->GetMaterial(0));
		Actor->FinishSpawning(FTransform::Identity);
		Instances = Actor->FindComponentByClass<UInstancedStaticMeshComponent>();

		// Runtime property systems require the same initialized-for-play
		// world lifecycle used by a real saved-level preview.
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		Sequence = NewObject<ULevelSequence>(World);
		Sequence->SetFlags(RF_Transactional);
		Sequence->Initialize();
		Sequence->SetSequenceFlags(EMovieSceneSequenceFlags::BlockingEvaluation);
		UMovieScene* Scene = Sequence->GetMovieScene();
		Scene->SetDisplayRate(FFrameRate(60, 1));
		Scene->SetTickResolutionDirectly(FFrameRate(24000, 1));
		Scene->SetEvaluationType(EMovieSceneEvaluationType::WithSubFrames);
		Scene->SetPlaybackRange(0, 1440000);
		const FGuid Binding = Scene->AddPossessable(TEXT("Visualizer"), Actor->GetClass());
		BindingId = UE::MovieScene::FRelativeObjectBindingID(Binding);
		Sequence->BindPossessableObject(Binding, *Actor, World);
		UMovieSceneFloatTrack* Track = Scene->AddTrack<UMovieSceneFloatTrack>(Binding);
		Track->SetPropertyNameAndPath(TEXT("SequenceTimeSeconds"), TEXT("SequenceTimeSeconds"));
		UMovieSceneFloatSection* Section = CastChecked<UMovieSceneFloatSection>(Track->CreateNewSection());
		Section->SetRange(TRange<FFrameNumber>(0, 1440001));
		Section->GetChannel().SetTickResolution(FFrameRate(24000, 1));
		Section->SetCompletionMode(EMovieSceneCompletionMode::KeepState);
		Section->GetChannel().AddLinearKey(0, 0.0f);
		Section->GetChannel().AddLinearKey(1440000, 60.0f);
		Track->AddSection(*Section);
		Section->MarkAsChanged();
		Track->MarkAsChanged();
		Scene->MarkAsChanged();
		UMovieSceneCompiledDataManager* Manager = UMovieSceneCompiledDataManager::GetPrecompiledData();
		Manager->Compile(Sequence);
		ALevelSequenceActor* SequenceActor = nullptr;
		FMovieSceneSequencePlaybackSettings Playback;
		Playback.bDisableCameraCuts = true;
		Player = ULevelSequencePlayer::CreateLevelSequencePlayer(World, Sequence, Playback, SequenceActor);
		// Transient test actors have no persistent level locator. Explicitly
		// bind that actor; the saved scene separately tests asset possessables.
		SequenceActor->SetBinding(BindingId, {Actor});
		TopologyHandle = FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.AddLambda(
			[this](UInstancedStaticMeshComponent* Component,
				TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Updates)
			{
				if (Component == Instances) TopologyChanges += Updates.Num();
			});
	}

	~FSequenceFixture()
	{
		if (Player) Player->Stop();
		FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.Remove(TopologyHandle);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	void SetAsset(const TCHAR* Name, UObject* Value)
	{
		FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name)->SetObjectPropertyValue_InContainer(Actor, Value);
	}

	void Seek(float Seconds, EUpdatePositionMethod Method = EUpdatePositionMethod::Jump)
	{
		Player->SetPlaybackPosition(FMovieSceneSequencePlaybackParams(Seconds, Method));
	}
};

void CheckFrame(FAutomationTestBase& Test, FSequenceFixture& F, double Seconds)
{
	Test.TestTrue(TEXT("Real float track invokes the evaluation setter"), F.Actor->LastSequenceEvaluation.bSucceeded);
	Test.TestTrue(TEXT("Applied evaluation time matches the playhead"),
		FMath::Abs(F.Actor->LastSequenceEvaluation.EvaluatedTimeSeconds - Seconds) < 1.e-4);
	Test.TestTrue(TEXT("Animated property matches the playhead"), FMath::Abs(F.Actor->SequenceTimeSeconds - Seconds) < 1.e-4);
	Test.TestEqual(TEXT("Sequence evaluation keeps 32 instances"), F.Instances->GetInstanceCount(), 32);
	if (F.Instances->GetInstanceCount() != 32) return;
	for (int32 Index = 0; Index < 32; ++Index)
	{
		const double Start = Index / 31.0;
		const double Normalized = FMath::Lerp(Start, 1.0 - Start, Seconds / 60.0);
		const double Response = FMath::Square(FMath::Clamp((Normalized - 0.1) / 0.9, 0.0, 1.0));
		Test.TestTrue(TEXT("Response matches independent linear analyzer/quadratic response calculation"),
			FMath::Abs(F.Instances->PerInstanceSMCustomData[Index * 2] - Response) < 1.e-6);
		FTransform Transform;
		F.Instances->GetInstanceTransform(Index, Transform, false);
		Test.TestTrue(TEXT("Real sequence track applies response height"),
			FMath::Abs(Transform.GetScale3D().Z - (0.1 + 3.9 * Response)) < 1.e-6);
		Test.TestTrue(TEXT("Band-index custom data stays intact"),
			FMath::Abs(F.Instances->PerInstanceSMCustomData[Index * 2 + 1] - Start) < 1.e-6);
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTSequenceSeeking,
	"RTCQT.D13.Sequence.RealTrackSeekingAndReferenceFrames", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTSequenceSeeking::RunTest(const FString& Parameters)
{
	using namespace RTCQTSequenceTests;
	FSequenceFixture F;
	if (!TestNotNull(TEXT("Native Level Sequence player initialized"), F.Player)) return false;
	TestEqual(TEXT("Transient possessable resolves to exactly one actor"), F.Player->GetBoundObjects(F.BindingId).Num(), 1);
	TestTrue(TEXT("Player evaluation template is valid"), F.Player->IsValid());
	UMaterialInterface* Material = F.Instances->GetMaterial(0);
	TestNotNull(TEXT("One shared dynamic material"), Cast<UMaterialInstanceDynamic>(Material));
	TArray<float> First;
	for (float Time : {16.75f, 0.75f, 44.25f, 2.0f, 11.75f, 33.25f, 16.75f})
	{
		F.Seek(Time);
		CheckFrame(*this, F, Time);
		if (First.IsEmpty()) First = F.Instances->PerInstanceSMCustomData;
		TestTrue(TEXT("Seeking reuses the shared material"), F.Instances->GetMaterial(0) == Material);
	}
	TestTrue(TEXT("Returning to the reference restores exact custom data"), First == F.Instances->PerInstanceSMCustomData);
	F.Seek(16.75f);
	CheckFrame(*this, F, 16.75);
	TestTrue(TEXT("Repeating the same seek restores exact inputs"), First == F.Instances->PerInstanceSMCustomData);
	TestEqual(TEXT("Valid seeking never recreates topology"), F.TopologyChanges, 0);
	TestFalse(TEXT("Visualizer has no tick clock"), F.Actor->PrimaryActorTick.bCanEverTick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTSequenceBoundaries,
	"RTCQT.D13.Sequence.SubframesBoundariesAndPauseRestart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTSequenceBoundaries::RunTest(const FString& Parameters)
{
	using namespace RTCQTSequenceTests;
	FSequenceFixture F;
	if (!TestNotNull(TEXT("Native player initialized"), F.Player)) return false;
	for (float Time : {0.0f, 1.0125f, 3599.0f / 60.0f, 60.0f})
	{
		F.Seek(Time);
		CheckFrame(*this, F, Time);
	}
	F.Seek(16.75f, EUpdatePositionMethod::Play);
	F.Player->Pause();
	CheckFrame(*this, F, 16.75);
	const auto PausedData = F.Instances->PerInstanceSMCustomData;
	F.Player->Stop();
	TestTrue(TEXT("Keep State retains the last evaluated visual frame at stop"), PausedData == F.Instances->PerInstanceSMCustomData);
	F.Seek(0.0f);
	F.Player->Play();
	F.Player->Pause();
	CheckFrame(*this, F, 0.0);
	F.Seek(16.75f, EUpdatePositionMethod::Scrub);
	CheckFrame(*this, F, 16.75);
	TestTrue(TEXT("Pause/restart/reseek restores identical inputs"), PausedData == F.Instances->PerInstanceSMCustomData);
	TestEqual(TEXT("Boundary checks preserve topology"), F.TopologyChanges, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTSequenceRecovery,
	"RTCQT.D13.Sequence.FailuresAndRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTSequenceRecovery::RunTest(const FString& Parameters)
{
	using namespace RTCQTSequenceTests;
	FSequenceFixture F;
	if (!TestNotNull(TEXT("Native player initialized"), F.Player)) return false;
	F.Seek(16.75f);
	const auto GoodData = F.Instances->PerInstanceSMCustomData;
	AddExpectedError(TEXT("Assign Constant QNRT before evaluating song time"), EAutomationExpectedErrorFlags::Contains, 1);
	F.SetAsset(TEXT("ConstantQNRT"), nullptr);
	F.Seek(2.0f);
	TestFalse(TEXT("Sequence exposes failed evaluation"), F.Actor->LastSequenceEvaluation.bSucceeded);
	TestEqual(TEXT("Failure identifies missing analyzer"), F.Actor->LastSequenceEvaluation.FailureReason, ERTCQTFailureReason::MissingAnalyzer);
	TestEqual(TEXT("Failure clears stale rendered geometry"), F.Instances->GetInstanceCount(), 0);
	F.SetAsset(TEXT("ConstantQNRT"), F.Analyzer);
	F.Seek(16.75f);
	CheckFrame(*this, F, 16.75);
	TestTrue(TEXT("Sequence recovers the previous frame exactly"), GoodData == F.Instances->PerInstanceSMCustomData);
	AddExpectedError(TEXT("Song time must be finite"), EAutomationExpectedErrorFlags::Contains, 1);
	F.Actor->SetSequenceTimeSeconds(std::numeric_limits<float>::quiet_NaN());
	TestEqual(TEXT("Nonfinite setter input has the existing failure semantics"),
		F.Actor->LastSequenceEvaluation.FailureReason, ERTCQTFailureReason::InvalidTime);
	TestEqual(TEXT("Invalid time clears stale geometry"), F.Instances->GetInstanceCount(), 0);
	F.Seek(11.75f);
	CheckFrame(*this, F, 11.75);
	return true;
}

#endif
