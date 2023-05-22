// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ConstantQNRT.h"
#include "CQTSettings.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UCQTSettings : public UConstantQNRTSettings
{

GENERATED_BODY()
public:
	UCQTSettings();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = AudioAnalyzer, meta = (DisplayName = "Proportion", ClampMin = "0.0", ClampMax = "1.0"))
	float Proportion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = AudioAnalyzer )
	float SampleRate;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = AudioAnalyzer, meta = (DisplayName = "End Center Frequency"))
	float EndingFrequency;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = AudioAnalyzer, meta = (DisplayName = "FFT"))
	int32 UnrestrictedFFTSize;
};
