#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTCQTPaletteComponent.generated.h"

class ARTCQTVisualizer;
class URTCQTPalettePresets;

/** Opt-in local palette transitions, independent of audio and pulse state. */
UCLASS(ClassGroup = (RTCQT), meta = (BlueprintSpawnableComponent))
class RTCQT_API URTCQTPaletteComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URTCQTPaletteComponent();

	UFUNCTION(BlueprintCallable, Category = "RTCQT|Palette")
	bool SelectPalette(FName Id);
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Palette")
	bool NextPalette();
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Palette")
	bool ResetPalette();
	UFUNCTION(BlueprintSetter, Category = "RTCQT|Palette")
	void SetPaletteEnabled(bool bNewEnabled);

	/** Disabled by default; changing this at runtime uses the same setup checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetPaletteEnabled, Category = "RTCQT|Palette")
	bool bPaletteEnabled = false;

	/** Optional artist-edited asset; unset uses the four built-in starter presets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	TObjectPtr<URTCQTPalettePresets> PresetAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	FName SelectedPaletteId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	bool bBlending = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	FString LastError;

	static constexpr double BlendDurationSeconds = 0.5;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class FRTCQTPaletteTransitionsTest;
	const URTCQTPalettePresets* GetPresets() const;
	ARTCQTVisualizer* GetVisualizer() const;
	bool CheckSetup();
	bool SelectPaletteAt(FName Id, double Now);
	bool ResetPaletteAt(double Now);
	bool StartBlend(FName Id, const FLinearColor& Low, const FLinearColor& High, bool bReset, double Now);
	bool AdvanceBlend(double Now);
	void StopBlend();

	UPROPERTY(Transient, DuplicateTransient)
	TWeakObjectPtr<ARTCQTVisualizer> OwnedVisualizer;

	double BlendStartedAt = 0;
	FLinearColor FromLow = FLinearColor::White;
	FLinearColor FromHigh = FLinearColor::White;
	FLinearColor TargetLow = FLinearColor::White;
	FLinearColor TargetHigh = FLinearColor::White;
	bool bResetOnCompletion = false;
};
