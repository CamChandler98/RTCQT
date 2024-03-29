// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/PropertyIterator.h"
#include "UObject/UnrealType.h"


#include "CQTSettings.h"
#include "SampleProcessorSettings.h"
#include "SampleProcessor.h"
#include "SpectrumProcessor.h"
#include "SampleProcessorSettings.h"
#include "../Widget/Utility/FloatPropertyInterface.h"
#include "../Widget/Utility/BoolPropertyInterface.h"
#include "../Widget/RadialSliderWidget.h"
#include "../CoreDSP/ConstantQAnalyzer.h"
#include "../CoreDSP/ConstantQ.h"

#include "RTCQTAnalyzer.generated.h"

USTRUCT(BlueprintType)
struct FSpectrumData
{	    
		GENERATED_BODY();


		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int Index;

	   	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float Value;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float Max;



		FSpectrumData()
		{
			Index = 0;
			Value = 0.0;
			Max = 0.0;
		}
};


UDELEGATE(BlueprintCallable)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpectrumDelegate, FSpectrumData, updateData);


UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UAnalyzerSettings : public UObject
{
	GENERATED_BODY()

	public:

	UAnalyzerSettings();
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TObjectPtr<UCQTSettings> CQTSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TObjectPtr<USampleSettings> SampleProcessorSettings;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TObjectPtr<USpectrumSettings> SpectrumProcessorSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TObjectPtr<USpectrumToggles> SpectrumToggles;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TObjectPtr<USampleToggles> SampleToggles;


};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API URTCQTAnalyzer : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URTCQTAnalyzer();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, instanced)
	TObjectPtr<UCQTSettings> ParameterSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 _NumBands;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float _NumBandsPerOctave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float _SFreq;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float _EFreq;


	TUniquePtr<Audio::FConstantQAnalyzer> ConstantQAnalyzer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processor")
	TObjectPtr<USampleProcessor> SampleProcessor; 

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processor")
	TObjectPtr<USpectrumProcessor> SpectrumProcessor;

	Audio::FConstantQAnalyzerSettings ConstantQSettings = Audio::FConstantQAnalyzerSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TotalBands = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CeilProportion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool BroadcastData = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<float> OutCQT; 


	int32 GetNumBands(int32 BandTotal, float Proportion, bool doCiel);

	float GetBandsPerOctave(const float BaseFrequency, const float EndFrequency, int32 NumBands);
	
	void GenerateAnalyzer();

	void Analyze(TArray<float> AudioData, bool bProcessSamples, bool bProcessSpectrum);

	Audio::FConstantQAnalyzerSettings GetCQTSettings();


	void GetSpectrumProcessor(USpectrumSettings* InSettings, USpectrumToggles* InToggles, FName InName);

	void GetSampleProcessor(USampleSettings* InSettings, USampleToggles* InToggles, FName InName);

	void GetParams(UCQTSettings* InSettings);


	TObjectPtr<FBoolProperty> GetBoolPropertyFName(FName InPropertyName);

	void SetSpectrumToggle(FName InPropertyName, bool InValue);

	void GetNumericWidgets(USampleProcessor* InProcessor, FName InName);
	void GetNumericWidgets(USpectrumProcessor*  InProcessor, FName InName);

	void GetToggleInterfaces(USpectrumProcessor* InProcessor, FName InName);
	void GetToggleInterfaces(USampleProcessor* InProcessor, FName InName);

	UPROPERTY(BlueprintAssignable);
	FOnSpectrumDelegate OnSpectrumUpdatedEvent;


	void FireOnSpectrumUpdatedEvent(const int Index, const float Value, const float Max);





protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
