// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstantQNRT.h"
#include "SpectrumProcessorSettings.generated.h"

/**
 * 
 */

DECLARE_LOG_CATEGORY_EXTERN(SpectrumSettingsLog, Log, All);

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API USpectrumToggles : public UObject
{
	GENERATED_BODY()
	public:
		USpectrumToggles();
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Interpolate"))
		bool DoInterpolate = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "CubicInterp"))
		bool DoCubicInterpolation = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Smooth"))
		bool DoSmooth = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Focus"))
		bool DoFocusExp = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Peak"))
		bool DoPeakExp = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Norm"))
		bool DoNormalize = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Scale"))
		bool DoScale = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "PostScale")) 
		bool DoPostScale = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Quiet"))
		bool DoSupressQuiet = true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "HiBoost"))
		bool DoBoostHighFrequency= true;
		UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Median")) 
		bool DoMedianSmooth = false;



};


USTRUCT(BlueprintType)
struct FSpectrumToggleNames
{
	GENERATED_BODY();

	
	FName DoInterpolateName = FName(TEXT("DoInterpolate"));
	
	FName DoCubicInterpolationName = FName(TEXT("DoCubicInterpolation"));
	
	FName DoSmoothName = FName(TEXT("DoSmooth"));

	FName DoMedianSmoothName = FName(TEXT("DoMedianSmooth"));
	
	FName DoFocusExpName = FName(TEXT("DoFocusExp"));
	
	FName DoPeakExpName = FName(TEXT("DoPeakExp"));
	
	FName DoNormalizeName = FName(TEXT("DoNormalize"));
	
	FName DoScaleName = FName(TEXT("DoScale"));
	FName DoPostScaleName = FName(TEXT("DoPostScale"));

	
	FName DoSupressQuietName = FName(TEXT("DoSupressQuiet"));
	
	FName DoBoostHighFrequency = FName(TEXT("DoBoostHighFrequency"));

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	TArray<FName> Names;

	FSpectrumToggleNames()
	{
		Names.Add(DoInterpolateName);
		Names.Add(DoCubicInterpolationName);
		Names.Add(DoSmoothName);
		Names.Add(DoMedianSmoothName);
		Names.Add(DoFocusExpName);
		Names.Add(DoPeakExpName);
		Names.Add(DoNormalizeName);
		Names.Add(DoScaleName);
		Names.Add(DoPostScaleName);
		Names.Add(DoSupressQuietName);
		Names.Add(DoBoostHighFrequency);
	}

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

	UFUNCTION(BlueprintCallable)
	void LogParams();
	

};


USTRUCT(BlueprintType)
struct FSpectrumPropertyNames
{
	GENERATED_USTRUCT_BODY()
	
	FName SmoothingWindowSizeName = FName(TEXT("SmoothingWindowSize"));
	
	FName InterpolationFactorName =  FName(TEXT("InterpolationFactor"));
	
	FName ScaleMultiplierName = FName(TEXT("ScaleMultiplier"));
	FName PostScaleMultiplierName = FName(TEXT("PostScaleMultiplier"));
	
	FName QuietMultiplierName = FName(TEXT("QuietMultiplier"));
	
	FName PeakExponentMultiplierName = FName(TEXT("PeakExponentMultiplier"));
	
	FName FocusExponentMultiplierName = FName(TEXT("FocusExponentMultiplier"));

	FName HighFrequencyBoostFactorName = FName(TEXT("HighFrequencyBoostFactor"));

	FName QuietThresholdName = FName(TEXT("QuietThreshold"));

	


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	TArray<FName> Names;

	FSpectrumPropertyNames()
	{
		Names.Add(SmoothingWindowSizeName);
		Names.Add(InterpolationFactorName);
		Names.Add(ScaleMultiplierName);
		Names.Add(PostScaleMultiplierName);
		Names.Add(QuietMultiplierName);
		Names.Add(PeakExponentMultiplierName);
		Names.Add(FocusExponentMultiplierName);
		Names.Add(HighFrequencyBoostFactorName);
		Names.Add(QuietThresholdName);

	}

};


