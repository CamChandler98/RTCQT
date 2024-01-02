// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "../AnalyzerComponents/SpectrumProcessorSettings.h"
#include "../AnalyzerComponents/SpectrumProcessor.h"
#include "SaveSpectrumConfig.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FSpectrumAnalyzerSettings
{
	GENERATED_BODY();
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing" )
	int32 SmoothingWindowSize = 7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0") , Category = "Spectrum Processing")
	float InterpolationFactor = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float ScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing",  meta = (ClampMin = 0.0, ClampMax = 8.0, ShortName = "PostScale"))
	float PostScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float PeakExponentMultiplier = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float HighFrequencyBoostFactor = .5 ;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float FocusExponentMultiplier = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietThreshold = .5;
};

UCLASS(Blueprintable)
class SYNRTCQT_API USaveSpectrumConfig : public USaveGame
{
	GENERATED_BODY()
	public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame)
	TMap<FName, FSpectrumAnalyzerSettings> SpectrumSettings;

	UFUNCTION(BlueprintCallable)
	void AddSettings(FName SettingsName, FSpectrumAnalyzerSettings NewSettings);
	
	UFUNCTION(BlueprintCallable)
	void Empty();
	
	UFUNCTION(BlueprintCallable)
	void SaveSettingsFromProcessor(FName SettingsName, USpectrumProcessor* SpectrumProcessor);

	UFUNCTION(BlueprintCallable)
	void LoadSettingsFromProcessor(FName SettingsName, USpectrumProcessor* SpectrumProcessor);

	// UFUNCTION(BlueprintCallable)
	// USpectrumSettings* GetSetting(FName SettingName);
};
