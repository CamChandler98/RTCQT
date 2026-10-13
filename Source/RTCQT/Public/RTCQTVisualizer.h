#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTCQTTypes.h"

#include "RTCQTVisualizer.generated.h"

class UConstantQNRT;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
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

	/** Song seconds assigned by a linear Sequencer property track; not a ticking clock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetSequenceTimeSeconds, Interp, Category = "RTCQT|Evaluation")
	float SequenceTimeSeconds = 0.0f;

	/** Evaluates every assigned sequence time, including repeated and backward seeks. */
	UFUNCTION(BlueprintSetter, Category = "RTCQT|Evaluation")
	void SetSequenceTimeSeconds(float TimeSeconds);

	/** Diagnostics from the latest sequence setter, without per-frame logging. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Evaluation")
	FRTCQTEvaluationResult LastSequenceEvaluation;

	/** Recreates static instances only when layout inputs or expected count changed. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Layout")
	bool RebuildInstances();

	/** Removes every rendered instance and invalidates cached layout identity. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Layout")
	void ClearVisualization();

	/** Atomically clears the prior frame and changes the playback analysis source. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Evaluation")
	void SetPlaybackAnalyzer(UConstantQNRT* Analyzer, int32 Channel = 0);

	/** Holds the authored line span when playback changes the number of bands. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTCQT|Layout")
	bool bKeepPlaybackLineWidth = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="RTCQT|Layout", meta=(ClampMin="2"))
	int32 PlaybackReferenceBandCount = 32;

	UFUNCTION(BlueprintPure, Category="RTCQT|Analysis")
	int32 GetBandCount() const { return ExpectedBandCount; }

protected:
	/** Refreshes the static preview after actor placement or an editor property change. */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Ensures loaded actors have valid topology when play begins. */
	virtual void BeginPlay() override;

private:
	FRTCQTLayoutSettings EffectiveLayout() const;
	bool bPlaybackDriven = false;
	/** Clears stale geometry and logs only when entering a different failure. */
	bool FailRebuild(ERTCQTFailureReason Reason, const TCHAR* Message);

	/** Removes topology without forgetting a continuing failure condition. */
	void ResetInstances();

	/** Reuses one component-wide material and refreshes bounded emissive parameters. */
	void RefreshMaterial();

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

	/** Runtime-only shared override; reconstructed independently in duplicated worlds. */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UMaterialInstanceDynamic> SharedMaterial;

	/** Stores the explicit D03 inputs used to generate local base transforms. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Layout", meta = (AllowPrivateAccess = "true"))
	FRTCQTLayoutSettings LayoutSettings;

	/** Defines both the static instance count and required analyzer result count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Analysis", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 ExpectedBandCount = 32;

	/** Selects the analyzer channel queried at the caller's exact song time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Analysis", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 AnalyzerChannel = 0;

	/** Remembers only the last reported failure to suppress repeated log spam. */
	ERTCQTFailureReason LastFailureReason = ERTCQTFailureReason::None;

	/** Caches only topology inputs; evaluated responses and song time are never cached. */
	FRTCQTLayoutSettings CachedLayoutSettings;
	int32 CachedBandCount = INDEX_NONE;
	bool bHasCachedLayout = false;

	/** Suppresses repeated warnings while the mesh's default material is in use. */
	bool bUsingDefaultMaterial = false;
};
