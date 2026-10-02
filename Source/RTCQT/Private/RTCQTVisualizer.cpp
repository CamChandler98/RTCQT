#include "RTCQTVisualizer.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "ConstantQNRT.h"
#include "RTCQTAnalyzerQuery.h"
#include "RTCQTLayout.h"
#include "RTCQTLog.h"
#include "RTCQTResponseSettings.h"

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
	const auto Fail = [this, &Result](ERTCQTFailureReason Reason, const TCHAR* Message)
	{
		Result.bSucceeded = false;
		Result.FailureReason = Reason;
		FailRebuild(Reason, Message);
		return Result;
	};
	if (!FMath::IsFinite(TimeSeconds))
		return Fail(ERTCQTFailureReason::InvalidTime, TEXT("Song time must be finite."));
	if (!ConstantQNRT)
		return Fail(ERTCQTFailureReason::MissingAnalyzer, TEXT("Assign Constant QNRT before evaluating song time."));
	if (!ConstantQNRT->Sound)
		return Fail(ERTCQTFailureReason::MissingAnalyzedSound, TEXT("Constant QNRT has no analyzed Sound; assign the test SoundWave and analyze it."));
	if (!ResponseSettings)
		return Fail(ERTCQTFailureReason::MissingResponseSettings, TEXT("Assign Response Settings before evaluating song time."));
	if (!InstanceMesh)
		return Fail(ERTCQTFailureReason::MissingMesh, TEXT("Assign Instance Mesh to display the configured bands."));
	if (ExpectedBandCount <= 0)
		return Fail(ERTCQTFailureReason::InvalidExpectedBandCount, TEXT("Set Expected Band Count above zero to display instances."));

	TArray<float> Bands;
	Result = RTCQTAnalysis::QueryBandsAtTime(ConstantQNRT, TimeSeconds, AnalyzerChannel, ExpectedBandCount, Bands);
	if (!Result.bSucceeded)
	{
		FString Message;
		switch (Result.FailureReason)
		{
		case ERTCQTFailureReason::TimeOutsideAnalysis:
			Message = FString::Printf(TEXT("Song time %.9g is outside a valid analysis duration of %.9g seconds."),
				TimeSeconds, ConstantQNRT->DurationInSeconds);
			break;
		case ERTCQTFailureReason::EmptyAnalyzerResult:
			Message = FString::Printf(TEXT("Constant QNRT has no queryable data for channel %d; check the channel, settings, and completed analysis."), AnalyzerChannel);
			break;
		case ERTCQTFailureReason::BandCountMismatch:
			Message = FString::Printf(TEXT("Constant QNRT returned %d bands; Expected Band Count is %d. Match the analyzer and visualizer counts."),
				Result.ObservedBandCount, ExpectedBandCount);
			break;
		default:
			Message = TEXT("Could not query Constant QNRT; check the assigned analyzer and song time.");
			break;
		}
		return Fail(Result.FailureReason, *Message);
	}
	if (!RebuildInstances())
	{
		Result.bSucceeded = false;
		Result.FailureReason = LastFailureReason;
		return Result;
	}

	TArray<float> Responses;
	FRTCQTResponseProcessor::ProcessBands(Bands, ResponseSettings->MakeProcessorParameters(), Responses);
	TArray<FRTCQTLayoutTransform> Layout;
	FRTCQTLayout::Build(LayoutSettings, ExpectedBandCount, Layout);
	if (Layout.Num() != Responses.Num())
	{
		return Fail(ERTCQTFailureReason::LayoutGenerationFailed,
			TEXT("Base layout does not match the evaluated bands; rebuild the visualizer."));
	}

	// Recreate values from the base layout, never from a previously scaled frame.
	// MeshZScaleRange is absolute local Z scale; X/Y, position, and rotation stay authored.
	TArray<FTransform> Transforms;
	Transforms.Reserve(Layout.Num());
	for (int32 Index = 0; Index < Layout.Num(); ++Index)
	{
		FTransform Transform = Layout[Index].ToTransform();
		FVector Scale = Transform.GetScale3D();
		Scale.Z = FRTCQTResponseProcessor::MapResponse(Responses[Index],
			ResponseSettings->MeshZScaleRange.Min, ResponseSettings->MeshZScaleRange.Max);
		Transform.SetScale3D(Scale);
		if (!Transform.IsValid())
		{
			return Fail(ERTCQTFailureReason::LayoutGenerationFailed,
				TEXT("Response produced an invalid instance transform; check the layout and scale range."));
		}
		Transforms.Add(Transform);
	}
	if (!InstanceComponent->BatchUpdateInstancesTransforms(0, Transforms, false, false, true))
	{
		return Fail(ERTCQTFailureReason::InstanceUpdateFailed,
			TEXT("Could not apply the evaluated instance transforms; rebuild the visualizer."));
	}
	for (int32 Index = 0; Index < Responses.Num(); ++Index)
	{
		if (!InstanceComponent->SetCustomDataValue(Index, RTCQTCustomData::Response, Responses[Index], false))
		{
			return Fail(ERTCQTFailureReason::InstanceUpdateFailed,
				TEXT("Could not apply the evaluated response custom data; rebuild the visualizer."));
		}
	}
	InstanceComponent->MarkRenderStateDirty();
	LastFailureReason = ERTCQTFailureReason::None;
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
	// A missing member reference must still clear the actor-owned default
	// component, which may remain attached and rendered through the root.
	UInstancedStaticMeshComponent* OwnedInstances = InstanceComponent.Get();
	if (!OwnedInstances)
	{
		OwnedInstances = Cast<UInstancedStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("RTCQTInstances")));
	}
	if (OwnedInstances)
	{
		OwnedInstances->ClearInstances();
	}
	bHasCachedLayout = false;
	CachedBandCount = INDEX_NONE;
}

void ARTCQTVisualizer::ClearVisualization()
{
	ResetInstances();
	LastFailureReason = ERTCQTFailureReason::None;
}
