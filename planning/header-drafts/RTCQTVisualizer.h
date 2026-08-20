#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTCQTTypes.h"

#include "RTCQTVisualizer.generated.h"

class UConstantQNRT;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class URTCQTResponseSettings;
class UStaticMesh;

/** Owns the deterministic analyzer-to-instanced-mesh runtime pipeline. */
UCLASS(BlueprintType)
class RTCQT_API ARTCQTVisualizer : public AActor
{
	GENERATED_BODY()

public:
	/** Creates the root and sole instanced-mesh component without enabling tick time. */
	ARTCQTVisualizer();

	/** Evaluates and applies one isolated visual frame at exactly TimeSeconds. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Evaluation")
	FRTCQTEvaluationResult EvaluateAtTime(float TimeSeconds);

	/** Recreates static instances only when layout inputs or expected count changed. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Layout")
	bool RebuildInstances();

	/** Removes every rendered instance and invalidates cached layout identity. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Layout")
	void ClearVisualization();

private:
	/** Owns every rendered band instance as one actor component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTCQT|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInstancedStaticMeshComponent> InstanceComponent;

	/** References the NRT analyzer that supplies normalized values, sound, and duration. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UConstantQNRT> ConstantQNRT;

	/** References the artist-authored D02 and visual-output settings asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<URTCQTResponseSettings> ResponseSettings;

	/** References the reusable geometry assigned to every band instance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> InstanceMesh;

	/** Optionally overrides the mesh material once for the whole component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Assets", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> InstanceMaterial;

	/** Stores the explicit D03 inputs used to generate local base transforms. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout", meta = (AllowPrivateAccess = "true"))
	FRTCQTLayoutSettings LayoutSettings;

	/** Defines both the static instance count and required analyzer result count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Analysis", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 ExpectedBandCount = 32;

	/** Selects the analyzer channel queried at the caller's exact song time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Analysis", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 AnalyzerChannel = 0;

	/** Remembers only the last reported failure to suppress repeated log spam. */
	ERTCQTFailureReason LastFailureReason = ERTCQTFailureReason::None;
};
