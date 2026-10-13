#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "RTCQTMusicLabControls.generated.h"
class ARTCQTMusicLabManager;
class UTextBlock;

UCLASS(Blueprintable)
class RTCQT_API URTCQTMusicLabControls : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, Transient) TObjectPtr<ARTCQTMusicLabManager> Manager;
	/** Allows control extensions and verification to inspect the populated list. */
	UFUNCTION(BlueprintPure) UComboBoxString* GetTrackDropdown() const { return Dropdown; }
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
private:
	UFUNCTION() void Selected(FString Option, ESelectInfo::Type Type);
	UFUNCTION() void PlayPause();
	UFUNCTION() void Restart();
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> Dropdown;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PlayLabel;
	TMap<FString, FString> OptionIds;
	bool bPopulating = false;
};
