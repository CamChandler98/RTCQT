#pragma once

#include "CoreMinimal.h"

/** Carries a UObject-free snapshot of the authored D02 response parameters. */
struct RTCQT_API FRTCQTResponseParameters
{
	/** Defines the lower edge of the useful analyzer input window. */
	float NoiseFloor = 0.1f;

	/** Defines the upper edge of the useful analyzer input window. */
	float InputCeiling = 1.0f;

	/** Shapes the weighted normalized response without temporal state. */
	float ResponseExponent = 2.0f;

	/** Supplies optional index-aligned weights; missing entries use one. */
	TArray<float> BandWeights;
};

/** Implements the stateless D02 analyzer-value to visual-response contract. */
class RTCQT_API FRTCQTResponseProcessor
{
public:
	/** Processes one analyzer value using its band-indexed weight and returns [0, 1]. */
	static float ProcessBand(
		float Input,
		const FRTCQTResponseParameters& Parameters,
		int32 BandIndex);

	/** Replaces OutResponses with one normalized response per input value. */
	static void ProcessBands(
		TConstArrayView<float> Inputs,
		const FRTCQTResponseParameters& Parameters,
		TArray<float>& OutResponses);

	/** Maps a normalized response through sanitized, ordered output bounds. */
	static float MapResponse(float Response, float OutputMin, float OutputMax);
};
