#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "RTCQTMusicLabControls.generated.h"
class ARTCQTMusicLabManager;
class UTextBlock;
class UButton;
class UCheckBox;
class URTCQTPaletteComponent;
class URTCQTPalettePresets;

UCLASS(Blueprintable)
class RTCQT_API URTCQTMusicLabControls : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Transient) TObjectPtr<ARTCQTMusicLabManager> Manager;
	/** Allows control extensions and verification to inspect the populated list. */
	UFUNCTION(BlueprintPure) UComboBoxString* GetTrackDropdown() const { return Dropdown; }
	UFUNCTION(BlueprintPure) UComboBoxString* GetPaletteDropdown() const { return PaletteDropdown; }
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
	friend class FRTCQTPaletteControlsTest;
	UFUNCTION() void Selected(FString Option, ESelectInfo::Type Type);
	UFUNCTION() void PlayPause();
	UFUNCTION() void Restart();
	UFUNCTION() void PaletteSelected(FString Option, ESelectInfo::Type Type);
	UFUNCTION() void PaletteNext();
	UFUNCTION() void PaletteReset();
	UFUNCTION() void PaletteEnabledChanged(bool bChecked);
	UFUNCTION() void ViewerPalettesChanged(bool bChecked);
	URTCQTPaletteComponent* GetPalette() const;
	void RefreshPaletteOptions();
	void RefreshPaletteState();
	void ShowPaletteResult(bool bAccepted);
	void ShowPaletteFeedback(const FString& Message, bool bAccepted);
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> Dropdown;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayLabel;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> PaletteDropdown;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> PaletteEnabled;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> ViewerPalettes;
	UPROPERTY(Transient) TObjectPtr<UButton> PaletteNextButton;
	UPROPERTY(Transient) TObjectPtr<UButton> PaletteResetButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PaletteStatus;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PaletteFeedback;
	TMap<FString, FString> OptionIds;
	TMap<FString, FName> PaletteOptionIds;
	TWeakObjectPtr<URTCQTPaletteComponent> CachedPalette;
	TWeakObjectPtr<URTCQTPalettePresets> CachedPresetAsset;
	int32 LastViewerPaletteCount = 0;
	FString LastPaletteError;
	double PaletteFeedbackUntil = 0;
	bool bPopulating = false;
	bool bUpdatingPaletteControls = false;
};
