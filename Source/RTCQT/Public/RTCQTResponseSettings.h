#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTCQTResponseProcessor.h"
#include "RTCQTTypes.h"

#include "RTCQTResponseSettings.generated.h"

/** Gain for bands whose center frequency falls in [MinHz, MaxHz). */
USTRUCT(BlueprintType)
struct RTCQT_API FRTCQTFrequencyWeight
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float MinHz = 20;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float MaxHz = 250;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="8")) float Gain = 1;
};

/** Stores artist-facing deterministic response and bounded visual-output settings. */
UCLASS(BlueprintType)
class RTCQT_API URTCQTResponseSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Creates the UObject-free parameter snapshot consumed by the response processor. */
	FRTCQTResponseParameters MakeProcessorParameters() const;

	/** Preview effective gains using the analyzer's frequency grid. Missing index weights are 1. */
	UFUNCTION(BlueprintPure, Category="RTCQT|Weights")
	TArray<float> ResolveBandWeights(int32 BandCount, float StartingFrequency, float BandsPerOctave) const;

	/** Defines the lower edge of the useful normalized analyzer window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Response", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NoiseFloor = 0.1f;

	/** Defines the upper edge of the useful normalized analyzer window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Response", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InputCeiling = 1.0f;

	/** Supplies optional per-band gain/focus weights aligned by band index. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Response")
	TArray<float> BandWeights;

	/** Portable Hz ranges; overlaps multiply. Empty means neutral gain at all frequencies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTCQT|Weights")
	TArray<FRTCQTFrequencyWeight> FrequencyWeights;

	/** Shapes the weighted normalized response using the fixed D02 operation order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Response", meta = (ClampMin = "0.001"))
	float ResponseExponent = 2.0f;

	/** Bounds the absolute local Z scale applied to instances by D11. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Output")
	FRTCQTScalarRange MeshZScaleRange = {0.1f, 4.0f};

	/** Bounds D12 emissive intensity; use-time limits sanitize to finite [0, 100]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Output")
	FRTCQTScalarRange EmissiveRange = {0.0f, 10.0f};
};
