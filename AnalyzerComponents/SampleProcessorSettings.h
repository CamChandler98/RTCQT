// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstantQNRT.h"
#include "SampleProcessorSettings.generated.h"

/**
 * 
 */

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API USampleToggles : public UObject
{
	GENERATED_BODY()
	public:
	
		USampleToggles();

		UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ShortName = "LowPass"))
		bool DoLowPass = false;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ShortName = "LowShelf"))
		bool DoLowShelf = false;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ShortName = "HighPass"))
		bool DoHighPass = false;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ShortName = "HighShelf"))
		bool DoHighShelf = false;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ShortName = "AbsAmp"))
		bool DoAbsAmp = false;


};

USTRUCT(BlueprintType)
struct FSampleToggleNames
{
	GENERATED_BODY();

	FName DoLowPassName = FName(TEXT("DoLowPass"));
	FName DoLowShelfName = FName(TEXT("DoLowShelf"));
	FName DoHighPassName = FName(TEXT("DoHighPass"));
	FName DoHighShelfName = FName(TEXT("DoHighShelf"));
	FName DoAbsAmpName = FName(TEXT("DoAbsAmp"));

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> Names;


	FSampleToggleNames()
	{
		Names.Add(DoLowPassName);
		Names.Add(DoLowShelfName);
		Names.Add(DoHighPassName);
		Names.Add(DoHighShelfName);
		Names.Add(DoAbsAmpName);
	}

};

USTRUCT(BlueprintType)
struct FSamplePropertyNames
{
	GENERATED_USTRUCT_BODY()
	
    
	FName HighPassCutoffFrequencyName = FName(TEXT("HighPassCutoffFrequency"));
	
	FName HighPassBandWidthName= FName(TEXT("HighPassBandWidth"));
	
	FName HighPassGainName= FName(TEXT("HighPassGain"));
	
	FName LowPassCutoffFrequencyName = FName(TEXT("LowPassCutoffFrequency"));
	
	FName LowPassBandWidthName= FName(TEXT("LowPassBandWidth"));
	
	FName LowPassGainName= FName(TEXT("LowPassGain"));
	
	FName GainFactorName = FName(TEXT("GainFactor"));

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FName> Names;

	FSamplePropertyNames()
	{
		Names.Add(HighPassCutoffFrequencyName);
		Names.Add(HighPassBandWidthName);
		Names.Add(HighPassGainName);
		Names.Add(LowPassCutoffFrequencyName);
		Names.Add(LowPassBandWidthName);
		Names.Add(LowPassGainName);
		Names.Add(GainFactorName);
	}

};


UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API USampleSettings : public UObject
{

GENERATED_BODY()
public:
	USampleSettings();

    UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float HighPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float HighPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float HighPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float LowPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float LowPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float LowPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	float GainFactor = 1.5;

};
