// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CQTSettings.h"
#include "../CoreDSP/ConstantQAnalyzer.h"
#include "../CoreDSP/ConstantQ.h"
#include "RTCQTAnalyzer.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API URTCQTAnalyzer : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URTCQTAnalyzer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCQTSettings> ParameterSettings;

	TUniquePtr<Audio::FConstantQAnalyzer> ConstantQAnalyzer;

	Audio::FConstantQAnalyzerSettings ConstantQSettings = Audio::FConstantQAnalyzerSettings();


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 TotalBands = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CeilProportion = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<float> OutCQT;

	int32 GetNumBands(int32 BandTotal, float Proportion, bool doCiel);

	float GetBandsPerOctave(float BaseFrequency, float EndFrequency, int32 NumBands);
	
	void GenerateAnalyzer();

	void Analyze(TArray<float> AudioData);
	Audio::FConstantQAnalyzerSettings GetCQTSettings();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
