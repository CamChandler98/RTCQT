#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "RTCQTAnalyzerQuery.h"
#include "RTCQTResponseSettings.h"
#include "RTCQTVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InstancedStaticMeshDelegates.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/Csv/CsvParser.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace RTCQTScaleTests
{
struct FScaleFixture
{
	UConstantQNRT* Analyzer = NewObject<UConstantQNRT>();
	UWorld* World;
	ARTCQTVisualizer* Actor;
	UInstancedStaticMeshComponent* Instances;
	URTCQTResponseSettings* Settings;

	FScaleFixture()
	{
		Analyzer->Sound = NewObject<USoundWave>(Analyzer);
		Analyzer->Settings->NoiseFloorDb = -60.0f;
		SetFrames({-60.0f, -40.0f, -20.0f, 0.0f}, {0.0f, -20.0f, -40.0f, -60.0f});
		const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
			.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
			ERHIFeatureLevel::Num, &Values);
		FActorSpawnParameters Spawn;
		Spawn.bDeferConstruction = true;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<ARTCQTVisualizer>(ARTCQTVisualizer::StaticClass(), FTransform::Identity, Spawn);
		Settings = NewObject<URTCQTResponseSettings>(Actor);
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		SetAsset(TEXT("ConstantQNRT"), Analyzer);
		SetAsset(TEXT("ResponseSettings"), Settings);
		SetAsset(TEXT("InstanceMesh"), Cube);
		SetAsset(TEXT("InstanceMaterial"), Cube->GetMaterial(0));
		SetCount(4);
		Actor->FinishSpawning(FTransform::Identity);
		Instances = Actor->FindComponentByClass<UInstancedStaticMeshComponent>();
	}

	~FScaleFixture() { World->DestroyWorld(false); }

	void SetFrames(const TArray<float>& First, const TArray<float>& Last)
	{
		auto Data = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
		Data->AddFrame(0, 0.0f, First);
		Data->AddFrame(0, 2.0f, Last);
		Data->SetDurationInSeconds(2.0f);
		Data->SortChronologically();
		Analyzer->SetResult(Data);
	}

	void SetAsset(const TCHAR* Name, UObject* Value)
	{
		FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name)->SetObjectPropertyValue_InContainer(Actor, Value);
	}

	void SetCount(int32 Count)
	{
		FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("ExpectedBandCount"))->SetPropertyValue_InContainer(Actor, Count);
	}

	FRTCQTLayoutSettings& Layout()
	{
		return *FindFProperty<FStructProperty>(Actor->GetClass(), TEXT("LayoutSettings"))
			->ContainerPtrToValuePtr<FRTCQTLayoutSettings>(Actor);
	}

	TArray<FMatrix> Matrices() const
	{
		TArray<FMatrix> Result;
		for (const auto& Instance : Instances->PerInstanceSMData) Result.Add(Instance.Transform);
		return Result;
	}
};

struct FTopologyObserver
{
	int32 Events = 0;
	FDelegateHandle Handle;

	explicit FTopologyObserver(UInstancedStaticMeshComponent* Watched)
	{
		Handle = FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.AddLambda(
			[this, Watched](UInstancedStaticMeshComponent* Component,
				TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Updates)
			{
				if (Component == Watched) Events += Updates.Num();
			});
	}

	~FTopologyObserver() { FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.Remove(Handle); }
};

void Close(FAutomationTestBase& Test, const TCHAR* Label, double Actual, double Expected)
{
	Test.TestTrue(Label, FMath::IsFinite(Actual) && FMath::Abs(Actual - Expected) <= 1.e-6);
}

bool SameMatrices(const TArray<FMatrix>& A, const TArray<FMatrix>& B)
{
	if (A.Num() != B.Num()) return false;
	for (int32 Index = 0; Index < A.Num(); ++Index)
	{
		if (A[Index] != B[Index]) return false;
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTKnownScaleValues,
	"RTCQT.D11.Scale.KnownValuesAndLocalTransforms", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTKnownScaleValues::RunTest(const FString& Parameters)
{
	using namespace RTCQTScaleTests;
	FScaleFixture F;
	F.Layout().Origin = FVector(10, 20, 30);
	F.Layout().Spacing = 120;
	F.Layout().BaseScale = FVector(0.5, 1.25, 7);
	const FTransform ActorTransform(FRotator(20, 45, -15), FVector(100, 200, 300), FVector(2, 3, 4));
	F.Actor->SetActorTransform(ActorTransform);
	TestTrue(TEXT("Default-settings evaluation succeeds"), F.Actor->EvaluateAtTime(0.0f).bSucceeded);
	// Native normalized inputs [0, 1/3, 2/3, 1] give these independent D02 results.
	const double Responses[] = {0, 49.0 / 729.0, 289.0 / 729.0, 1};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FTransform Local, World;
		TestTrue(TEXT("Read applied local transform"), F.Instances->GetInstanceTransform(Index, Local, false));
		F.Instances->GetInstanceTransform(Index, World, true);
		TestTrue(TEXT("Base translation preserved"), Local.GetTranslation().Equals(FVector(10 + (Index - 1.5) * 120, 20, 30)));
		TestTrue(TEXT("Base rotation preserved"), Local.GetRotation().Equals(FQuat::Identity));
		Close(*this, TEXT("Base X scale preserved"), Local.GetScale3D().X, 0.5);
		Close(*this, TEXT("Base Y scale preserved"), Local.GetScale3D().Y, 1.25);
		Close(*this, TEXT("Z uses absolute range, not base-Z multiplication"), Local.GetScale3D().Z, 0.1 + 3.9 * Responses[Index]);
		Close(*this, TEXT("Response custom data matches D02"), F.Instances->PerInstanceSMCustomData[Index * 2], Responses[Index]);
		Close(*this, TEXT("Band ordering preserved"), F.Instances->PerInstanceSMCustomData[Index * 2 + 1], Index / 3.0);
		TestTrue(TEXT("Actor translation and rotation apply once"), World.GetTranslation().Equals(ActorTransform.TransformPosition(Local.GetTranslation()))
			&& World.GetRotation().Equals(ActorTransform.GetRotation()));
	}
	TestTrue(TEXT("Midpoint frame succeeds"), F.Actor->EvaluateAtTime(1.0f).bSucceeded);
	for (const auto& Instance : F.Instances->PerInstanceSMData)
	{
		Close(*this, TEXT("Known midpoint response maps to height"), Instance.Transform.M[2][2], 0.1 + 3.9 * 16.0 / 81.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTScaleSeeking,
	"RTCQT.D11.Scale.SeekingAndSettingsEdits", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTScaleSeeking::RunTest(const FString& Parameters)
{
	using namespace RTCQTScaleTests;
	FScaleFixture F;
	FTopologyObserver Topology(F.Instances);
	TMap<float, TArray<FMatrix>> Baselines;
	for (float Time : {0.0f, 0.5f, 1.0f, 2.0f, 0.75f, 0.0f, 2.0f, 0.5f, 0.0f})
	{
		TestTrue(TEXT("Seek evaluation succeeds"), F.Actor->EvaluateAtTime(Time).bSucceeded);
		const auto Matrices = F.Matrices();
		if (const auto* Earlier = Baselines.Find(Time))
		{
			TestTrue(TEXT("Repeated seeks reproduce every stored matrix element exactly"), SameMatrices(Matrices, *Earlier));
		}
		else Baselines.Add(Time, Matrices);
	}
	F.Settings->ResponseExponent = 1;
	TestTrue(TEXT("Exponent edit evaluates without recreating the actor"), F.Actor->EvaluateAtTime(0).bSucceeded);
	Close(*this, TEXT("Linear exponent refreshes response"), F.Instances->PerInstanceSMCustomData[2], 7.0 / 27.0);
	TestFalse(TEXT("Exponent edit changes transforms"), SameMatrices(F.Matrices(), Baselines[0]));
	F.Settings->ResponseExponent = 2;
	F.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("Restored exponent restores exact transforms"), SameMatrices(F.Matrices(), Baselines[0]));
	F.Settings->BandWeights = {0, 2, 0.5f};
	F.Actor->EvaluateAtTime(0);
	Close(*this, TEXT("Gain before exponentiation"), F.Instances->PerInstanceSMCustomData[2], 196.0 / 729.0);
	Close(*this, TEXT("Reduced band weight"), F.Instances->PerInstanceSMCustomData[4], 289.0 / 2916.0);
	Close(*this, TEXT("Missing weight defaults to one"), F.Instances->PerInstanceSMCustomData[6], 1);
	F.Settings->BandWeights.Reset();
	F.Settings->MeshZScaleRange = {0.5f, 2};
	F.Actor->EvaluateAtTime(0);
	Close(*this, TEXT("Edited minimum refreshes absolute Z"), F.Instances->PerInstanceSMData[0].Transform.M[2][2], 0.5);
	Close(*this, TEXT("Edited maximum refreshes absolute Z"), F.Instances->PerInstanceSMData[3].Transform.M[2][2], 2);
	F.Settings->MeshZScaleRange = {0.1f, 4};
	F.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("Restored settings recover exact original matrices"), SameMatrices(F.Matrices(), Baselines[0]));
	URTCQTResponseSettings* Replacement = NewObject<URTCQTResponseSettings>(F.Actor);
	Replacement->ResponseExponent = 1;
	F.SetAsset(TEXT("ResponseSettings"), Replacement);
	F.Actor->EvaluateAtTime(0);
	Close(*this, TEXT("Replacement settings asset is read on the next evaluation"), F.Instances->PerInstanceSMCustomData[2], 7.0 / 27.0);
	F.SetAsset(TEXT("ResponseSettings"), F.Settings);
	F.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("Replacing the original asset restores exact transforms"), SameMatrices(F.Matrices(), Baselines[0]));
	TestEqual(TEXT("Seeks and response settings edits add/remove/clear zero instances"), Topology.Events, 0);
	TestEqual(TEXT("Fixed component retains four instances"), F.Instances->GetInstanceCount(), 4);
	TestTrue(TEXT("Same component remains owned by the actor"), F.Actor->FindComponentByClass<UInstancedStaticMeshComponent>() == F.Instances);
	TestFalse(TEXT("No playback clock or tick"), F.Actor->PrimaryActorTick.bCanEverTick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTScaleBounds,
	"RTCQT.D11.Scale.BoundsAndFailureRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTScaleBounds::RunTest(const FString& Parameters)
{
	using namespace RTCQTScaleTests;
	FScaleFixture F;
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const float Infinity = std::numeric_limits<float>::infinity();
	const TArray<FRTCQTScalarRange> Ranges = {{4, 0.1f}, {2, 2}, {NaN, Infinity}, {-2, -0.5f}};
	const double Minima[] = {0.1, 2, 0, -2};
	const double Maxima[] = {4, 2, 1, -0.5};
	for (int32 Range = 0; Range < Ranges.Num(); ++Range)
	{
		F.Settings->MeshZScaleRange = Ranges[Range];
		TestTrue(TEXT("Sanitized/reversed/constant/signed range evaluates"), F.Actor->EvaluateAtTime(0).bSucceeded);
		for (const auto& Instance : F.Instances->PerInstanceSMData)
		{
			const double Z = Instance.Transform.M[2][2];
			TestTrue(TEXT("Stored matrix is finite and within the sanitized range"), !Instance.Transform.ContainsNaN()
				&& Z >= Minima[Range] - 1.e-6 && Z <= Maxima[Range] + 1.e-6);
		}
		Close(*this, TEXT("Silence reaches the lower bound"), F.Instances->PerInstanceSMData[0].Transform.M[2][2], Minima[Range]);
		Close(*this, TEXT("Peak reaches the upper bound"), F.Instances->PerInstanceSMData[3].Transform.M[2][2], Maxima[Range]);
	}
	F.Settings->MeshZScaleRange = {0.1f, 4};
	F.Settings->NoiseFloor = 1;
	F.Settings->InputCeiling = 0.5f;
	F.Actor->EvaluateAtTime(0);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Close(*this, TEXT("Invalid input window gives zero response"), F.Instances->PerInstanceSMCustomData[Index * 2], 0);
		Close(*this, TEXT("Invalid input window gives minimum height"), F.Instances->PerInstanceSMData[Index].Transform.M[2][2], 0.1);
	}
	F.Settings->NoiseFloor = 0.1f;
	F.Settings->InputCeiling = 1;
	F.Settings->ResponseExponent = NaN;
	F.Actor->EvaluateAtTime(0);
	Close(*this, TEXT("Invalid exponent uses the neutral response"), F.Instances->PerInstanceSMCustomData[2], 7.0 / 27.0);
	F.Settings->ResponseExponent = 2;
	F.Actor->EvaluateAtTime(0);
	const auto Baseline = F.Matrices();
	AddExpectedMessagePlain(TEXT("Assign Response Settings"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	F.SetAsset(TEXT("ResponseSettings"), nullptr);
	for (int32 Repeat = 0; Repeat < 2; ++Repeat)
	{
		const auto Result = F.Actor->EvaluateAtTime(0);
		TestFalse(TEXT("Missing settings fails"), Result.bSucceeded);
		TestTrue(TEXT("Typed failure is preserved"), Result.FailureReason == ERTCQTFailureReason::MissingResponseSettings);
		TestEqual(TEXT("No old heights remain visible"), F.Instances->GetInstanceCount(), 0);
	}
	F.SetAsset(TEXT("ResponseSettings"), F.Settings);
	TestTrue(TEXT("Settings restoration recovers"), F.Actor->EvaluateAtTime(0).bSucceeded);
	TestTrue(TEXT("Recovery restores exact scaled matrices"), SameMatrices(F.Matrices(), Baseline));
	AddExpectedMessagePlain(TEXT("Expected Band Count is 3"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	F.SetCount(3);
	const auto Mismatch = F.Actor->EvaluateAtTime(0);
	TestFalse(TEXT("Count mismatch fails after a rendered frame"), Mismatch.bSucceeded);
	TestEqual(TEXT("Actual count remains available for diagnosis"), Mismatch.ObservedBandCount, 4);
	TestEqual(TEXT("Count mismatch clears old transforms"), F.Instances->GetInstanceCount(), 0);
	F.SetCount(4);
	TestTrue(TEXT("Restored count recovers"), F.Actor->EvaluateAtTime(0).bSucceeded);
	TestTrue(TEXT("Count recovery reproduces the scaled frame"), SameMatrices(F.Matrices(), Baseline));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTSingleBandScale,
	"RTCQT.D11.Scale.SingleBandAndLayoutRefresh", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTSingleBandScale::RunTest(const FString& Parameters)
{
	using namespace RTCQTScaleTests;
	FScaleFixture F;
	F.SetFrames({-60}, {0});
	F.SetCount(1);
	F.Layout().Origin = FVector(2, 3, 4);
	F.Layout().BaseScale = FVector(2, 3, 9);
	TestTrue(TEXT("Single interpolated band evaluates"), F.Actor->EvaluateAtTime(1).bSucceeded);
	TestEqual(TEXT("One value produces one instance"), F.Instances->GetInstanceCount(), 1);
	FTransform Local;
	F.Instances->GetInstanceTransform(0, Local, false);
	TestTrue(TEXT("Single instance stays at the base origin"), Local.GetTranslation().Equals(F.Layout().Origin));
	Close(*this, TEXT("Single X scale"), Local.GetScale3D().X, 2);
	Close(*this, TEXT("Single Y scale"), Local.GetScale3D().Y, 3);
	Close(*this, TEXT("Single absolute Z"), Local.GetScale3D().Z, 0.1 + 3.9 * 16.0 / 81.0);
	TestEqual(TEXT("Single normalized band index is zero"), F.Instances->PerInstanceSMCustomData[1], 0.0f);
	F.SetFrames({-60, -40, -20, 0}, {0, -20, -40, -60});
	F.SetCount(4);
	F.Layout().Spacing = 25;
	FTopologyObserver Topology(F.Instances);
	TestTrue(TEXT("Changed count/layout evaluates"), F.Actor->EvaluateAtTime(0).bSucceeded);
	TestEqual(TEXT("Changed count yields exactly four instances"), F.Instances->GetInstanceCount(), 4);
	TestTrue(TEXT("Actual layout change rebuilds topology"), Topology.Events > 0);
	F.Instances->GetInstanceTransform(0, Local, false);
	TestTrue(TEXT("New layout origin/spacing take effect"), Local.GetTranslation().Equals(FVector(-35.5, 3, 4)));
	const int32 Events = Topology.Events;
	TestTrue(TEXT("Unchanged rebuild succeeds"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Unchanged rebuild preserves scaled response"), F.Instances->PerInstanceSMCustomData[6], 1.0f);
	F.Actor->EvaluateAtTime(2);
	TestEqual(TEXT("Next seek keeps the repaired topology"), Topology.Events, Events);
	F.Actor->ClearVisualization();
	TestEqual(TEXT("Explicit clear removes scaled instances"), F.Instances->GetInstanceCount(), 0);
	TestTrue(TEXT("Evaluation after clear recovers"), F.Actor->EvaluateAtTime(0).bSucceeded);
	Close(*this, TEXT("Recovered peak height"), F.Instances->PerInstanceSMData[3].Transform.M[2][2], 4);
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FRTCQTRealScaleReferences,
	"RTCQT.D11.LocalFixture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FRTCQTRealScaleReferences::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	if (FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("Audio/PrivateTest/CQNRT_NoTimeToExplain_32.uasset")))
	{
		OutBeautifiedNames.Add(TEXT("D07ScaleReferences"));
		OutTestCommands.Add(TEXT("D07"));
	}
}

bool FRTCQTRealScaleReferences::RunTest(const FString& Parameters)
{
	using namespace RTCQTScaleTests;
	UConstantQNRT* Analyzer = LoadObject<UConstantQNRT>(nullptr, TEXT("/Game/Audio/PrivateTest/CQNRT_NoTimeToExplain_32.CQNRT_NoTimeToExplain_32"));
	if (!TestNotNull(TEXT("Load private 32-band analyzer"), Analyzer)) return false;
	FString Csv;
	if (!TestTrue(TEXT("Read independent D07 reference"), FFileHelper::LoadFileToString(Csv,
		*(FPaths::ProjectDir() / TEXT("planning/evidence/d07/baseline.csv"))))) return false;
	const FCsvParser Parser(MoveTemp(Csv));
	const auto& Rows = Parser.GetRows();
	if (!TestEqual(TEXT("D07 has 192 reference rows"), Rows.Num(), 193)) return false;
	TMap<float, TArray<float>> ExpectedBands;
	for (int32 Index = 1; Index < Rows.Num(); ++Index)
	{
		if (!TestEqual(TEXT("D07 row has four columns"), Rows[Index].Num(), 4)) return false;
		const int32 Band = FCString::Atoi(Rows[Index][1]);
		if (!TestTrue(TEXT("D07 band index in range"), Band >= 0 && Band < 32)) return false;
		auto& Values = ExpectedBands.FindOrAdd(FCString::Atof(Rows[Index][0]));
		Values.SetNum(32);
		Values[Band] = FCString::Atof(Rows[Index][3]);
	}
	FScaleFixture F;
	F.SetAsset(TEXT("ConstantQNRT"), Analyzer);
	F.SetCount(32);
	F.Layout().BaseScale = FVector(0.5);
	if (!TestTrue(TEXT("Build 32-band static topology"), F.Actor->RebuildInstances())) return false;
	FTopologyObserver Topology(F.Instances);
	const TArray<float> Order = {16.75f, 0.75f, 44.25f, 16.75f, 2.0f, 11.75f, 33.25f, 16.75f};
	TMap<float, TArray<FMatrix>> Matrices;
	FString Capture = TEXT("query_index,time_seconds,band_index,analyzer_normalized,response,local_x,local_y,local_z,scale_x,scale_y,scale_z,normalized_band_index\n");
	for (int32 Query = 0; Query < Order.Num(); ++Query)
	{
		const float Time = Order[Query];
		TArray<float> Bands;
		if (!TestTrue(TEXT("Real exact-time bands available"), RTCQTAnalysis::QueryBandsAtTime(Analyzer, Time, 0, 32, Bands).bSucceeded)) return false;
		const auto Result = F.Actor->EvaluateAtTime(Time);
		if (!TestTrue(TEXT("Real response evaluation succeeds"), Result.bSucceeded)) return false;
		TestEqual(TEXT("Actor reports exact time"), Result.EvaluatedTimeSeconds, Time);
		TestEqual(TEXT("Actor reports 32 queried values"), Result.ObservedBandCount, 32);
		TestEqual(TEXT("One instance per real band"), F.Instances->GetInstanceCount(), 32);
		if (!TestTrue(TEXT("D07 timestamp is present"), ExpectedBands.Contains(Time))) return false;
		for (int32 Band = 0; Band < 32; ++Band)
		{
			Close(*this, TEXT("Analyzer input matches D07 reference"), Bands[Band], ExpectedBands[Time][Band]);
			FTransform Local;
			F.Instances->GetInstanceTransform(Band, Local, false);
			const float Response = F.Instances->PerInstanceSMCustomData[Band * 2];
			const float BandIndex = F.Instances->PerInstanceSMCustomData[Band * 2 + 1];
			TestTrue(TEXT("Finite bounded applied Z"), Local.IsValid() && Local.GetScale3D().Z >= 0.1f && Local.GetScale3D().Z <= 4.0f);
			Close(*this, TEXT("Real base X preserved"), Local.GetScale3D().X, 0.5);
			Close(*this, TEXT("Real base Y preserved"), Local.GetScale3D().Y, 0.5);
			Capture += FString::Printf(TEXT("%d,%.2f,%d,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g\n"),
				Query, Time, Band, Bands[Band], Response, Local.GetTranslation().X, Local.GetTranslation().Y, Local.GetTranslation().Z,
				Local.GetScale3D().X, Local.GetScale3D().Y, Local.GetScale3D().Z, BandIndex);
		}
		if (const auto* Earlier = Matrices.Find(Time))
		{
			TestTrue(TEXT("Real repeated timestamp reproduces exact stored matrices"), SameMatrices(F.Matrices(), *Earlier));
		}
		else Matrices.Add(Time, F.Matrices());
	}
	TestEqual(TEXT("Real seeks never recreate instances"), Topology.Events, 0);
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/D11");
	IFileManager::Get().MakeDirectory(*Directory, true);
	TestTrue(TEXT("Write applied transform reference capture"), FFileHelper::SaveStringToFile(Capture,
		*(Directory / TEXT("captured-transforms.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	AddInfo(TEXT("Verified 192 D07 reference bands and 256 applied scale rows across 8 queries; repeated matrices match exactly and topology changes are zero."));
	return true;
}

#endif
