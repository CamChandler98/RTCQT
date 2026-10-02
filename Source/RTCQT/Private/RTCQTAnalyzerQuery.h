#pragma once

#include "RTCQTTypes.h"

class UConstantQNRT;

namespace RTCQTAnalysis
{
/** Replaces OutBands with one exact-time normalized query, clearing output on failure. */
FRTCQTEvaluationResult QueryBandsAtTime(
	const UConstantQNRT* Analyzer,
	float TimeSeconds,
	int32 Channel,
	int32 ExpectedBandCount,
	TArray<float>& OutBands);
}
