// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstantQNRT.h"
#include "SpectrumProcessorSettings.generated.h"

/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(SpectrumSettingsLog, Log, All);

USTRUCT(BlueprintType)
struct FSpectrumToggles
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool DoInterpolate = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool DoCubicInterpolation = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool DoSmooth = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool DoFocusExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool DoPeakExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool DoNormalize = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool DoScale = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool DoSupressQuiet = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool DoClamp = true;

};

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API USpectrumSettings : public UObject
{

GENERATED_BODY()
public:
	USpectrumSettings();
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing" )
	int32 SmoothingWindowSize = 7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0") , Category = "Spectrum Processing")
	float InterpolationFactor = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float ScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float PeakExponentMultiplier = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float FocusExponentMultiplier = 2;

	UFUNCTION(BlueprintCallable)
	void LogParams();
	

};
