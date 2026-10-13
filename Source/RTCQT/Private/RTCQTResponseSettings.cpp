#include "RTCQTResponseSettings.h"

FRTCQTResponseParameters URTCQTResponseSettings::MakeProcessorParameters() const
{
	FRTCQTResponseParameters Parameters;
	Parameters.NoiseFloor = NoiseFloor;
	Parameters.InputCeiling = InputCeiling;
	Parameters.ResponseExponent = ResponseExponent;
	Parameters.BandWeights = BandWeights;
	return Parameters;
}

TArray<float> URTCQTResponseSettings::ResolveBandWeights(int32 BandCount, float StartingFrequency, float BandsPerOctave) const
{
	TArray<float> Result;
	if (BandCount < 1 || BandCount > 96 || !FMath::IsFinite(StartingFrequency) || StartingFrequency <= 0
		|| !FMath::IsFinite(BandsPerOctave) || BandsPerOctave <= 0) return Result;
	for (int32 Index = 0; Index < BandCount; ++Index)
	{
		const double Hz = StartingFrequency * FMath::Pow(2.0, double(Index) / BandsPerOctave);
		double Gain = BandWeights.IsValidIndex(Index)
			? (FMath::IsFinite(BandWeights[Index]) ? FMath::Max(0.f, BandWeights[Index]) : 0.f) : 1.0;
		for (const auto& Range : FrequencyWeights)
		{
			if (!FMath::IsFinite(Range.MinHz) || !FMath::IsFinite(Range.MaxHz)
				|| Range.MinHz < 0 || Range.MaxHz <= Range.MinHz || !FMath::IsFinite(Range.Gain)) continue;
			if (Hz >= Range.MinHz && Hz < Range.MaxHz)
				Gain = FMath::Min(double(TNumericLimits<float>::Max()), Gain * FMath::Clamp(double(Range.Gain), 0.0, 8.0));
		}
		Result.Add(float(Gain));
	}
	return Result;
}
