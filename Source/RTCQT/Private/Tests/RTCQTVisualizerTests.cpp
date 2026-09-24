#if WITH_DEV_AUTOMATION_TESTS

#include "RTCQTVisualizer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace
{
struct FVisualizerFixture
{
	UWorld* World;
	ARTCQTVisualizer* Actor;
	UInstancedStaticMeshComponent* Instances;
	UStaticMesh* Cube;

	FVisualizerFixture()
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
		SetAsset(TEXT("InstanceMesh"), Cube);
		SetAsset(TEXT("InstanceMaterial"), Cube->GetMaterial(0));
		Actor->FinishSpawning(FTransform::Identity);
		Instances = Actor->FindComponentByClass<UInstancedStaticMeshComponent>();
	}

	~FVisualizerFixture() { World->DestroyWorld(false); }

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
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTStaticInstances,
	"RTCQT.D08.Visualizer.TopologyAndCache", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTStaticInstances::RunTest(const FString& Parameters)
{
	FVisualizerFixture F;
	TestEqual(TEXT("Construction creates 32 instances"), F.Instances->GetInstanceCount(), 32);
	TInlineComponentArray<UInstancedStaticMeshComponent*> Components(F.Actor);
	TestEqual(TEXT("One instanced component"), Components.Num(), 1);
	TestFalse(TEXT("No time-advancing tick"), F.Actor->PrimaryActorTick.bCanEverTick);
	TestEqual(TEXT("Two floats per instance"), F.Instances->NumCustomDataFloats, 2);
	TestEqual(TEXT("Custom data allocation"), F.Instances->PerInstanceSMCustomData.Num(), 64);
	for (int32 Index = 0; Index < 32; ++Index)
	{
		TestEqual(TEXT("Initial response"), F.Instances->PerInstanceSMCustomData[Index * 2], 0.0f);
		TestEqual(TEXT("Normalized band ordering"), F.Instances->PerInstanceSMCustomData[Index * 2 + 1], Index / 31.0f);
	}
	// A sentinel stands in for a later D11/D12 response: unchanged topology must preserve it.
	F.Instances->SetCustomDataValue(0, RTCQTCustomData::Response, 0.42f);
	TestTrue(TEXT("Unchanged rebuild succeeds"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Unchanged topology preserves response"), F.Instances->PerInstanceSMCustomData[0], 0.42f);
	F.Actor->SetActorTransform(FTransform(FRotator(0, 90, 0), FVector(20, 40, 60), FVector(2)));
	F.Actor->RerunConstructionScripts();
	TestEqual(TEXT("Moving the actor preserves topology"), F.Instances->PerInstanceSMCustomData[0], 0.42f);
	FTransform Local;
	FTransform World;
	F.Instances->GetInstanceTransform(0, Local, false);
	F.Instances->GetInstanceTransform(0, World, true);
	TestTrue(TEXT("Layout remains local"), Local.GetTranslation().Equals(FVector(-1550, 0, 0)));
	TestTrue(TEXT("Actor transform applies once"), World.GetTranslation().Equals(F.Actor->GetActorTransform().TransformPosition(Local.GetTranslation())));
	F.SetAsset(TEXT("InstanceMesh"), LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	TestTrue(TEXT("Mesh replacement succeeds"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Mesh replacement preserves topology"), F.Instances->PerInstanceSMCustomData[0], 0.42f);
	F.SetCount(1);
	F.Layout().Origin = FVector(13, 25, 80);
	F.Layout().BaseScale = FVector(0.5, 1, 2);
	TestTrue(TEXT("Changed settings rebuild"), F.Actor->RebuildInstances());
	TestEqual(TEXT("New count replaces old instances"), F.Instances->GetInstanceCount(), 1);
	F.Instances->GetInstanceTransform(0, Local, false);
	TestTrue(TEXT("Single band at configured origin"), Local.GetTranslation().Equals(F.Layout().Origin));
	TestTrue(TEXT("Configured nonuniform scale"), Local.GetScale3D().Equals(F.Layout().BaseScale));
	TestEqual(TEXT("Single band index is zero"), F.Instances->PerInstanceSMCustomData[1], 0.0f);
	F.SetCount(32);
	F.Actor->RebuildInstances();
	F.Actor->ClearVisualization();
	TestEqual(TEXT("Explicit clear removes instances"), F.Instances->GetInstanceCount(), 0);
	TestTrue(TEXT("Explicit clear invalidates cache"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Rebuild after clear restores count"), F.Instances->GetInstanceCount(), 32);
	F.Instances->ClearInstances();
	TestTrue(TEXT("Missing component data repairs itself"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Component repair restores count"), F.Instances->GetInstanceCount(), 32);
	F.Instances->SetNumCustomDataFloats(1);
	TestTrue(TEXT("Custom data width repairs itself"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Contract width restored"), F.Instances->NumCustomDataFloats, 2);
	F.Layout().Spacing = std::numeric_limits<float>::quiet_NaN();
	TestTrue(TEXT("Invalid spacing sanitizes"), F.Actor->RebuildInstances());
	F.Instances->SetCustomDataValue(0, RTCQTCustomData::Response, 0.42f);
	F.Actor->RebuildInstances();
	TestEqual(TEXT("NaN inputs do not defeat caching"), F.Instances->PerInstanceSMCustomData[0], 0.42f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTInstanceFailures,
	"RTCQT.D08.Visualizer.SafeFailuresAndRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTInstanceFailures::RunTest(const FString& Parameters)
{
	FVisualizerFixture F;
	AddExpectedMessagePlain(TEXT("Assign Instance Mesh"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
	F.SetAsset(TEXT("InstanceMesh"), nullptr);
	TestFalse(TEXT("Missing mesh fails"), F.Actor->RebuildInstances());
	TestFalse(TEXT("Repeated missing mesh fails without another warning"), F.Actor->RebuildInstances());
	TestEqual(TEXT("Missing mesh clears stale instances"), F.Instances->GetInstanceCount(), 0);
	F.SetAsset(TEXT("InstanceMesh"), F.Cube);
	TestTrue(TEXT("Mesh restoration recovers"), F.Actor->RebuildInstances());
	F.SetAsset(TEXT("InstanceMesh"), nullptr);
	F.Actor->RebuildInstances(); // A new failure after success is reported again.
	F.SetAsset(TEXT("InstanceMesh"), F.Cube);
	F.Actor->RebuildInstances();
	AddExpectedMessagePlain(TEXT("Set Expected Band Count above zero"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	for (int32 Count : { 0, -2, 0 })
	{
		F.SetCount(Count);
		TestFalse(TEXT("Nonpositive count fails"), F.Actor->RebuildInstances());
		TestEqual(TEXT("Nonpositive count clears instances"), F.Instances->GetInstanceCount(), 0);
	}
	F.SetCount(32);
	TestTrue(TEXT("Valid count recovers"), F.Actor->RebuildInstances());
	AddExpectedMessagePlain(TEXT("Select Line layout"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	for (ERTCQTLayoutMode Mode : { ERTCQTLayoutMode::Circle, ERTCQTLayoutMode::FibonacciSphere })
	{
		F.Layout().Mode = Mode;
		TestFalse(TEXT("Deferred modes fail explicitly"), F.Actor->RebuildInstances());
		TestEqual(TEXT("Deferred mode leaves no stale line"), F.Instances->GetInstanceCount(), 0);
	}
	F.Layout().Mode = ERTCQTLayoutMode::Line;
	TestTrue(TEXT("Line restores instances"), F.Actor->RebuildInstances());
	AddExpectedMessagePlain(TEXT("Instance Material is unset"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
	F.SetAsset(TEXT("InstanceMaterial"), nullptr);
	TestTrue(TEXT("Optional material can be missing"), F.Actor->RebuildInstances());
	F.Actor->RebuildInstances();
	TestTrue(TEXT("Mesh default material is used"), F.Instances->GetMaterial(0) == F.Cube->GetMaterial(0));
	F.SetAsset(TEXT("InstanceMaterial"), F.Cube->GetMaterial(0));
	F.Actor->RebuildInstances();
	F.SetAsset(TEXT("InstanceMaterial"), nullptr);
	F.Actor->RebuildInstances();
	TestEqual(TEXT("Optional material retains all bands"), F.Instances->GetInstanceCount(), 32);
	return true;
}

#endif
