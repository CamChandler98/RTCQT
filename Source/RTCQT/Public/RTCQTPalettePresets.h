#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTCQTPalettePresets.generated.h"

USTRUCT(BlueprintType)
struct RTCQT_API FRTCQTPalettePreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	FLinearColor LowColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	FLinearColor HighColor = FLinearColor::White;
};

/** Editable linear RGB pairs; array order defines the local cycle order. */
UCLASS(BlueprintType)
class RTCQT_API URTCQTPalettePresets : public UDataAsset
{
	GENERATED_BODY()
public:
	URTCQTPalettePresets();
	bool ValidatePresets(FString& Error) const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTCQT|Palette")
	TArray<FRTCQTPalettePreset> Presets;
};
