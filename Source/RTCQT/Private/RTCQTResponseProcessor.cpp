#include "RTCQTResponseProcessor.h"

namespace
{
double FiniteOr(float Value, double Fallback)
{
	return FMath::IsFinite(Value) ? static_cast<double>(Value) : Fallback;
}

double UnitInterval(float Value, double Fallback)
{
	return FMath::Clamp(FiniteOr(Value, Fallback), 0.0, 1.0);
}
}

float FRTCQTResponseProcessor::ProcessBand(
	float Input,
	const FRTCQTResponseParameters& Parameters,
	int32 BandIndex)
{
	const double X = FiniteOr(Input, 0.0);
	const double Floor = UnitInterval(Parameters.NoiseFloor, 0.0);
	const double Ceiling = UnitInterval(Parameters.InputCeiling, 1.0);
	if (Ceiling <= Floor)
	{
		return 0.0f;
	}

	// Promote before subtraction/division: tiny windows and extreme float samples
	// can overflow a float intermediate even though the final response is bounded.
	const double Normalized = FMath::Clamp((X - Floor) / (Ceiling - Floor), 0.0, 1.0);
	const double Weight = Parameters.BandWeights.IsValidIndex(BandIndex)
		? FMath::Max(FiniteOr(Parameters.BandWeights[BandIndex], 0.0), 0.0) : 1.0;
	const double Exponent = FMath::IsFinite(Parameters.ResponseExponent) && Parameters.ResponseExponent > 0.0f
		? static_cast<double>(Parameters.ResponseExponent) : 1.0;
	const double Weighted = FMath::Clamp(Normalized * Weight, 0.0, 1.0);
	return static_cast<float>(FMath::Clamp(FMath::Pow(Weighted, Exponent), 0.0, 1.0));
}

void FRTCQTResponseProcessor::ProcessBands(
	TConstArrayView<float> Inputs,
	const FRTCQTResponseParameters& Parameters,
	TArray<float>& OutResponses)
{
	// Keep input views and parameter weights valid until every band is read,
	// including when the caller reuses either array as the output destination.
	TArray<float> Responses;
	Responses.Reserve(Inputs.Num());
	for (int32 Index = 0; Index < Inputs.Num(); ++Index)
	{
		Responses.Add(ProcessBand(Inputs[Index], Parameters, Index));
	}
	OutResponses = MoveTemp(Responses);
}

float FRTCQTResponseProcessor::MapResponse(float Response, float OutputMin, float OutputMax)
{
	const double Alpha = UnitInterval(Response, 0.0);
	double Min = FiniteOr(OutputMin, 0.0);
	double Max = FiniteOr(OutputMax, 1.0);
	if (Min > Max)
	{
		Swap(Min, Max);
	}

	// The convex form also preserves endpoints when bounds have very different
	// magnitudes; float (Max - Min) could overflow for opposite signed extremes.
	const double Mapped = FMath::LerpStable(Min, Max, Alpha);
	return static_cast<float>(FMath::Clamp(Mapped, Min, Max));
}
