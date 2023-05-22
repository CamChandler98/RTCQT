// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstantQNRT.h"
#include "SampleProcessorSettings.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FSampleToggles
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoLowPass = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoLowShelf = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoHighPass = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoHighShelf = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoAbsAmp = false;

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
