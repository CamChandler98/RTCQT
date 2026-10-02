#include "RTCQTAnalyzerQuery.h"

#include "ConstantQNRT.h"
#include "ConstantQNRTFactory.h"

FRTCQTEvaluationResult RTCQTAnalysis::QueryBandsAtTime(
	const UConstantQNRT* Analyzer,
	float TimeSeconds,
	int32 Channel,
	int32 ExpectedBandCount,
	TArray<float>& OutBands)
{
	OutBands.Reset();
	FRTCQTEvaluationResult Result;
	Result.EvaluatedTimeSeconds = TimeSeconds;
	const auto Fail = [&Result, &OutBands](ERTCQTFailureReason Reason)
	{
		Result.FailureReason = Reason;
		OutBands.Reset();
		return Result;
	};
	if (!FMath::IsFinite(TimeSeconds)) return Fail(ERTCQTFailureReason::InvalidTime);
	if (!Analyzer) return Fail(ERTCQTFailureReason::MissingAnalyzer);
	if (!Analyzer->Sound) return Fail(ERTCQTFailureReason::MissingAnalyzedSound);
	if (ExpectedBandCount <= 0) return Fail(ERTCQTFailureReason::InvalidExpectedBandCount);
	if (!FMath::IsFinite(Analyzer->DurationInSeconds) || Analyzer->DurationInSeconds < 0.0f
		|| TimeSeconds < 0.0f || TimeSeconds > Analyzer->DurationInSeconds)
	{
		return Fail(ERTCQTFailureReason::TimeOutsideAnalysis);
	}

	// Check native result metadata before Unreal's query: unavailable channels
	// otherwise produce an engine warning on every call, and null settings crash.
	const auto Analysis = Analyzer->GetResult<Audio::FConstantQNRTResult>();
	if (!Analyzer->Settings || !Analysis.IsValid() || !Analysis->IsSortedChronologically()
		|| !Analysis->ContainsChannel(Channel) || Analysis->GetFramesForChannel(Channel).IsEmpty())
	{
		return Fail(ERTCQTFailureReason::EmptyAnalyzerResult);
	}
	Analyzer->GetNormalizedChannelConstantQAtTime(TimeSeconds, Channel, OutBands);
	Result.ObservedBandCount = OutBands.Num();
	if (OutBands.IsEmpty()) return Fail(ERTCQTFailureReason::EmptyAnalyzerResult);
	if (OutBands.Num() != ExpectedBandCount) return Fail(ERTCQTFailureReason::BandCountMismatch);

	Result.bSucceeded = true;
	return Result;
}
