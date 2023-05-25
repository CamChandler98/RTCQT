// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DSP/Filter.h"

#include "../Widget/Utility/FloatPropertyInterface.h"
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

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",  meta = (ClampMin = "0.0", ClampMax = "20000.0"))
	float HighPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing" , meta = (ClampMin = "-5.0", ClampMax = "5.0"))
	float HighPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",  meta = (ClampMin = "-5.0", ClampMax = "100.0"))
	float HighPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing" ,meta = (ClampMin = "0.0", ClampMax = "20000.0"))
	float LowPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing" , meta = (ClampMin = "-5.0", ClampMax = "5.0")) 
	float LowPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",   meta = (ClampMin = "-5.0", ClampMax = "100.0"))
	float LowPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing",   meta = (ClampMin = "-5.0", ClampMax = "100.0"))
	float GainFactor = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doLowpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doHighpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doAbsAmp = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	TArray<UFloatPropertyInterface*> WidgetInterfaces;

    TObjectPtr<USampleSettings> Settings;

	TUniquePtr<Audio::FBiquadFilter> LowPassFilter;
	TUniquePtr<Audio::FBiquadFilter> HighPassFilter;
	TUniquePtr<Audio::FBiquadFilter> LowShelfFilter;
	TUniquePtr<Audio::FBiquadFilter> HighShelfFilter;


	void SetParams();

	void SetSettings(USampleSettings* InSettings);

	void GetFilters(float SampleRate);

	void ProcessAudio(TArray<float>& AudioData, FSampleToggles& Toggles);
};
