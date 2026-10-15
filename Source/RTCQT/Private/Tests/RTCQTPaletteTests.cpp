#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "RTCQTMaterialParameters.h"
#include "RTCQTMusicLabManager.h"
#include "RTCQTPaletteComponent.h"
#include "RTCQTPalettePresets.h"
#include "RTCQTResponseSettings.h"
#include "RTCQTVisualizer.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "InstancedStaticMeshDelegates.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace RTCQTPaletteTests
{
const FLinearColor AuthoredLow(0.12f, 0.23f, 0.34f, 1.f);
const FLinearColor AuthoredHigh(0.78f, 0.67f, 0.56f, 1.f);

UMaterial* MakeMaterial(UObject* Outer, bool bIncludeHigh = true)
{
	auto* Material = NewObject<UMaterial>(Outer, NAME_None, RF_Transient);
	const auto Vector = [Material](FName Name, FLinearColor Color)
	{
		auto* Expression = NewObject<UMaterialExpressionVectorParameter>(Material);
		Expression->ParameterName = Name;
		Expression->DefaultValue = Color;
		Expression->ExpressionGUID = FGuid::NewGuid();
		Material->GetExpressionCollection().AddExpression(Expression);
	};
	Vector(RTCQTMaterialParameters::LowBandColor, AuthoredLow);
	if (bIncludeHigh) Vector(RTCQTMaterialParameters::HighBandColor, AuthoredHigh);
	Vector(TEXT("UnrelatedColor"), FLinearColor::Green);
	for (const auto Name : {RTCQTMaterialParameters::EmissiveMin, RTCQTMaterialParameters::EmissiveMax})
	{
		auto* Expression = NewObject<UMaterialExpressionScalarParameter>(Material);
		Expression->ParameterName = Name;
		Expression->ExpressionGUID = FGuid::NewGuid();
		Material->GetExpressionCollection().AddExpression(Expression);
	}
	Material->UpdateCachedExpressionData();
	return Material;
}

struct FFixture
{
	UWorld* World;
	ARTCQTVisualizer* Actor;
	ARTCQTMusicLabManager* Manager;
	UInstancedStaticMeshComponent* Instances;
	UMaterial* Parent;
	UConstantQNRT* Analyzer;
	URTCQTPaletteComponent* Palette;

	FFixture()
	{
		const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
			.CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
		World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
		FActorSpawnParameters Spawn;
		Spawn.bDeferConstruction = true;
		Spawn.ObjectFlags |= RF_Transient;
		Actor = World->SpawnActor<ARTCQTVisualizer>(ARTCQTVisualizer::StaticClass(), FTransform::Identity, Spawn);
		Parent = MakeMaterial(World);
		auto* Settings = NewObject<URTCQTResponseSettings>(Actor);
		Settings->NoiseFloor = 0;
		Settings->ResponseExponent = 1;
		Settings->EmissiveRange = {2.f, 8.f};
		Analyzer = MakeAnalyzer(3);
		SetAsset(TEXT("ConstantQNRT"), Analyzer);
		SetAsset(TEXT("ResponseSettings"), Settings);
		SetAsset(TEXT("InstanceMesh"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		SetAsset(TEXT("InstanceMaterial"), Parent);
		FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("ExpectedBandCount"))->SetPropertyValue_InContainer(Actor, 3);
		Actor->FinishSpawning(FTransform::Identity);
		Instances = Actor->FindComponentByClass<UInstancedStaticMeshComponent>();
		Manager = World->SpawnActor<ARTCQTMusicLabManager>(ARTCQTMusicLabManager::StaticClass(), FTransform::Identity, Spawn);
		Manager->bAutoPlay = false;
		Manager->Visualizer = Actor;
		Manager->FinishSpawning(FTransform::Identity);
		Palette = Manager->Palette;
	}

	~FFixture() { World->DestroyWorld(false); }

	UConstantQNRT* MakeAnalyzer(int32 Count)
	{
		auto* Result = NewObject<UConstantQNRT>(Actor);
		Result->Sound = NewObject<USoundWave>(Result);
		Result->Settings->NumBands = Count;
		Result->Settings->NoiseFloorDb = -60;
		auto Data = MakeShared<Audio::FConstantQNRTResult, ESPMode::ThreadSafe>();
		TArray<float> First, Last;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			First.Add(-60.f + 60.f * Index / FMath::Max(1, Count - 1));
			Last.Add(-60.f * Index / FMath::Max(1, Count - 1));
		}
		Data->AddFrame(0, 0.f, First);
		Data->AddFrame(0, 2.f, Last);
		Data->SetDurationInSeconds(2);
		Data->SortChronologically();
		Result->SetResult(Data);
		return Result;
	}

	void SetAsset(const TCHAR* Name, UObject* Value)
	{
		FindFProperty<FObjectPropertyBase>(Actor->GetClass(), Name)->SetObjectPropertyValue_InContainer(Actor, Value);
	}
	UMaterialInstanceDynamic* Material() const { return Cast<UMaterialInstanceDynamic>(Instances->GetMaterial(0)); }
};

void Colors(FAutomationTestBase& Test, ARTCQTVisualizer* Actor, const FLinearColor& Low, const FLinearColor& High)
{
	FLinearColor ActualLow, ActualHigh;
	Test.TestTrue(TEXT("Displayed palette is readable"), Actor->GetDisplayedPaletteColors(ActualLow, ActualHigh));
	Test.TestTrue(TEXT("Displayed low color matches"), ActualLow.Equals(Low, 1.e-6f));
	Test.TestTrue(TEXT("Displayed high color matches"), ActualHigh.Equals(High, 1.e-6f));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPalettePresetsTest, "RTCQT.Palette.PresetsAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPalettePresetsTest::RunTest(const FString& Parameters)
{
	auto* Asset = NewObject<URTCQTPalettePresets>();
	FString Error;
	TestTrue(TEXT("Starter presets are valid"), Asset->ValidatePresets(Error));
	TestEqual(TEXT("Four starter presets"), Asset->Presets.Num(), 4);
	const TArray<FName> Ids = {TEXT("neon"), TEXT("sunset"), TEXT("ice"), TEXT("ember")};
	for (int32 Index = 0; Index < Ids.Num(); ++Index)
		TestEqual(TEXT("Stable cycle order"), Asset->Presets[Index].Id, Ids[Index]);
	TestTrue(TEXT("Neon preserves D12 low color"), Asset->Presets[0].LowColor == FLinearColor(0.01f, 0.65f, 1.f, 1.f));
	const auto Original = Asset->Presets;
	Asset->Presets[1].Id = TEXT("NEON");
	TestFalse(TEXT("Duplicate IDs including case variants rejected"), Asset->ValidatePresets(Error));
	Asset->Presets = Original;
	Asset->Presets[0].Id = NAME_None;
	TestFalse(TEXT("Empty ID rejected"), Asset->ValidatePresets(Error));
	Asset->Presets = Original;
	Asset->Presets[0].DisplayName = FText::GetEmpty();
	TestFalse(TEXT("Empty display name rejected"), Asset->ValidatePresets(Error));
	for (const float Invalid : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
		for (int32 Channel = 0; Channel < 4; ++Channel)
			for (const bool bHigh : {false, true})
			{
				Asset->Presets = Original;
				auto& Color = bHigh ? Asset->Presets[0].HighColor : Asset->Presets[0].LowColor;
				Color.Component(Channel) = Invalid;
				TestFalse(TEXT("Non-finite color channel rejected"), Asset->ValidatePresets(Error));
			}
	Asset->Presets.Reset();
	TestFalse(TEXT("Empty catalog rejected"), Asset->ValidatePresets(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPaletteRuntimeTest, "RTCQT.Palette.RuntimeColorsAndIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPaletteRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace RTCQTPaletteTests;
	FFixture F, Other;
	TestTrue(TEXT("Music evaluation succeeds"), F.Actor->EvaluateAtTime(0).bSucceeded);
	auto* Material = F.Material();
	if (!TestNotNull(TEXT("Shared material exists"), Material)) return false;
	const auto Transforms = F.Instances->PerInstanceSMData;
	const auto Responses = F.Instances->PerInstanceSMCustomData;
	F.Actor->SetActorScale3D(FVector(1.15));
	const auto ActorTransform = F.Actor->GetActorTransform();
	int32 TopologyEvents = 0;
	const auto Handle = FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.AddLambda(
		[&F, &TopologyEvents](UInstancedStaticMeshComponent* Component,
			TArrayView<const FInstancedStaticMeshDelegates::FInstanceIndexUpdateData> Updates)
		{ if (Component == F.Instances) TopologyEvents += Updates.Num(); });
	const FLinearColor Low(0.1f, 0.8f, 0.9f, 1.f), High(0.7f, 0.3f, 0.1f, 1.f);
	TestTrue(TEXT("Runtime colors accepted"), F.Actor->SetRuntimePaletteColors(Low, High));
	Colors(*this, F.Actor, Low, High);
	Colors(*this, Other.Actor, AuthoredLow, AuthoredHigh);
	TestTrue(TEXT("Material stays actor-owned and shared"), F.Material() == Material && Material->GetOuter() == F.Actor);
	TestTrue(TEXT("Runtime override recorded"), F.Actor->HasRuntimePaletteOverride());
	TestTrue(TEXT("Palette does not alter pulse actor scale"), F.Actor->GetActorTransform().Equals(ActorTransform));
	for (int32 Index = 0; Index < Transforms.Num(); ++Index)
		TestTrue(TEXT("Palette preserves exact instance transform"), F.Instances->PerInstanceSMData[Index].Transform == Transforms[Index].Transform);
	TestTrue(TEXT("Palette preserves analyzed response data"), F.Instances->PerInstanceSMCustomData == Responses);
	TestEqual(TEXT("Palette does not rebuild topology"), TopologyEvents, 0);
	TestEqual(TEXT("Emissive minimum preserved"), Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMin), 2.f);
	TestEqual(TEXT("Emissive maximum preserved"), Material->K2_GetScalarParameterValue(RTCQTMaterialParameters::EmissiveMax), 8.f);
	for (const float Invalid : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
		for (int32 Channel = 0; Channel < 4; ++Channel)
		{
			FLinearColor Bad = Low;
			Bad.Component(Channel) = Invalid;
			TestFalse(TEXT("Invalid low color rejected"), F.Actor->SetRuntimePaletteColors(Bad, High));
			TestFalse(TEXT("Invalid high color rejected"), F.Actor->SetRuntimePaletteColors(Low, Bad));
			Colors(*this, F.Actor, Low, High);
		}
	Material->SetVectorParameterValue(TEXT("UnrelatedColor"), FLinearColor::Yellow);
	F.Actor->ClearRuntimePaletteColors();
	Colors(*this, F.Actor, AuthoredLow, AuthoredHigh);
	TestFalse(TEXT("Reset clears runtime selection"), F.Actor->HasRuntimePaletteOverride());
	TestTrue(TEXT("Reset reuses existing MID"), F.Material() == Material);
	TestTrue(TEXT("Reset preserves unrelated parameters"), Material->K2_GetVectorParameterValue(TEXT("UnrelatedColor")) == FLinearColor::Yellow);
	TestEqual(TEXT("Reset does not rebuild topology"), TopologyEvents, 0);
	TestFalse(TEXT("Visualizer still has no time-advancing tick"), F.Actor->PrimaryActorTick.bCanEverTick);
	FInstancedStaticMeshDelegates::OnInstanceIndexUpdated.Remove(Handle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPalettePersistenceTest, "RTCQT.Palette.RefreshPlaybackAndMaterialReplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPalettePersistenceTest::RunTest(const FString& Parameters)
{
	using namespace RTCQTPaletteTests;
	FFixture F;
	const FLinearColor Low(0.8f, 0.2f, 0.1f, 1.f), High(0.3f, 0.4f, 0.9f, 1.f);
	TestTrue(TEXT("Set palette before evaluation"), F.Actor->SetRuntimePaletteColors(Low, High));
	for (const float Time : {0.f, 2.f, 1.f, 0.f})
	{
		TestTrue(TEXT("Normal and backward evaluations succeed"), F.Actor->EvaluateAtTime(Time).bSucceeded);
		Colors(*this, F.Actor, Low, High);
	}
	F.Actor->ClearVisualization();
	TestEqual(TEXT("Geometry is cleared"), F.Instances->GetInstanceCount(), 0);
	TestTrue(TEXT("Geometry can rebuild"), F.Actor->RebuildInstances());
	Colors(*this, F.Actor, Low, High);
	for (const int32 Count : {16, 64, 32})
	{
		F.Actor->SetPlaybackAnalyzer(F.MakeAnalyzer(Count));
		TestTrue(TEXT("New playback analyzer evaluates"), F.Actor->EvaluateAtTime(0).bSucceeded);
		TestEqual(TEXT("New band count applies"), F.Instances->GetInstanceCount(), Count);
		Colors(*this, F.Actor, Low, High);
	}
	auto* PreviousMaterial = F.Material();
	auto* Replacement = NewObject<UMaterialInstanceConstant>(F.World, NAME_None, RF_Transient);
	Replacement->SetParentEditorOnly(F.Parent);
	const FLinearColor NewDefault(0.9f, 0.8f, 0.7f, 1.f);
	Replacement->SetVectorParameterValueEditorOnly(RTCQTMaterialParameters::LowBandColor, NewDefault);
	F.SetAsset(TEXT("InstanceMaterial"), Replacement);
	TestTrue(TEXT("Material replacement rebuild succeeds"), F.Actor->RebuildInstances());
	TestTrue(TEXT("Replacement creates a new MID"), F.Material() != PreviousMaterial);
	Colors(*this, F.Actor, Low, High);
	F.Actor->ClearRuntimePaletteColors();
	Colors(*this, F.Actor, NewDefault, AuthoredHigh);
	Replacement->SetVectorParameterValueEditorOnly(RTCQTMaterialParameters::LowBandColor, FLinearColor::Red);
	F.Actor->RebuildInstances();
	Colors(*this, F.Actor, FLinearColor::Red, AuthoredHigh);
	TestTrue(TEXT("Palette can be selected again"), F.Actor->SetRuntimePaletteColors(Low, High));
	auto* Incompatible = MakeMaterial(F.World, false);
	F.SetAsset(TEXT("InstanceMaterial"), Incompatible);
	TestFalse(TEXT("Material missing one parameter rejected before mutation"), F.Actor->SetRuntimePaletteColors(High, Low));
	Colors(*this, F.Actor, Low, High);
	F.Actor->RebuildInstances();
	FLinearColor OutLow = FLinearColor::Yellow, OutHigh = FLinearColor::Green;
	TestFalse(TEXT("Incompatible displayed material is reported"), F.Actor->GetDisplayedPaletteColors(OutLow, OutHigh));
	TestTrue(TEXT("Failed read preserves both outputs"), OutLow == FLinearColor::Yellow && OutHigh == FLinearColor::Green);
	F.SetAsset(TEXT("InstanceMaterial"), Replacement);
	F.Actor->RebuildInstances();
	Colors(*this, F.Actor, Low, High);
	F.SetAsset(TEXT("InstanceMaterial"), nullptr);
	TestFalse(TEXT("Missing explicit material cannot accept palette"), F.Actor->SetRuntimePaletteColors(High, Low));
	F.SetAsset(TEXT("InstanceMaterial"), Replacement);
	F.Actor->RebuildInstances();
	Colors(*this, F.Actor, Low, High);
	auto* Duplicate = DuplicateObject<ARTCQTVisualizer>(F.Actor, F.World->PersistentLevel);
	TestFalse(TEXT("Duplicated actor starts without a runtime palette"), Duplicate->HasRuntimePaletteOverride());
	TestTrue(TEXT("Duplicated actor rebuilds"), Duplicate->RebuildInstances());
	Colors(*this, Duplicate, FLinearColor::Red, AuthoredHigh);
	Colors(*this, F.Actor, Low, High);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPaletteTransitionsTest, "RTCQT.Palette.TransitionsCycleAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPaletteTransitionsTest::RunTest(const FString& Parameters)
{
	using namespace RTCQTPaletteTests;
	FFixture F;
	auto* Palette = F.Palette;
	const auto* Defaults = GetDefault<URTCQTPalettePresets>();
	TestFalse(TEXT("Palette component is opt-in"), Palette->bPaletteEnabled);
	TestFalse(TEXT("Idle palette does not tick"), Palette->IsComponentTickEnabled());
	TestFalse(TEXT("Disabled selection rejected"), Palette->SelectPalette(TEXT("ice")));
	Palette->SetPaletteEnabled(true);
	TestTrue(TEXT("Valid setup enables palettes"), Palette->bPaletteEnabled);
	F.Manager->PlaybackState = ERTCQTMusicPlaybackState::Paused;
	TestTrue(TEXT("Palette can start while music is paused"), Palette->SelectPaletteAt(TEXT("ice"), 1000.0));
	TestTrue(TEXT("Active transition ticks"), Palette->IsComponentTickEnabled());
	Colors(*this, F.Actor, AuthoredLow, AuthoredHigh);
	TestTrue(TEXT("Early transition advances"), Palette->AdvanceBlend(1000.125));
	const auto& Ice = Defaults->Presets[2];
	Colors(*this, F.Actor, FMath::LerpStable(AuthoredLow, Ice.LowColor, 0.15625f),
		FMath::LerpStable(AuthoredHigh, Ice.HighColor, 0.15625f));
	TestTrue(TEXT("Midpoint advances"), Palette->AdvanceBlend(1000.25));
	const FLinearColor MidLow = FMath::LerpStable(AuthoredLow, Ice.LowColor, 0.5f);
	const FLinearColor MidHigh = FMath::LerpStable(AuthoredHigh, Ice.HighColor, 0.5f);
	Colors(*this, F.Actor, MidLow, MidHigh);
	TestTrue(TEXT("Selecting same target succeeds without restarting"), Palette->SelectPaletteAt(TEXT("ice"), 1000.25));
	TestEqual(TEXT("No-op preserves blend start"), Palette->BlendStartedAt, 1000.0);
	TestFalse(TEXT("Unknown name rejected"), Palette->SelectPaletteAt(TEXT("unknown"), 1000.3));
	TestEqual(TEXT("Invalid request preserves selection"), Palette->SelectedPaletteId, FName(TEXT("ice")));
	TestEqual(TEXT("Invalid request preserves blend clock"), Palette->BlendStartedAt, 1000.0);
	Colors(*this, F.Actor, MidLow, MidHigh);
	TestTrue(TEXT("Mid-transition override accepted"), Palette->SelectPaletteAt(TEXT("ember"), 1000.25));
	Colors(*this, F.Actor, MidLow, MidHigh);
	TestTrue(TEXT("Replacement transition completes in half a second"), Palette->AdvanceBlend(1000.75));
	const auto& Ember = Defaults->Presets[3];
	Colors(*this, F.Actor, Ember.LowColor, Ember.HighColor);
	TestFalse(TEXT("Completed transition stops ticking"), Palette->IsComponentTickEnabled());
	TestFalse(TEXT("Completed transition is neutral"), Palette->bBlending);
	FLinearColor ExactLow, ExactHigh;
	F.Actor->GetDisplayedPaletteColors(ExactLow, ExactHigh);
	TestTrue(TEXT("Endpoint is exact"), ExactLow == Ember.LowColor && ExactHigh == Ember.HighColor);
	TestTrue(TEXT("Reset begins"), Palette->ResetPaletteAt(1001.0));
	TestTrue(TEXT("Long frame finishes reset"), Palette->AdvanceBlend(1010.0));
	Colors(*this, F.Actor, AuthoredLow, AuthoredHigh);
	TestFalse(TEXT("Reset clears override"), F.Actor->HasRuntimePaletteOverride());
	TestTrue(TEXT("Reset clears selection"), Palette->SelectedPaletteId.IsNone());
	for (const auto& Preset : Defaults->Presets)
	{
		TestTrue(TEXT("Next palette accepted"), Palette->NextPalette());
		TestEqual(TEXT("Next follows preset order"), Palette->SelectedPaletteId, Preset.Id);
	}
	TestTrue(TEXT("Next wraps"), Palette->NextPalette());
	TestEqual(TEXT("Wrapped selection is Neon"), Palette->SelectedPaletteId, FName(TEXT("neon")));
	const double PreviousStart = Palette->BlendStartedAt;
	auto* InvalidAsset = NewObject<URTCQTPalettePresets>(F.Manager);
	InvalidAsset->Presets.Reset();
	Palette->PresetAsset = InvalidAsset;
	TestFalse(TEXT("Invalid asset rejected"), Palette->SelectPalette(TEXT("ice")));
	TestEqual(TEXT("Invalid asset preserves blend clock"), Palette->BlendStartedAt, PreviousStart);
	TestEqual(TEXT("Invalid asset preserves selection"), Palette->SelectedPaletteId, FName(TEXT("neon")));
	Palette->SetPaletteEnabled(false);
	TestFalse(TEXT("Disable stops ticks"), Palette->IsComponentTickEnabled());
	TestFalse(TEXT("Disable clears runtime override"), F.Actor->HasRuntimePaletteOverride());
	Colors(*this, F.Actor, AuthoredLow, AuthoredHigh);
	TestFalse(TEXT("Blend did not resume audio"), F.Manager->PlaybackState == ERTCQTMusicPlaybackState::Playing);
	TestNull(TEXT("Blend did not start an audio source"), F.Manager->AudioComponent->Sound.Get());
	return true;
}

#endif
