#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "RTCQTMaterialParameters.h"
#include "RTCQTResponseSettings.h"
#include "RTCQTVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InstancedStaticMeshDelegates.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace RTCQTMaterialTests
{
struct FFixture
{
	UWorld* World;
	ARTCQTVisualizer* Actor;
	UInstancedStaticMeshComponent* Instances;
	URTCQTResponseSettings* Settings;
	UMaterialInterface* Parent;

	FFixture()
	{
		const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
			.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
		FActorSpawnParameters Spawn;
		Spawn.bDeferConstruction = true;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<ARTCQTVisualizer>(ARTCQTVisualizer::StaticClass(), FTransform::Identity, Spawn);
		auto* Analyzer = NewObject<UConstantQNRT>(Actor);
		Analyzer->Sound = NewObject<USoundWave>(Analyzer);
		Analyzer->Settings->NoiseFloorDb = -60;
		auto Data = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
		Data->AddFrame(0, 0.0f, {-60.0f, -30.0f, 0.0f});
		Data->AddFrame(0, 2.0f, {0.0f, -30.0f, -60.0f});
		Data->SetDurationInSeconds(2);
		Data->SortChronologically();
		Analyzer->SetResult(Data);
		Settings = NewObject<URTCQTResponseSettings>(Actor);
		Settings->NoiseFloor = 0;
		Settings->ResponseExponent = 1;
		auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		Parent = Cube->GetMaterial(0);
		SetAsset(TEXT("ConstantQNRT"), Analyzer);
		SetAsset(TEXT("ResponseSettings"), Settings);
		SetAsset(TEXT("InstanceMesh"), Cube);
		SetAsset(TEXT("InstanceMaterial"), Parent);
		FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("ExpectedBandCount"))->SetPropertyValue_InContainer(Actor, 3);
		Actor->FinishSpawning(FTransform::Identity);
		Instances = Actor->FindComponentByClass<UInstancedStaticMeshComponent>();
	}

	~FFixture() { World->DestroyWorld(false); }
	void SetAsset(const TCHAR* Name, UObject* Value)
	{
		FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name)->SetObjectPropertyValue_InContainer(Actor, Value);
	}
	UMaterialInstanceDynamic* Material() const { return Cast<UMaterialInstanceDynamic>(Instances->GetMaterial(0)); }
};

void Close(FAutomationTestBase& Test, const TCHAR* Label, double Actual, double Expected)
{
	Test.TestTrue(Label, FMath::IsFinite(Actual) && FMath::Abs(Actual - Expected) < 1.e-6);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTMaterialSettings,
	"RTCQT.D12.Material.SharedSettingsAndBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTMaterialSettings::RunTest(const FString& Parameters)
{
	using namespace RTCQTMaterialTests;
	FFixture F;
	TestTrue(TEXT("Evaluation succeeds"), F.Actor->EvaluateAtTime(0).bSucceeded);
	auto* Material = F.Material();
	if (!TestNotNull(TEXT("Shared dynamic material assigned"), Material)) return false;
	TestTrue(TEXT("Material belongs to this actor"), Material->GetOuter() == F.Actor);
	TestTrue(TEXT("Assigned parent preserved"), Material->Parent == F.Parent);
	TestTrue(TEXT("Material does not serialize into the level"), Material->HasAnyFlags(RF_Transient));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float Response = F.Instances->PerInstanceSMCustomData[Index * 2];
		Close(*this, TEXT("Independent silence/midpoint/peak response"), Response, Index * 0.5);
		Close(*this, TEXT("Material endpoints share the height response"),
			FMath::Lerp(Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMin),
				Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), Response), Index * 5.0);
		Close(*this, TEXT("Height remains correlated"), F.Instances->PerInstanceSMData[Index].Transform.M[2][2], 0.1 + 3.9 * Response);
		Close(*this, TEXT("Band order preserved"), F.Instances->PerInstanceSMCustomData[Index * 2 + 1], Index * 0.5);
	}
	const auto OriginalTransforms = F.Instances->PerInstanceSMData;
	struct FRangeCase { float A, B, Min, Max; };
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const float Infinity = std::numeric_limits<float>::infinity();
	const FRangeCase Cases[] = {{8, 2, 2, 8}, {-4, 7, 0, 7}, {4, 4, 4, 4},
		{NaN, 5, 0, 5}, {Infinity, -Infinity, 0, 0}, {1, std::numeric_limits<float>::max(), 1, 100}};
	for (const auto& Case : Cases)
	{
		F.Settings->EmissiveRange = {Case.A, Case.B};
		TestTrue(TEXT("Range edit evaluates"), F.Actor->EvaluateAtTime(0).bSucceeded);
		TestTrue(TEXT("Settings edit reuses material"), F.Material() == Material);
		Close(*this, TEXT("Sanitized lower bound"), Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMin), Case.Min);
		Close(*this, TEXT("Sanitized upper bound"), Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), Case.Max);
		for (int32 Index = 0; Index < 3; ++Index)
			TestTrue(TEXT("Emissive edit preserves exact stored transform"), F.Instances->PerInstanceSMData[Index].Transform == OriginalTransforms[Index].Transform);
	}
	auto* Replacement = NewObject<URTCQTResponseSettings>(F.Actor);
	Replacement->EmissiveRange = {3, 9};
	F.SetAsset(TEXT("ResponseSettings"), Replacement);
	TestTrue(TEXT("Replacement settings evaluate"), F.Actor->EvaluateAtTime(0).bSucceeded);
	Close(*this, TEXT("Replacement lower bound read"), Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMin), 3);
	Close(*this, TEXT("Replacement upper bound read"), Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), 9);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTMaterialSeeking,
	"RTCQT.D12.Material.SeekingReplacementAndFallback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTMaterialSeeking::RunTest(const FString& Parameters)
{
	using namespace RTCQTMaterialTests;
	FFixture F;
	F.Actor->EvaluateAtTime(0);
	auto* Material = F.Material();
	const auto Baseline = F.Instances->PerInstanceSMCustomData;
	int32 TopologyEvents = 0;
	const auto Handle = FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.AddLambda(
		[&F, &TopologyEvents](UInstancedStaticMeshComponent* Component,
			TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Updates)
		{ if (Component == F.Instances) TopologyEvents += Updates.Num(); });
	for (float Time : {2.0f, 1.0f, 0.0f, 2.0f, 0.0f})
	{
		TestTrue(TEXT("Reordered query succeeds"), F.Actor->EvaluateAtTime(Time).bSucceeded);
		TestTrue(TEXT("Every query reuses one material"), F.Material() == Material);
	}
	TestTrue(TEXT("Return to the same time restores exact custom data"), F.Instances->PerInstanceSMCustomData == Baseline);
	auto* ReplacementParent = NewObject<UMaterialInstanceConstant>(F.Actor);
	ReplacementParent->SetParentEditorOnly(F.Parent);
	TestTrue(TEXT("Test replacement differs from original"), ReplacementParent != F.Parent);
	F.SetAsset(TEXT("InstanceMaterial"), ReplacementParent);
	TestTrue(TEXT("Parent replacement succeeds"), F.Actor->EvaluateAtTime(0).bSucceeded);
	TestTrue(TEXT("Replacement creates appropriate shared material"), F.Material() && F.Material() != Material && F.Material()->Parent == ReplacementParent);
	auto* Replacement = F.Material();
	F.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("New parent material reused"), F.Material() == Replacement);
	AddExpectedMessagePlain(TEXT("Instance Material is unset"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	F.SetAsset(TEXT("InstanceMaterial"), nullptr);
	TestTrue(TEXT("Missing optional material still evaluates"), F.Actor->EvaluateAtTime(0).bSucceeded);
	F.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("Mesh default fallback restored"), F.Instances->GetMaterial(0) == F.Parent);
	TestNull(TEXT("Fallback has no shared override"), F.Material());
	F.SetAsset(TEXT("InstanceMaterial"), F.Parent);
	F.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("Restored override evaluates with the original parent"), F.Material() && F.Material()->Parent == F.Parent);
	TestTrue(TEXT("Material operations preserve exact custom data"), F.Instances->PerInstanceSMCustomData == Baseline);
	TestEqual(TEXT("No instance add/remove/clear operations"), TopologyEvents, 0);
	FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTMaterialOwnership,
	"RTCQT.D12.Material.ActorOwnership", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTMaterialOwnership::RunTest(const FString& Parameters)
{
	using namespace RTCQTMaterialTests;
	FFixture A, B;
	A.Settings->EmissiveRange = {0, 4};
	B.Settings->EmissiveRange = {2, 8};
	A.Actor->EvaluateAtTime(0);
	B.Actor->EvaluateAtTime(0);
	TestTrue(TEXT("Different actors own distinct shared materials"), A.Material() && B.Material() && A.Material() != B.Material());
	Close(*this, TEXT("First actor upper bound"), A.Material()->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), 4);
	Close(*this, TEXT("Second actor upper bound"), B.Material()->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), 8);
	B.Settings->EmissiveRange = {1, 6};
	B.Actor->EvaluateAtTime(2);
	Close(*this, TEXT("Settings do not leak between actors"), A.Material()->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), 4);
	return true;
}

#endif
