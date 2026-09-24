#include "RTCQTVisualizer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "RTCQTLayout.h"
#include "RTCQTLog.h"

namespace
{
bool SameScalar(double A, double B)
{
	// Invalid values sanitize to the same zero; NaN must not defeat the cache.
	return A == B || (!FMath::IsFinite(A) && !FMath::IsFinite(B));
}

bool SameVector(const FVector& A, const FVector& B)
{
	return SameScalar(A.X, B.X) && SameScalar(A.Y, B.Y) && SameScalar(A.Z, B.Z);
}

bool SameLayout(const FRTCQTLayoutSettings& A, const FRTCQTLayoutSettings& B)
{
	return A.Mode == B.Mode && SameVector(A.Origin, B.Origin)
		&& SameVector(A.BaseScale, B.BaseScale) && SameScalar(A.Spacing, B.Spacing)
		&& SameScalar(A.Radius, B.Radius);
}
}

ARTCQTVisualizer::ARTCQTVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	InstanceComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
		TEXT("RTCQTInstances"));
	InstanceComponent->SetNumCustomDataFloats(RTCQTCustomData::NumFloats);
	InstanceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InstanceComponent->SetCanEverAffectNavigation(false);
	SetRootComponent(InstanceComponent);
}

void ARTCQTVisualizer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildInstances();
}

void ARTCQTVisualizer::BeginPlay()
{
	Super::BeginPlay();
	RebuildInstances();
}

FRTCQTEvaluationResult ARTCQTVisualizer::EvaluateAtTime(float TimeSeconds)
{
	FRTCQTEvaluationResult Result;
	Result.EvaluatedTimeSeconds = TimeSeconds;
	return Result;
}

bool ARTCQTVisualizer::RebuildInstances()
{
	if (!InstanceComponent)
	{
		return FailRebuild(ERTCQTFailureReason::InstanceUpdateFailed,
			TEXT("RTCQTInstances component is missing; restore the visualizer actor."));
	}
	if (!InstanceMesh)
	{
		InstanceComponent->SetStaticMesh(nullptr);
		return FailRebuild(ERTCQTFailureReason::MissingMesh,
			TEXT("Assign Instance Mesh to display the configured bands."));
	}
	if (ExpectedBandCount <= 0)
	{
		return FailRebuild(ERTCQTFailureReason::InvalidExpectedBandCount,
			TEXT("Set Expected Band Count above zero to display instances."));
	}
	if (LayoutSettings.Mode != ERTCQTLayoutMode::Line)
	{
		return FailRebuild(ERTCQTFailureReason::LayoutGenerationFailed,
			TEXT("Select Line layout; Circle and Fibonacci Sphere are deferred to D19."));
	}

	InstanceComponent->SetStaticMesh(InstanceMesh);
	InstanceComponent->SetMaterial(0, InstanceMaterial);
	if (!InstanceMaterial && !bUsingDefaultMaterial)
	{
		UE_LOG(LogRTCQT, Warning, TEXT("%s: Instance Material is unset; using the mesh's default material. Assign an override if needed."), *GetName());
	}
	bUsingDefaultMaterial = !InstanceMaterial;

	if (bHasCachedLayout && CachedBandCount == ExpectedBandCount
		&& SameLayout(CachedLayoutSettings, LayoutSettings)
		&& InstanceComponent->GetInstanceCount() == ExpectedBandCount
		&& InstanceComponent->NumCustomDataFloats == RTCQTCustomData::NumFloats)
	{
		LastFailureReason = ERTCQTFailureReason::None;
		return true;
	}

	TArray<FRTCQTLayoutTransform> Layout;
	FRTCQTLayout::Build(LayoutSettings, ExpectedBandCount, Layout);
	if (Layout.Num() != ExpectedBandCount)
	{
		return FailRebuild(ERTCQTFailureReason::LayoutGenerationFailed,
			TEXT("Layout count does not match Expected Band Count; check the layout settings."));
	}
	TArray<FTransform> Transforms;
	Transforms.Reserve(Layout.Num());
	for (const FRTCQTLayoutTransform& Item : Layout)
	{
		const FTransform Transform = Item.ToTransform();
		if (!Transform.IsValid())
		{
			return FailRebuild(ERTCQTFailureReason::LayoutGenerationFailed,
				TEXT("Layout produced an invalid transform; check origin, scale, and spacing."));
		}
		Transforms.Add(Transform);
	}

	ResetInstances();
	InstanceComponent->SetNumCustomDataFloats(RTCQTCustomData::NumFloats);
	InstanceComponent->AddInstances(Transforms, false, false, false);
	if (InstanceComponent->GetInstanceCount() != ExpectedBandCount)
	{
		return FailRebuild(ERTCQTFailureReason::InstanceUpdateFailed,
			TEXT("Could not create the requested instances; rebuild the visualizer."));
	}
	for (int32 Index = 0; Index < ExpectedBandCount; ++Index)
	{
		const float BandIndex = ExpectedBandCount > 1
			? static_cast<float>(Index) / (ExpectedBandCount - 1) : 0.0f;
		if (!InstanceComponent->SetCustomDataValue(Index, RTCQTCustomData::Response, 0.0f)
			|| !InstanceComponent->SetCustomDataValue(Index, RTCQTCustomData::NormalizedBandIndex, BandIndex))
		{
			return FailRebuild(ERTCQTFailureReason::InstanceUpdateFailed,
				TEXT("Could not initialize instance custom data; rebuild the visualizer."));
		}
	}
	InstanceComponent->MarkRenderStateDirty();
	CachedLayoutSettings = LayoutSettings;
	CachedBandCount = ExpectedBandCount;
	bHasCachedLayout = true;
	LastFailureReason = ERTCQTFailureReason::None;
	return true;
}

bool ARTCQTVisualizer::FailRebuild(ERTCQTFailureReason Reason, const TCHAR* Message)
{
	ResetInstances();
	if (LastFailureReason != Reason)
	{
		UE_LOG(LogRTCQT, Warning, TEXT("%s: %s"), *GetName(), Message);
	}
	LastFailureReason = Reason;
	return false;
}

void ARTCQTVisualizer::ResetInstances()
{
	if (InstanceComponent)
	{
		InstanceComponent->ClearInstances();
	}
	bHasCachedLayout = false;
	CachedBandCount = INDEX_NONE;
}

void ARTCQTVisualizer::ClearVisualization()
{
	ResetInstances();
	LastFailureReason = ERTCQTFailureReason::None;
}
