#pragma once

#include "CoreMinimal.h"
#include "RTCQTTypes.h"

/** Preserves a D03 transform as independently testable translation and axes. */
struct RTCQT_API FRTCQTLayoutTransform
{
	/** Stores the instance center in visualizer-local space. */
	FVector Translation = FVector::ZeroVector;

	/** Stores the instance local positive-X direction. */
	FVector Forward = FVector::ForwardVector;

	/** Stores the instance local positive-Z direction. */
	FVector Up = FVector::UpVector;

	/** Stores the sanitized D03 base scale. */
	FVector Scale = FVector::OneVector;

	/** Converts the documented axes and values into an Unreal local transform. */
	FTransform ToTransform() const;
};

/** Generates one deterministic D03 layout transform per input band index. */
class RTCQT_API FRTCQTLayout
{
public:
	/** Replaces OutTransforms with Count ordered transforms for the selected layout. */
	static void Build(
		const FRTCQTLayoutSettings& Settings,
		int32 Count,
		TArray<FRTCQTLayoutTransform>& OutTransforms);
};
