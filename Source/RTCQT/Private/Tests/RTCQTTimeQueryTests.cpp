#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "RTCQTAnalyzerQuery.h"
#include "RTCQTResponseSettings.h"
#include "RTCQTVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/Csv/CsvParser.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace RTCQTTimeTests
{
struct FAnalyzerFixture
{
	UConstantQNRT* Analyzer = NewObject<UConstantQNRT>();
	USoundWave* Sound = NewObject<USoundWave>(Analyzer);

	FAnalyzerFixture()
	{
		Analyzer->Sound = Sound;
		Analyzer->Settings->NoiseFloorDb = -60.0f;
		auto Data = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
		const TArray<float> First = {-60.0f, -40.0f, -20.0f, 0.0f};
		const TArray<float> Last = {0.0f, -20.0f, -40.0f, -60.0f};
		Data->AddFrame(0, 0.0f, First);
		Data->AddFrame(0, 2.0f, Last);
		Data->SetDurationInSeconds(2.0f);
		Data->SortChronologically();
		Analyzer->SetResult(Data);
	}
};

struct FTimeVisualizerFixture
{
	FAnalyzerFixture Analysis;
	UWorld* World;
	ARTCQTVisualizer* Actor;
	UInstancedStaticMeshComponent* Instances;
	UStaticMesh* Cube;
	URTCQTResponseSettings* Settings;

	FTimeVisualizerFixture()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
			.CreateAISystem(false).ShouldSimulatePhysics(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
			ERHIFeatureLevel::Num, &Values);
		FActorSpawnParameters Spawn;
		Spawn.bDeferConstruction = true;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<ARTCQTVisualizer>(ARTCQTVisualizer::StaticClass(), FTransform::Identity, Spawn);
		Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		Settings = NewObject<URTCQTResponseSettings>(Actor);
		SetAsset(TEXT("ConstantQNRT"), Analysis.Analyzer);
		SetAsset(TEXT("ResponseSettings"), Settings);
		SetAsset(TEXT("InstanceMesh"), Cube);
		SetAsset(TEXT("InstanceMaterial"), Cube->GetMaterial(0));
		SetInt(TEXT("ExpectedBandCount"), 4);
		Actor->FinishSpawning(FTransform::Identity);
		Instances = Actor->FindComponentByClass<UInstancedStaticMeshComponent>();
	}

	~FTimeVisualizerFixture() { World->DestroyWorld(false); }

	void SetAsset(const TCHAR* Name, UObject* Value)
	{
		FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name)->SetObjectPropertyValue_InContainer(Actor, Value);
	}

	void SetInt(const TCHAR* Name, int32 Value)
	{
		FindFProperty<FIntProperty>(Actor->GetClass(), Name)->SetPropertyValue_InContainer(Actor, Value);
	}
};

void CheckBands(FAutomationTestBase& Test, const FString& Label, TConstArrayView<float> Actual, TConstArrayView<float> Expected)
{
	if (!Test.TestEqual(Label + TEXT(" count"), Actual.Num(), Expected.Num())) return;
	for (int32 Index = 0; Index < Actual.Num(); ++Index)
	{
		Test.TestTrue(FString::Printf(TEXT("%s band %d"), *Label, Index),
			FMath::IsFinite(Actual[Index]) && FMath::Abs(Actual[Index] - Expected[Index]) <= 1.e-6f);
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTExactTimeQueries,
	"RTCQT.D10.Query.ExactTimeAndOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTExactTimeQueries::RunTest(const FString& Parameters)
{
	using namespace RTCQTTimeTests;
	FAnalyzerFixture F;
	const TArray<float> Times = {0.0f, 0.5f, 1.0f, 1.75f, 2.0f};
	const TArray<TArray<float>> Expected = {{0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f},
		{0.25f, 5.0f / 12.0f, 7.0f / 12.0f, 0.75f}, {0.5f, 0.5f, 0.5f, 0.5f},
		{0.875f, 0.625f, 0.375f, 0.125f}, {1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 0.0f}};
	TArray<TArray<float>> Baselines;
	for (int32 Index = 0; Index < Times.Num(); ++Index)
	{
		TArray<float> Bands = {99.0f};
		const auto Result = RTCQTAnalysis::QueryBandsAtTime(F.Analyzer, Times[Index], 0, 4, Bands);
		TestTrue(TEXT("Exact-time query succeeds"), Result.bSucceeded);
		TestTrue(TEXT("Success has no failure reason"), Result.FailureReason == ERTCQTFailureReason::None);
		TestEqual(TEXT("Requested time preserved exactly"), Result.EvaluatedTimeSeconds, Times[Index]);
		TestEqual(TEXT("Observed count recorded"), Result.ObservedBandCount, 4);
		CheckBands(*this, FString::Printf(TEXT("Time %.2f"), Times[Index]), Bands, Expected[Index]);
		Baselines.Add(Bands);
	}
	for (int32 Index : {4, 0, 3, 1, 2, 1})
	{
		TArray<float> Bands;
		const auto Result = RTCQTAnalysis::QueryBandsAtTime(F.Analyzer, Times[Index], 0, 4, Bands);
		TestTrue(TEXT("Backward/forward/repeated queries succeed"), Result.bSucceeded);
		TestTrue(TEXT("Query order cannot alter any float"), Bands == Baselines[Index]);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTQueryGuards,
	"RTCQT.D10.Query.InvalidInputs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTQueryGuards::RunTest(const FString& Parameters)
{
	using namespace RTCQTTimeTests;
	FAnalyzerFixture F;
	const auto ExpectFailure = [this](const UConstantQNRT* Analyzer, float Time, int32 Channel, int32 Count, ERTCQTFailureReason Reason)
	{
		TArray<float> Bands = {10.0f, 20.0f};
		const auto Result = RTCQTAnalysis::QueryBandsAtTime(Analyzer, Time, Channel, Count, Bands);
		TestFalse(TEXT("Invalid query fails"), Result.bSucceeded);
		TestTrue(TEXT("Invalid query has the expected typed reason"), Result.FailureReason == Reason);
		TestTrue(TEXT("Invalid query clears previous output"), Bands.IsEmpty());
		return Result;
	};
	ExpectFailure(nullptr, 1.0f, 0, 4, ERTCQTFailureReason::MissingAnalyzer);
	F.Analyzer->Sound = nullptr;
	ExpectFailure(F.Analyzer, 1.0f, 0, 4, ERTCQTFailureReason::MissingAnalyzedSound);
	F.Analyzer->Sound = F.Sound;
	for (float Time : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()})
	{
		ExpectFailure(F.Analyzer, Time, 0, 4, ERTCQTFailureReason::InvalidTime);
	}
	for (float Time : {-0.01f, 2.01f, std::numeric_limits<float>::max()})
	{
		ExpectFailure(F.Analyzer, Time, 0, 4, ERTCQTFailureReason::TimeOutsideAnalysis);
	}
	for (int32 Count : {0, -1}) ExpectFailure(F.Analyzer, 1.0f, 0, Count, ERTCQTFailureReason::InvalidExpectedBandCount);
	for (int32 Count : {3, 5})
	{
		const auto Result = ExpectFailure(F.Analyzer, 1.0f, 0, Count, ERTCQTFailureReason::BandCountMismatch);
		TestEqual(TEXT("Mismatch still reports actual count"), Result.ObservedBandCount, 4);
	}
	for (int32 Channel : {-1, 1, MAX_int32}) ExpectFailure(F.Analyzer, 1.0f, Channel, 4, ERTCQTFailureReason::EmptyAnalyzerResult);
	UConstantQNRTSettings* SavedSettings = F.Analyzer->Settings;
	F.Analyzer->Settings = nullptr;
	ExpectFailure(F.Analyzer, 1.0f, 0, 4, ERTCQTFailureReason::EmptyAnalyzerResult);
	F.Analyzer->Settings = SavedSettings;
	for (float Duration : {-1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
	{
		F.Analyzer->DurationInSeconds = Duration;
		ExpectFailure(F.Analyzer, 1.0f, 0, 4, ERTCQTFailureReason::TimeOutsideAnalysis);
	}
	F.Analyzer->DurationInSeconds = 2.0f;
	UConstantQNRT* Empty = NewObject<UConstantQNRT>();
	Empty->Sound = F.Sound;
	Empty->DurationInSeconds = 2.0f;
	ExpectFailure(Empty, 1.0f, 0, 4, ERTCQTFailureReason::EmptyAnalyzerResult);
	auto Unsorted = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
	const TArray<float> Frame = {-60.0f, -40.0f, -20.0f, 0.0f};
	Unsorted->AddFrame(0, 1.0f, Frame);
	Unsorted->AddFrame(0, 0.0f, Frame);
	Unsorted->SetDurationInSeconds(2.0f);
	Empty->SetResult(Unsorted);
	ExpectFailure(Empty, 1.0f, 0, 4, ERTCQTFailureReason::EmptyAnalyzerResult);
	TArray<float> Bands;
	TestTrue(TEXT("A valid query recovers after invalid calls"), RTCQTAnalysis::QueryBandsAtTime(F.Analyzer, 0.5f, 0, 4, Bands).bSucceeded);
	CheckBands(*this, TEXT("Recovery values"), Bands, TArray<float>{0.25f, 5.0f / 12.0f, 7.0f / 12.0f, 0.75f});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTTimeEvaluation,
	"RTCQT.D10.Visualizer.FailuresAndRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTTimeEvaluation::RunTest(const FString& Parameters)
{
	using namespace RTCQTTimeTests;
	FTimeVisualizerFixture F;
	const auto Recover = [this, &F]()
	{
		const auto Result = F.Actor->EvaluateAtTime(0.5f);
		TestTrue(TEXT("Valid evaluation succeeds"), Result.bSucceeded);
		TestTrue(TEXT("Recovery clears failure reason"), Result.FailureReason == ERTCQTFailureReason::None);
		TestEqual(TEXT("Observed bands match"), Result.ObservedBandCount, 4);
		TestEqual(TEXT("Valid call restores instances"), F.Instances->GetInstanceCount(), 4);
	};
	const auto ExpectFailure = [this, &F](ERTCQTFailureReason Reason, const TCHAR* Message, float Time = 0.5f)
	{
		AddExpectedMessagePlain(Message, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		for (int32 Repeat = 0; Repeat < 2; ++Repeat)
		{
			const auto Result = F.Actor->EvaluateAtTime(Time);
			TestFalse(TEXT("Invalid evaluation fails"), Result.bSucceeded);
			TestTrue(TEXT("Evaluation reports the expected failure"), Result.FailureReason == Reason);
			TestEqual(TEXT("Invalid evaluation clears stale instances"), F.Instances->GetInstanceCount(), 0);
			if (Reason == ERTCQTFailureReason::BandCountMismatch) TestEqual(TEXT("Mismatch preserves observed count"), Result.ObservedBandCount, 4);
		}
	};
	Recover();
	F.SetAsset(TEXT("ConstantQNRT"), nullptr);
	ExpectFailure(ERTCQTFailureReason::MissingAnalyzer, TEXT("Assign Constant QNRT"));
	F.SetAsset(TEXT("ConstantQNRT"), F.Analysis.Analyzer);
	Recover();
	F.Analysis.Analyzer->Sound = nullptr;
	ExpectFailure(ERTCQTFailureReason::MissingAnalyzedSound, TEXT("has no analyzed Sound"));
	F.Analysis.Analyzer->Sound = F.Analysis.Sound;
	Recover();
	F.SetAsset(TEXT("ResponseSettings"), nullptr);
	ExpectFailure(ERTCQTFailureReason::MissingResponseSettings, TEXT("Assign Response Settings"));
	F.SetAsset(TEXT("ResponseSettings"), F.Settings);
	Recover();
	F.SetAsset(TEXT("InstanceMesh"), nullptr);
	ExpectFailure(ERTCQTFailureReason::MissingMesh, TEXT("Assign Instance Mesh"));
	F.SetAsset(TEXT("InstanceMesh"), F.Cube);
	Recover();
	F.SetInt(TEXT("ExpectedBandCount"), 0);
	ExpectFailure(ERTCQTFailureReason::InvalidExpectedBandCount, TEXT("Set Expected Band Count above zero"));
	F.SetInt(TEXT("ExpectedBandCount"), 4);
	Recover();
	F.SetInt(TEXT("ExpectedBandCount"), 3);
	ExpectFailure(ERTCQTFailureReason::BandCountMismatch, TEXT("returned 4 bands; Expected Band Count is 3"));
	F.SetInt(TEXT("ExpectedBandCount"), 4);
	Recover();
	F.SetInt(TEXT("AnalyzerChannel"), 1);
	ExpectFailure(ERTCQTFailureReason::EmptyAnalyzerResult, TEXT("no queryable data for channel 1"));
	F.SetInt(TEXT("AnalyzerChannel"), 0);
	Recover();
	ExpectFailure(ERTCQTFailureReason::InvalidTime, TEXT("Song time must be finite"), std::numeric_limits<float>::quiet_NaN());
	Recover();
	ExpectFailure(ERTCQTFailureReason::TimeOutsideAnalysis, TEXT("outside a valid analysis duration"), -1.0f);
	Recover();
	FRTCQTLayoutSettings& Layout = *FindFProperty<FStructProperty>(F.Actor->GetClass(), TEXT("LayoutSettings"))
		->ContainerPtrToValuePtr<FRTCQTLayoutSettings>(F.Actor);
	Layout.Mode = ERTCQTLayoutMode::Circle;
	ExpectFailure(ERTCQTFailureReason::LayoutGenerationFailed, TEXT("Select Line layout"));
	Layout.Mode = ERTCQTLayoutMode::Line;
	Recover();
	F.SetAsset(TEXT("InstanceComponent"), nullptr);
	ExpectFailure(ERTCQTFailureReason::InstanceUpdateFailed, TEXT("RTCQTInstances component is missing"));
	F.SetAsset(TEXT("InstanceComponent"), F.Instances);
	Recover();
	F.SetAsset(TEXT("InstanceMaterial"), nullptr);
	AddExpectedMessagePlain(TEXT("Instance Material is unset"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	Recover();
	for (float Time : {2.0f, 0.0f, 1.0f, 0.5f})
	{
		TestTrue(TEXT("Reordered evaluations succeed"), F.Actor->EvaluateAtTime(Time).bSucceeded);
		TestTrue(TEXT("Actor retains the same instance component"), F.Actor->FindComponentByClass<UInstancedStaticMeshComponent>() == F.Instances);
		TestEqual(TEXT("Evaluation preserves instance count"), F.Instances->GetInstanceCount(), 4);
	}
	TestFalse(TEXT("Actor has no time-advancing tick"), F.Actor->PrimaryActorTick.bCanEverTick);
	return true;
}

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FRTCQTReferenceTimeQueries,
	"RTCQT.D10.LocalFixture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FRTCQTReferenceTimeQueries::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	// The portable synthetic tests always run. This licensed-fixture test is
	// discoverable only on workstations where the ignored private asset exists.
	if (FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("Audio/PrivateTest/CQNRT_NoTimeToExplain_32.uasset")))
	{
		OutBeautifiedNames.Add(TEXT("D07ReferenceValues"));
		OutTestCommands.Add(TEXT("D07"));
	}
}

bool FRTCQTReferenceTimeQueries::RunTest(const FString& Parameters)
{
	using namespace RTCQTTimeTests;
	UConstantQNRT* Analyzer = LoadObject<UConstantQNRT>(nullptr, TEXT("/Game/Audio/PrivateTest/CQNRT_NoTimeToExplain_32.CQNRT_NoTimeToExplain_32"));
	if (!TestNotNull(TEXT("Load private D07 analyzer"), Analyzer)) return false;
	FString Csv;
	if (!TestTrue(TEXT("Read D07 reference values"), FFileHelper::LoadFileToString(Csv,
		*(FPaths::ProjectDir() / TEXT("planning/evidence/d07/baseline.csv"))))) return false;
	const FCsvParser Parser(MoveTemp(Csv));
	const auto& Rows = Parser.GetRows();
	if (!TestEqual(TEXT("Reference has 192 rows and a header"), Rows.Num(), 193)) return false;
	if (!TestEqual(TEXT("Reference header has four columns"), Rows[0].Num(), 4)) return false;
	const TCHAR* Columns[] = {TEXT("time_seconds"), TEXT("band_index"), TEXT("raw_db"), TEXT("normalized")};
	for (int32 Column = 0; Column < 4; ++Column)
	{
		if (!TestEqual(TEXT("Reference column order"), FString(Rows[0][Column]), FString(Columns[Column]))) return false;
	}
	const TArray<float> ReferenceTimes = {0.75f, 2.0f, 11.75f, 16.75f, 33.25f, 44.25f};
	TMap<float, TArray<float>> Expected;
	TSet<FString> Keys;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const auto& Row = Rows[RowIndex];
		if (!TestEqual(TEXT("Reference row width"), Row.Num(), 4)) return false;
		const float Time = FCString::Atof(Row[0]);
		const int32 Band = FCString::Atoi(Row[1]);
		const float Value = FCString::Atof(Row[3]);
		const FString Key = FString::Printf(TEXT("%.2f:%d"), Time, Band);
		if (!TestTrue(TEXT("Reference key and value are valid"), ReferenceTimes.Contains(Time)
			&& Band >= 0 && Band < 32 && FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f && !Keys.Contains(Key))) return false;
		Keys.Add(Key);
		TArray<float>& Bands = Expected.FindOrAdd(Time);
		Bands.SetNum(32);
		Bands[Band] = Value;
	}
	FTimeVisualizerFixture F;
	F.SetAsset(TEXT("ConstantQNRT"), Analyzer);
	F.SetInt(TEXT("ExpectedBandCount"), 32);
	const TArray<float> QueryOrder = {0.75f, 2.0f, 11.75f, 16.75f, 33.25f, 44.25f,
		44.25f, 33.25f, 2.0f, 0.75f, 16.75f, 11.75f, 16.75f, 0.75f, 16.75f};
	TMap<float, TArray<float>> Baselines;
	FString Capture = TEXT("query_index,time_seconds,band_index,normalized\n");
	float MaxDifference = 0.0f;
	for (int32 QueryIndex = 0; QueryIndex < QueryOrder.Num(); ++QueryIndex)
	{
		const float Time = QueryOrder[QueryIndex];
		TArray<float> Bands;
		const auto Result = RTCQTAnalysis::QueryBandsAtTime(Analyzer, Time, 0, 32, Bands);
		if (!TestTrue(TEXT("D07 exact-time query succeeds"), Result.bSucceeded)) return false;
		CheckBands(*this, FString::Printf(TEXT("D07 time %.2f"), Time), Bands, Expected[Time]);
		if (const TArray<float>* Earlier = Baselines.Find(Time))
		{
			TestTrue(TEXT("D07 reordered/repeated queries reproduce every float"), Bands == *Earlier);
		}
		else Baselines.Add(Time, Bands);
		for (int32 Band = 0; Band < Bands.Num(); ++Band)
		{
			MaxDifference = FMath::Max(MaxDifference, FMath::Abs(Bands[Band] - Expected[Time][Band]));
			Capture += FString::Printf(TEXT("%d,%.2f,%d,%.9g\n"), QueryIndex, Time, Band, Bands[Band]);
		}
		const auto ActorResult = F.Actor->EvaluateAtTime(Time);
		TestTrue(TEXT("Actor query with real asset succeeds"), ActorResult.bSucceeded);
		TestTrue(TEXT("Real-asset success reports no failure"), ActorResult.FailureReason == ERTCQTFailureReason::None);
		TestEqual(TEXT("Actor reports exact requested time"), ActorResult.EvaluatedTimeSeconds, Time);
		TestEqual(TEXT("Actor reports 32 real bands"), ActorResult.ObservedBandCount, 32);
		TestEqual(TEXT("Actor retains 32 instances"), F.Instances->GetInstanceCount(), 32);
	}
	for (float Time : {0.0f, Analyzer->DurationInSeconds})
	{
		TestTrue(TEXT("Real-asset endpoints are accepted"), F.Actor->EvaluateAtTime(Time).bSucceeded);
	}
	const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/D10");
	IFileManager::Get().MakeDirectory(*Directory, true);
	TestTrue(TEXT("Write exact-time captures"), FFileHelper::SaveStringToFile(Capture, *(Directory / TEXT("captured-values.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
	AddInfo(FString::Printf(TEXT("Verified 192 D07 reference values across %d queries; maximum absolute difference %.9g. Analysis duration %.9g seconds."),
		QueryOrder.Num(), MaxDifference, Analyzer->DurationInSeconds));
	return true;
}

#endif
