#include "RTCQTVisualizer.h"

#include "Components/InstancedStaticMeshComponent.h"

ARTCQTVisualizer::ARTCQTVisualizer()
{
	PrimaryActorTick.bCanEverTick = false;

	InstanceComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
		TEXT("RTCQTInstances"));
	SetRootComponent(InstanceComponent);
}

FRTCQTEvaluationResult ARTCQTVisualizer::EvaluateAtTime(float TimeSeconds)
{
	FRTCQTEvaluationResult Result;
	Result.EvaluatedTimeSeconds = TimeSeconds;
	return Result;
}

bool ARTCQTVisualizer::RebuildInstances()
{
	return false;
}

void ARTCQTVisualizer::ClearVisualization()
{
	if (InstanceComponent)
	{
		InstanceComponent->ClearInstances();
	}

	LastFailureReason = ERTCQTFailureReason::None;
}
