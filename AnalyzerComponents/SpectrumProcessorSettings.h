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


	void SetToggle(FName InPropertyName, bool InValue);

};


USTRUCT(BlueprintType)
struct FSpectrumToggleNames
{
	GENERATED_BODY();

	
	FName DoInterpolateName = FName(TEXT("DoInterpolate"));
	
	FName DoCubicInterpolationName = FName(TEXT("DoCubicInterpolation"));
	
	FName DoSmoothName = FName(TEXT("DoSmooth"));
	
	FName DoFocusExpName = FName(TEXT("DoFocusExp"));
	
	FName DoPeakExpName = FName(TEXT("DoPeakExp"));
	
	FName DoNormalizeName = FName(TEXT("DoNormalize"));
	
	FName DoScaleName = FName(TEXT("DoScale"));
	
	FName DoSupressQuietName = FName(TEXT("DoSupressQuiet"));
	
	FName DoClampName = FName(TEXT("DoClamp"));

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	TArray<FName> Names;

	FSpectrumToggleNames()
	{
		Names.Add(DoInterpolateName);
		Names.Add(DoCubicInterpolationName);
		Names.Add(DoSmoothName);
		Names.Add(DoFocusExpName);
		Names.Add(DoPeakExpName);
		Names.Add(DoNormalizeName);
		Names.Add(DoScaleName);
		Names.Add(DoSupressQuietName);
		Names.Add(DoClampName);
	}

};


USTRUCT(BlueprintType)
struct FSpectrumPropertyNames
{
	GENERATED_USTRUCT_BODY()
	
	FName SmoothingWindowSizeName = FName(TEXT("SmoothingWindowSize"));
	
	FName InterpolationFactorName =  FName(TEXT("InterpolationFactor"));
	
	FName ScaleMultiplierName = FName(TEXT("ScaleMultiplier"));
	
	FName QuietMultiplierName = FName(TEXT("QuietMultiplier"));
	
	FName PeakExponentMultiplierName = FName(TEXT("PeakExponentMultiplier"));
	
	FName FocusExponentMultiplierName = FName(TEXT("FocusExponentMultiplier"));

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	TArray<FName> Names;

	FSpectrumPropertyNames()
	{
		Names.Add(SmoothingWindowSizeName);
		Names.Add(InterpolationFactorName);
		Names.Add(ScaleMultiplierName);
		Names.Add(QuietMultiplierName);
		Names.Add(PeakExponentMultiplierName);
		Names.Add(FocusExponentMultiplierName);
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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float PeakExponentMultiplier = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float FocusExponentMultiplier = 2;

	UFUNCTION(BlueprintCallable)
	void LogParams();
	

};
