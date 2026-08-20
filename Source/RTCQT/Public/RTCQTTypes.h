#pragma once

#include "CoreMinimal.h"

#include "RTCQTTypes.generated.h"

/** Selects the deterministic D03 placement contract for all band instances. */
UENUM(BlueprintType)
enum class ERTCQTLayoutMode : uint8
{
	Line UMETA(DisplayName = "Line"),
	Circle UMETA(DisplayName = "Circle"),
	FibonacciSphere UMETA(DisplayName = "Fibonacci Sphere")
};

/** Identifies why an isolated visualizer evaluation could not be completed. */
UENUM(BlueprintType)
enum class ERTCQTFailureReason : uint8
{
	None,
	MissingAnalyzer,
	MissingAnalyzedSound,
	MissingResponseSettings,
	MissingMesh,
	InvalidExpectedBandCount,
	InvalidTime,
	TimeOutsideAnalysis,
	EmptyAnalyzerResult,
	BandCountMismatch,
	LayoutGenerationFailed,
	InstanceUpdateFailed
};

/** Stores a bounded scalar output range consumed by D02 range mapping. */
USTRUCT(BlueprintType)
struct RTCQT_API FRTCQTScalarRange
{
	GENERATED_BODY()

	/** Creates the default normalized output range [0, 1]. */
	FRTCQTScalarRange() = default;

	/** Creates an authored output range whose bounds D02 will sanitize at use time. */
	FRTCQTScalarRange(float InMin, float InMax)
		: Min(InMin)
		, Max(InMax)
	{
	}

	/** Lower authored output bound; D02 sanitizes and orders it at use time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT")
	float Min = 0.0f;

	/** Upper authored output bound; D02 sanitizes and orders it at use time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT")
	float Max = 1.0f;
};

/** Stores the explicit D03 inputs shared by all deterministic layout modes. */
USTRUCT(BlueprintType)
struct RTCQT_API FRTCQTLayoutSettings
{
	GENERATED_BODY()

	/** Chooses the deterministic placement algorithm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout")
	ERTCQTLayoutMode Mode = ERTCQTLayoutMode::Line;

	/** Translates the generated layout in visualizer-local space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout")
	FVector Origin = FVector::ZeroVector;

	/** Supplies the sanitized local scale before audio response is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout")
	FVector BaseScale = FVector::OneVector;

	/** Sets line-layout center-to-center distance in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout", meta = (ClampMin = "0.0"))
	float Spacing = 100.0f;

	/** Sets circle or sphere center-to-instance radius in Unreal units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout", meta = (ClampMin = "0.0"))
	float Radius = 500.0f;
};

/** Reports the typed outcome of one exact-time visualizer evaluation. */
USTRUCT(BlueprintType)
struct RTCQT_API FRTCQTEvaluationResult
{
	GENERATED_BODY()

	/** States whether every required stage completed successfully. */
	UPROPERTY(BlueprintReadOnly, Category = "RTCQT|Evaluation")
	bool bSucceeded = false;

	/** Identifies the first failed stage, or None after success. */
	UPROPERTY(BlueprintReadOnly, Category = "RTCQT|Evaluation")
	ERTCQTFailureReason FailureReason = ERTCQTFailureReason::None;

	/** Records the exact time supplied by the caller. */
	UPROPERTY(BlueprintReadOnly, Category = "RTCQT|Evaluation")
	float EvaluatedTimeSeconds = 0.0f;

	/** Records the analyzer value count observed during the evaluation. */
	UPROPERTY(BlueprintReadOnly, Category = "RTCQT|Evaluation")
	int32 ObservedBandCount = 0;
};

/** Defines stable material custom-data offsets shared by runtime and material documentation. */
namespace RTCQTCustomData
{
	/** Stores the normalized D02 response. */
	inline constexpr int32 Response = 0;

	/** Stores band index mapped to [0, 1]. */
	inline constexpr int32 NormalizedBandIndex = 1;

	/** Reserves exactly the custom-data slots declared above. */
	inline constexpr int32 NumFloats = 2;
}
