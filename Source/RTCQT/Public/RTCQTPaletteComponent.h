#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTCQTPalettePresets.h"
#include "RTCQTPaletteComponent.generated.h"

class ARTCQTVisualizer;
class URTCQTPalettePresets;
class URTCQTStreamerBotComponent;

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

	/** Viewer-only path; local selection/reset never consult this lock or cooldown. */
	UFUNCTION(BlueprintCallable, Category = "RTCQT|Palette")
	bool TryViewerPalette(const FString& Value);
	UFUNCTION(BlueprintSetter, Category = "RTCQT|Palette")
	void SetViewerPalettesEnabled(bool bNewEnabled);
	UFUNCTION(BlueprintPure, Category = "RTCQT|Palette")
	float GetViewerPaletteCooldownRemaining() const;
	/** Validated copies for local controls; an invalid catalog supplies no choices. */
	UFUNCTION(BlueprintPure, Category = "RTCQT|Palette")
	TArray<FRTCQTPalettePreset> GetAvailablePalettes() const;

	/** Disabled by default; changing this at runtime uses the same setup checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetPaletteEnabled, Category = "RTCQT|Palette")
	bool bPaletteEnabled = false;

	/** Blocks new viewer requests; a previously accepted color transition can finish. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetViewerPalettesEnabled, Category = "RTCQT|Palette")
	bool bViewerPalettesEnabled = true;

	/** Optional artist-edited asset; unset uses the four built-in starter presets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	TObjectPtr<URTCQTPalettePresets> PresetAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	FName SelectedPaletteId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	bool bBlending = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	FString LastError;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, DuplicateTransient, Category = "RTCQT|Palette")
	int32 AcceptedViewerPaletteCount = 0;

	static constexpr double BlendDurationSeconds = 0.5;
	static constexpr double ViewerCooldownSeconds = 15.0;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	friend class FRTCQTPaletteTransitionsTest;
	friend class FRTCQTPaletteViewerControlsTest;
	friend class FRTCQTPalettePlaybackPersistenceTest;
	friend class FRTCQTPaletteControlsTest;
	const URTCQTPalettePresets* GetPresets() const;
	ARTCQTVisualizer* GetVisualizer() const;
	URTCQTStreamerBotComponent* GetReceiver() const;
	void BindReceiver();
	void UnbindReceiver();
	UFUNCTION() void HandleChatCommand(const FString& Command, const FString& Value);
	bool TryViewerPaletteAt(const FString& Value, double Now);
	double ViewerCooldownRemainingAt(double Now) const;
	bool IsAlreadySelected(FName Id, const FLinearColor& Low, const FLinearColor& High) const;
	bool CheckSetup();
	bool SelectPaletteAt(FName Id, double Now);
	bool ResetPaletteAt(double Now);
	bool StartBlend(FName Id, const FLinearColor& Low, const FLinearColor& High, bool bReset, double Now);
	bool AdvanceBlend(double Now);
	void StopBlend();

	UPROPERTY(Transient, DuplicateTransient)
	TWeakObjectPtr<ARTCQTVisualizer> OwnedVisualizer;
	UPROPERTY(Transient, DuplicateTransient)
	TWeakObjectPtr<URTCQTStreamerBotComponent> BoundReceiver;

	double BlendStartedAt = 0;
	FLinearColor FromLow = FLinearColor::White;
	FLinearColor FromHigh = FLinearColor::White;
	FLinearColor TargetLow = FLinearColor::White;
	FLinearColor TargetHigh = FLinearColor::White;
	bool bResetOnCompletion = false;
	bool bHasAcceptedViewerPalette = false;
	double LastViewerAcceptedAt = 0;
};
