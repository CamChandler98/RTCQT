#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "RTCQTMaterialParameters.h"
#include "RTCQTMusicLabManager.h"
#include "RTCQTMusicLabControls.h"
#include "RTCQTPaletteComponent.h"
#include "RTCQTPalettePresets.h"
#include "RTCQTResponseSettings.h"
#include "RTCQTStreamerBotComponent.h"
#include "RTCQTVisualizer.h"
#include "Components/AudioComponent.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"
#include "Engine/Level.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "InstancedStaticMeshDelegates.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
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
	bool bOwnsWorldContext = false;

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

	~FFixture()
	{
		if (World)
		{
			World->DestroyWorld(false);
			if (bOwnsWorldContext) GEngine->DestroyWorldContext(World);
		}
	}
	void AddWorldContext()
	{
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		bOwnsWorldContext = true;
	}

	UConstantQNRT* MakeAnalyzer(int32 Count)
	{
		auto* Result = NewObject<UConstantQNRT>(Actor);
		Result->Sound = NewObject<USoundWave>(Result);
		Result->Sound->Duration = 2.f;
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

void CaptureControls(FAutomationTestBase& Test, const TSharedRef<SWidget>& Widget, const TCHAR* Name)
{
	if (!FParse::Param(FCommandLine::Get(), TEXT("RTCQTPaletteCaptureUI"))) return;
	if (!Test.TestTrue(TEXT("Requested UI capture has rendering enabled"), FApp::CanEverRender())) return;
	FWidgetRenderer Renderer(true);
	auto* Target = Renderer.DrawWidget(Widget, FVector2D(640, 720));
	if (!Test.TestNotNull(TEXT("Widget render target exists"), Target)) return;
	FlushRenderingCommands();
	FImage Image;
	if (!Test.TestTrue(TEXT("Rendered widget pixels are readable"), FImageUtils::GetRenderTargetImage(Target, Image))) return;
	const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/PaletteStep3/captures"));
	IFileManager::Get().MakeDirectory(*Directory, true);
	const FString Path = FPaths::Combine(Directory, FString(Name) + TEXT(".png"));
	Test.TestTrue(TEXT("Widget screenshot saved"), FImageUtils::SaveImageByExtension(*Path, Image));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPaletteViewerControlsTest, "RTCQT.Palette.ViewerGateCooldownAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPaletteViewerControlsTest::RunTest(const FString& Parameters)
{
	using namespace RTCQTPaletteTests;
	FFixture F;
	auto* Palette = F.Palette;
	auto* Receiver = F.Manager->StreamerBot.Get();
	const auto* Defaults = GetDefault<URTCQTPalettePresets>();
	Receiver->ConnectionStatus = TEXT("Ready");
	F.Instances->SetMaterial(0, nullptr);
	FLinearColor UnreadyLow, UnreadyHigh;
	TestFalse(TEXT("Fixture has no displayed palette before visualizer initialization"),
		F.Actor->GetDisplayedPaletteColors(UnreadyLow, UnreadyHigh));
	Palette->SetPaletteEnabled(true);
	TestTrue(TEXT("Valid authored setup enables before the visualizer's MID is ready"), Palette->bPaletteEnabled);
	TestFalse(TEXT("Viewer request before BeginPlay is ignored"), Palette->TryViewerPaletteAt(TEXT("ice"), 1000));
	TestFalse(TEXT("Pre-play request does not bind the receiver"), Receiver->OnCommandReceived.GetAllObjects().Contains(Palette));
	Palette->RegisterAllComponentTickFunctions(true);
	Palette->BeginPlay();
	TestTrue(TEXT("Component startup retains the authored opt-in"), Palette->bPaletteEnabled);
	TestTrue(TEXT("BeginPlay binds the receiver"), Receiver->OnCommandReceived.GetAllObjects().Contains(Palette));
	TestFalse(TEXT("Early request safely waits for displayed material initialization"), Palette->TryViewerPaletteAt(TEXT("ice"), 1000));
	TestEqual(TEXT("Early request does not consume cooldown"), Palette->ViewerCooldownRemainingAt(1000), 0.0);
	TestTrue(TEXT("Visualizer initializes its displayed material"), F.Actor->RebuildInstances());
	Palette->SetPaletteEnabled(true);
	Palette->SetPaletteEnabled(true);
	TestEqual(TEXT("Repeated enabling retains one palette listener"), Receiver->OnCommandReceived.GetAllObjects().Num(), 1);
	Palette->LastError = TEXT("Unrelated commands leave palette feedback alone");
	Receiver->OnCommandReceived.Broadcast(TEXT("pulse"), TEXT("gentle"));
	Receiver->OnCommandReceived.Broadcast(TEXT("test"), TEXT("hello"));
	Receiver->OnCommandReceived.Broadcast(TEXT("PALETTE"), TEXT("ice"));
	TestTrue(TEXT("Pulse and test are ignored"), Palette->SelectedPaletteId.IsNone());
	TestEqual(TEXT("Unrelated commands preserve feedback"), Palette->LastError,
		FString(TEXT("Unrelated commands leave palette feedback alone")));
	Receiver->ConnectionStatus = TEXT("ready");
	TestFalse(TEXT("Receiver readiness uses its exact status"), Palette->TryViewerPaletteAt(TEXT("ice"), 1000));
	Receiver->ConnectionStatus = TEXT("Ready");
	for (const auto& Value : {TEXT("ICE"), TEXT("Ice"), TEXT("unknown"), TEXT("ice extra"), TEXT(" ice"), TEXT("ice "), TEXT("")})
		TestFalse(TEXT("Only canonical viewer values are allowed"), Palette->TryViewerPaletteAt(Value, 1000));
	TestEqual(TEXT("Invalid requests do not consume cooldown"), Palette->ViewerCooldownRemainingAt(1000), 0.0);
	TestTrue(TEXT("First viewer request works immediately"), Palette->TryViewerPaletteAt(TEXT("ice"), 1000));
	TestEqual(TEXT("First accepted request counted once"), Palette->AcceptedViewerPaletteCount, 1);
	TestEqual(TEXT("Viewer palette cooldown is 15 seconds"), Palette->ViewerCooldownRemainingAt(1000), 15.0);
	TestFalse(TEXT("Burst request rejected"), Palette->TryViewerPaletteAt(TEXT("ember"), 1000.1));
	TestFalse(TEXT("Request just before cooldown boundary rejected"), Palette->TryViewerPaletteAt(TEXT("ember"), 1014.999));
	TestEqual(TEXT("Rejected requests preserve blend clock"), Palette->BlendStartedAt, 1000.0);
	TestFalse(TEXT("Same target at boundary remains a no-op"), Palette->TryViewerPaletteAt(TEXT("ice"), 1015));
	TestEqual(TEXT("No-op preserves acceptance time"), Palette->LastViewerAcceptedAt, 1000.0);
	TestTrue(TEXT("Different target accepted at exact cooldown boundary"), Palette->TryViewerPaletteAt(TEXT("sunset"), 1015));
	TestEqual(TEXT("Boundary acceptance counted"), Palette->AcceptedViewerPaletteCount, 2);
	Palette->SetViewerPalettesEnabled(false);
	TestFalse(TEXT("Viewer lock rejects a new request"), Palette->TryViewerPaletteAt(TEXT("ember"), 1030));
	TestTrue(TEXT("Accepted transition finishes through a viewer lock"), Palette->AdvanceBlend(1015.5));
	Colors(*this, F.Actor, Defaults->Presets[1].LowColor, Defaults->Presets[1].HighColor);
	TestTrue(TEXT("Local selection bypasses viewer lock and cooldown"), Palette->SelectPaletteAt(TEXT("ember"), 1016));
	Palette->AdvanceBlend(1016.5);
	TestTrue(TEXT("Local reset bypasses viewer lock and cooldown"), Palette->ResetPaletteAt(1017));
	Palette->AdvanceBlend(1017.5);
	TestEqual(TEXT("Local selection and reset preserve viewer cooldown"), Palette->LastViewerAcceptedAt, 1015.0);
	TestEqual(TEXT("Local controls do not count as viewer accepts"), Palette->AcceptedViewerPaletteCount, 2);
	Palette->SetPaletteEnabled(false);
	TestFalse(TEXT("Overall feature disable rejects viewer requests"), Palette->TryViewerPaletteAt(TEXT("next"), 1018));
	Palette->SetPaletteEnabled(true);
	Palette->SetViewerPalettesEnabled(true);
	TestFalse(TEXT("Disable/re-enable does not bypass cooldown"), Palette->TryViewerPaletteAt(TEXT("next"), 1018));
	TestTrue(TEXT("Bare palette selects first preset after reset"), Palette->TryViewerPaletteAt(TEXT("next"), 1030));
	TestEqual(TEXT("Bare palette target is Neon"), Palette->SelectedPaletteId, FName(TEXT("neon")));
	Receiver->Disconnect();
	TestTrue(TEXT("Accepted transition finishes through disconnect"), Palette->AdvanceBlend(1030.5));
	Colors(*this, F.Actor, Defaults->Presets[0].LowColor, Defaults->Presets[0].HighColor);
	TestFalse(TEXT("Disconnected viewer request rejected"), Palette->TryViewerPaletteAt(TEXT("sunset"), 1045));
	TestTrue(TEXT("Local palette remains available while disconnected"), Palette->SelectPaletteAt(TEXT("ice"), 1031));
	Palette->AdvanceBlend(1031.5);
	Receiver->ConnectionStatus = TEXT("Ready");
	TestEqual(TEXT("Reconnect does not replay a request"), Palette->AcceptedViewerPaletteCount, 3);
	TestEqual(TEXT("Reconnect retains local palette"), Palette->SelectedPaletteId, FName(TEXT("ice")));
	TestTrue(TEXT("Fresh request accepted after reconnect"), Palette->TryViewerPaletteAt(TEXT("sunset"), 1045));
	F.Manager->StreamerBot = nullptr;
	TestFalse(TEXT("Missing receiver safely rejects viewer requests"), Palette->TryViewerPaletteAt(TEXT("next"), 1060));
	F.Manager->StreamerBot = Receiver;
	auto* InvalidAsset = NewObject<URTCQTPalettePresets>(F.Manager);
	InvalidAsset->Presets.RemoveAll([](const auto& Preset) { return Preset.Id == TEXT("ice"); });
	Palette->PresetAsset = InvalidAsset;
	TestFalse(TEXT("Unconfigured named preset is rejected"), Palette->TryViewerPaletteAt(TEXT("ice"), 1060));
	InvalidAsset->Presets.Reset();
	TestFalse(TEXT("Invalid catalog rejects viewer requests"), Palette->TryViewerPaletteAt(TEXT("next"), 1060));
	TestEqual(TEXT("Invalid catalog preserves acceptance time"), Palette->LastViewerAcceptedAt, 1045.0);
	TestEqual(TEXT("Invalid catalog preserves accepted count"), Palette->AcceptedViewerPaletteCount, 4);
	Palette->PresetAsset = nullptr;
	Palette->EndPlay(EEndPlayReason::EndPlayInEditor);
	TestFalse(TEXT("EndPlay removes the receiver binding"), Receiver->OnCommandReceived.GetAllObjects().Contains(Palette));
	TestFalse(TEXT("EndPlay restores authored colors"), F.Actor->HasRuntimePaletteOverride());
	Colors(*this, F.Actor, AuthoredLow, AuthoredHigh);
	Receiver->OnCommandReceived.Broadcast(TEXT("palette"), TEXT("ember"));
	TestTrue(TEXT("An event after EndPlay has no effect"), Palette->SelectedPaletteId.IsNone());
	Palette->SetPaletteEnabled(true);
	Palette->BeginPlay();
	TestEqual(TEXT("New session has no prior viewer cooldown"), Palette->ViewerCooldownRemainingAt(2000), 0.0);
	TestEqual(TEXT("New session has no accepted requests"), Palette->AcceptedViewerPaletteCount, 0);
	Receiver->OnCommandReceived.Broadcast(TEXT("palette"), TEXT("ember"));
	Receiver->OnCommandReceived.Broadcast(TEXT("palette"), TEXT("ember"));
	TestEqual(TEXT("Delegate routes one changed request"), Palette->AcceptedViewerPaletteCount, 1);
	TestEqual(TEXT("Delegate selects named palette"), Palette->SelectedPaletteId, FName(TEXT("ember")));
	Palette->EndPlay(EEndPlayReason::EndPlayInEditor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPalettePlaybackPersistenceTest, "RTCQT.Palette.MusicLabSwitchRestartAndPause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPalettePlaybackPersistenceTest::RunTest(const FString& Parameters)
{
	using namespace RTCQTPaletteTests;
	FFixture F;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(F.World);
	F.Manager->Catalog = NewObject<URTCQTMusicCatalog>(F.Manager);
	for (const int32 Count : {16, 64, 32})
	{
		FRTCQTMusicTrack Track;
		Track.Id = FString::FromInt(Count);
		Track.Label = Track.Id;
		Track.Analyzer = F.MakeAnalyzer(Count);
		Track.SoundWave = Track.Analyzer->Sound;
		F.Manager->Catalog->Tracks.Add(Track);
	}
	auto* Palette = F.Palette;
	Palette->SetPaletteEnabled(true);
	const auto& Ice = GetDefault<URTCQTPalettePresets>()->Presets[2];
	F.Actor->SetActorScale3D(FVector(1.15));
	const auto PulseTransform = F.Actor->GetActorTransform();
	TestTrue(TEXT("Begin palette transition before playback"), Palette->SelectPaletteAt(TEXT("ice"), 1000));
	TestTrue(TEXT("Select 16-band track through Music Lab"), F.Manager->SelectTrack(TEXT("16")));
	auto* InitialAudio = F.Manager->AudioComponent.Get();
	InitialAudio->OnAudioPlaybackPercentNative.Broadcast(InitialAudio, F.Manager->CurrentTrack.SoundWave, 0.f);
	Palette->AdvanceBlend(1000.25);
	const auto MidLow = FMath::LerpStable(AuthoredLow, Ice.LowColor, 0.5f);
	const auto MidHigh = FMath::LerpStable(AuthoredHigh, Ice.HighColor, 0.5f);
	Colors(*this, F.Actor, MidLow, MidHigh);
	TestEqual(TEXT("16 analyzed instances rendered"), F.Instances->GetInstanceCount(), 16);
	TestTrue(TEXT("Switch to 64 bands during transition"), F.Manager->SelectTrack(TEXT("64")));
	TestTrue(TEXT("Track switch replaced the audio component"), F.Manager->AudioComponent != InitialAudio);
	F.Manager->AudioComponent->OnAudioPlaybackPercentNative.Broadcast(
		F.Manager->AudioComponent, F.Manager->CurrentTrack.SoundWave, 0.f);
	Colors(*this, F.Actor, MidLow, MidHigh);
	TestEqual(TEXT("64 analyzed instances rendered"), F.Instances->GetInstanceCount(), 64);
	Palette->AdvanceBlend(1000.5);
	Colors(*this, F.Actor, Ice.LowColor, Ice.HighColor);
	auto* BeforeRestart = F.Manager->AudioComponent.Get();
	TestTrue(TEXT("Restart current track through Music Lab"), F.Manager->Restart());
	TestTrue(TEXT("Same-track restart replaced the audio component"), F.Manager->AudioComponent != BeforeRestart);
	F.Manager->AudioComponent->OnAudioPlaybackPercentNative.Broadcast(
		F.Manager->AudioComponent, F.Manager->CurrentTrack.SoundWave, 0.f);
	TestEqual(TEXT("Selection survives restart"), Palette->SelectedPaletteId, FName(TEXT("ice")));
	Colors(*this, F.Actor, Ice.LowColor, Ice.HighColor);
	F.Manager->TogglePlayPause();
	TestTrue(TEXT("Music Lab is paused"), F.Manager->PlaybackState == ERTCQTMusicPlaybackState::Paused);
	Colors(*this, F.Actor, Ice.LowColor, Ice.HighColor);
	F.Manager->TogglePlayPause();
	TestTrue(TEXT("Music Lab resumed"), F.Manager->PlaybackState == ERTCQTMusicPlaybackState::Playing);
	TestTrue(TEXT("Switch to 32-band track"), F.Manager->SelectTrack(TEXT("32")));
	auto* CurrentAudio = F.Manager->AudioComponent.Get();
	CurrentAudio->OnAudioPlaybackPercentNative.Broadcast(CurrentAudio, F.Manager->CurrentTrack.SoundWave, 0.f);
	TestEqual(TEXT("32 analyzed instances rendered"), F.Instances->GetInstanceCount(), 32);
	Colors(*this, F.Actor, Ice.LowColor, Ice.HighColor);
	TestFalse(TEXT("Invalid track rejected"), F.Manager->SelectTrack(TEXT("missing")));
	TestTrue(TEXT("Invalid track preserves current audio"), F.Manager->AudioComponent == CurrentAudio);
	TestEqual(TEXT("Invalid track preserves palette selection"), Palette->SelectedPaletteId, FName(TEXT("ice")));
	CurrentAudio->OnAudioFinishedNative.Broadcast(CurrentAudio);
	TestTrue(TEXT("Music Lab reports completion"), F.Manager->PlaybackState == ERTCQTMusicPlaybackState::Stopped);
	TestEqual(TEXT("Completion clears geometry"), F.Instances->GetInstanceCount(), 0);
	Colors(*this, F.Actor, Ice.LowColor, Ice.HighColor);
	TestTrue(TEXT("Restart after completion succeeds"), F.Manager->Restart());
	F.Manager->AudioComponent->OnAudioPlaybackPercentNative.Broadcast(
		F.Manager->AudioComponent, F.Manager->CurrentTrack.SoundWave, 0.f);
	Colors(*this, F.Actor, Ice.LowColor, Ice.HighColor);
	TestTrue(TEXT("Palette and playback changes preserve pulse actor transform"), F.Actor->GetActorTransform().Equals(PulseTransform));
	auto* AudioBeforeReset = F.Manager->AudioComponent.Get();
	const int32 CallbacksBeforeReset = F.Manager->PlaybackCallbackCount;
	TestTrue(TEXT("Reset palette while playback continues"), Palette->ResetPaletteAt(1001));
	Palette->AdvanceBlend(1001.5);
	Colors(*this, F.Actor, AuthoredLow, AuthoredHigh);
	TestTrue(TEXT("Reset does not restart audio"), F.Manager->AudioComponent == AudioBeforeReset);
	TestEqual(TEXT("Reset does not manufacture an analysis callback"), F.Manager->PlaybackCallbackCount, CallbacksBeforeReset);
	TestTrue(TEXT("Reset leaves playback active"), F.Manager->PlaybackState == ERTCQTMusicPlaybackState::Playing);
	F.World->DestroyWorld(false);
	GEngine->DestroyWorldContext(F.World);
	F.World = nullptr;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTPaletteControlsTest, "RTCQT.Palette.Controls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTPaletteControlsTest::RunTest(const FString& Parameters)
{
	using namespace RTCQTPaletteTests;
	FFixture F;
	F.AddWorldContext();
	F.Manager->Catalog = NewObject<URTCQTMusicCatalog>(F.Manager);
	FRTCQTMusicTrack Track;
	Track.Id = TEXT("fixture");
	Track.Label = TEXT("Synthetic test track");
	Track.Analyzer = F.Analyzer;
	Track.SoundWave = F.Analyzer->Sound;
	F.Manager->Catalog->Tracks.Add(Track);
	TestTrue(TEXT("Playback is active before constructing controls"), F.Manager->SelectTrack(Track.Id));
	auto* Audio = F.Manager->AudioComponent.Get();
	const int32 CallbackCount = F.Manager->PlaybackCallbackCount;
	F.Palette->RegisterAllComponentTickFunctions(true);
	F.Palette->BeginPlay();
	auto* Controls = NewObject<URTCQTMusicLabControls>(F.World);
	Controls->Manager = F.Manager;
	TestTrue(TEXT("Native widget initializes"), Controls->Initialize());
	TSharedPtr<SWidget> Slate = Controls->TakeWidget();
	Controls->NativeTick(FGeometry(), 0.f);
	TestEqual(TEXT("Track choices remain available"), Controls->GetTrackDropdown()->GetOptionCount(), 1);
	TestEqual(TEXT("Palette choices include authored colors plus four presets"), Controls->GetPaletteDropdown()->GetOptionCount(), 5);
	TestFalse(TEXT("UI starts with palette opt-in off"), Controls->PaletteEnabled->IsChecked());
	TestFalse(TEXT("Palette dropdown is disabled until enabled"), Controls->PaletteDropdown->GetIsEnabled());
	TestTrue(TEXT("Initial UI synchronization does not select a palette"), F.Palette->SelectedPaletteId.IsNone());
	TestFalse(TEXT("Initial UI synchronization does not apply a color override"), F.Actor->HasRuntimePaletteOverride());
	CaptureControls(*this, Slate.ToSharedRef(), TEXT("01-disabled"));
	Controls->PaletteEnabled->OnCheckStateChanged.Broadcast(true);
	TestTrue(TEXT("Feature checkbox enables the component"), F.Palette->bPaletteEnabled);
	TestTrue(TEXT("Enabled palette dropdown accepts input"), Controls->PaletteDropdown->GetIsEnabled());
	Controls->PaletteDropdown->SetSelectedOption(TEXT("Ice"));
	TestTrue(TEXT("Programmatic selection has no effect"), F.Palette->SelectedPaletteId.IsNone());
	Controls->PaletteDropdown->OnSelectionChanged.Broadcast(TEXT("Ice"), ESelectInfo::OnMouseClick);
	TestEqual(TEXT("User selection maps to stable palette ID"), F.Palette->SelectedPaletteId, FName(TEXT("ice")));
	TestEqual(TEXT("Dropdown follows local selection"), Controls->PaletteDropdown->GetSelectedOption(), FString(TEXT("Ice")));
	CaptureControls(*this, Slate.ToSharedRef(), TEXT("02-ice"));
	Controls->ViewerPalettes->OnCheckStateChanged.Broadcast(false);
	TestFalse(TEXT("Viewer checkbox locks only viewer palettes"), F.Palette->bViewerPalettesEnabled);
	TestTrue(TEXT("Viewer lock leaves local dropdown enabled"), Controls->PaletteDropdown->GetIsEnabled());
	Controls->PaletteNextButton->OnClicked.Broadcast();
	TestEqual(TEXT("Next button uses shared cycle order through viewer lock"), F.Palette->SelectedPaletteId, FName(TEXT("ember")));
	CaptureControls(*this, Slate.ToSharedRef(), TEXT("03-viewer-locked"));
	Controls->PaletteResetButton->OnClicked.Broadcast();
	TestTrue(TEXT("Reset button uses authored colors"), F.Palette->SelectedPaletteId.IsNone());
	TestEqual(TEXT("Dropdown follows reset"), Controls->PaletteDropdown->GetSelectedOption(), FString(TEXT("Authored colors")));
	Controls->ViewerPalettes->OnCheckStateChanged.Broadcast(true);
	F.Manager->StreamerBot->ConnectionStatus = TEXT("Ready");
	F.Manager->StreamerBot->OnCommandReceived.Broadcast(TEXT("palette"), TEXT("ice"));
	Controls->RefreshPaletteState();
	TestEqual(TEXT("Dropdown follows an accepted viewer change"), Controls->PaletteDropdown->GetSelectedOption(), FString(TEXT("Ice")));
	TestTrue(TEXT("Accepted viewer change has brief feedback"), Controls->PaletteFeedback->GetText().ToString().Contains(TEXT("viewer")));
	Controls->RefreshPaletteState();
	TestEqual(TEXT("Feedback synchronization does not replay requests"), F.Palette->AcceptedViewerPaletteCount, 1);
	Controls->PaletteFeedbackUntil = FPlatformTime::Seconds() - 1;
	Controls->RefreshPaletteState();
	TestTrue(TEXT("Brief feedback expires"), Controls->PaletteFeedback->GetText().IsEmpty());
	TestTrue(TEXT("External local selection is accepted during viewer cooldown"), F.Palette->SelectPalette(TEXT("sunset")));
	Controls->RefreshPaletteState();
	TestEqual(TEXT("Dropdown follows external local selection"), Controls->PaletteDropdown->GetSelectedOption(), FString(TEXT("Sunset")));
	auto* ArtistAsset = NewObject<URTCQTPalettePresets>(F.Manager);
	ArtistAsset->Presets[0].DisplayName = FText::FromString(TEXT("Same label"));
	ArtistAsset->Presets[1].DisplayName = FText::FromString(TEXT("Same label"));
	F.Palette->PresetAsset = ArtistAsset;
	Controls->RefreshPaletteState();
	Controls->PaletteDropdown->OnSelectionChanged.Broadcast(TEXT("Same label"), ESelectInfo::OnMouseClick);
	TestEqual(TEXT("First duplicate label maps to Neon"), F.Palette->SelectedPaletteId, FName(TEXT("neon")));
	Controls->PaletteDropdown->OnSelectionChanged.Broadcast(TEXT("Same label (2)"), ESelectInfo::OnMouseClick);
	TestEqual(TEXT("Second duplicate label maps to Sunset"), F.Palette->SelectedPaletteId, FName(TEXT("sunset")));
	Controls->ReleaseSlateResources(true);
	Slate.Reset();
	Slate = Controls->TakeWidget();
	Controls->RefreshPaletteState();
	TestEqual(TEXT("Widget rebuild preserves palette selection"), F.Palette->SelectedPaletteId, FName(TEXT("sunset")));
	TestEqual(TEXT("Rebuilt dropdown has no duplicate entries"), Controls->PaletteDropdown->GetOptionCount(), 5);
	Controls->PaletteEnabled->OnCheckStateChanged.Broadcast(false);
	TestFalse(TEXT("Disabling from UI restores authored colors"), F.Actor->HasRuntimePaletteOverride());
	ArtistAsset->Presets[1].Id = ArtistAsset->Presets[0].Id;
	Controls->PaletteEnabled->OnCheckStateChanged.Broadcast(true);
	TestFalse(TEXT("Invalid catalog cannot be enabled by the UI"), F.Palette->bPaletteEnabled);
	TestFalse(TEXT("Checkbox reflects rejected opt-in"), Controls->PaletteEnabled->IsChecked());
	TestFalse(TEXT("Invalid catalog leaves selection disabled"), Controls->PaletteDropdown->GetIsEnabled());
	TestFalse(TEXT("Rejected setup has visible feedback"), Controls->PaletteFeedback->GetText().IsEmpty());
	TestTrue(TEXT("UI did not replace the audio component"), F.Manager->AudioComponent == Audio);
	TestEqual(TEXT("UI did not advance music analysis"), F.Manager->PlaybackCallbackCount, CallbackCount);
	TestTrue(TEXT("UI left playback active"), F.Manager->PlaybackState == ERTCQTMusicPlaybackState::Playing);
	Controls->Manager = nullptr;
	Controls->RefreshPaletteState();
	TestFalse(TEXT("Missing manager disables feature checkbox safely"), Controls->PaletteEnabled->GetIsEnabled());
	F.Palette->EndPlay(EEndPlayReason::EndPlayInEditor);
	Controls->ReleaseSlateResources(true);
	Slate.Reset();
	return true;
}

#endif
