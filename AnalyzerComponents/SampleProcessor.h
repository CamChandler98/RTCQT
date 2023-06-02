// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DSP/Filter.h"
#include "DSP/FloatArrayMath.h"


#include "../Widget/Utility/FloatPropertyInterface.h"
#include "../Widget/Utility/BoolPropertyInterface.h"

#include "SampleProcessorSettings.h"
#include "SampleProcessor.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API USampleProcessor : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USampleProcessor();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",  meta = (ClampMin = "0.0", ClampMax = "20000.0", ShortName = "HPCutoff"))
	float HighPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing" , meta = (ClampMin = "-5.0", ClampMax = "5.0", ShortName = "HPBandWidth"))
	float HighPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",  meta = (ClampMin = "-5.0", ClampMax = "10.0", ShortName = "HPGain"))
	float HighPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing" ,meta = (ClampMin = "0.0", ClampMax = "20000.0", ShortName = "LPCutoff"))
	float LowPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing" , meta = (ClampMin = "-5.0", ClampMax = "5.0", ShortName = "LPBandWidth")) 
	float LowPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",   meta = (ClampMin = "-5.0", ClampMax = "10.0", ShortName = "LPGain"))
	float LowPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",   meta = (ClampMin = "-5.0", ClampMax = "10.0", ShortName = "Gain"))
	float GainFactor = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doLowpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doHighpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doAbsAmp = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	TArray<UFloatPropertyInterface*> WidgetInterfaces;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",  Instanced)
	TArray<UBoolPropertyInterface*> ToggleInterfaces;

    TObjectPtr<USampleSettings> Settings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processor",  Instanced)
	TObjectPtr<USampleToggles> Toggles;

	TUniquePtr<Audio::FBiquadFilter> LowPassFilter;
	TUniquePtr<Audio::FBiquadFilter> HighPassFilter;
	TUniquePtr<Audio::FBiquadFilter> LowShelfFilter;
	TUniquePtr<Audio::FBiquadFilter> HighShelfFilter;


	void SetParams();

	void SetSettings(USampleSettings* InSettings);

	void GetFilters(float SampleRate);

	void ProcessAudio(TArray<float>& AudioData);
};
