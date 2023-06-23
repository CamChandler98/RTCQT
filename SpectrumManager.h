// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DSP/SlidingWindow.h"
#include "DSP/AlignedBuffer.h"

#include "./AnalyzerComponents/RTCQTAnalyzer.h"
#include "./AnalyzerComponents/Sampler.h"
#include "./Widget/RadialSliderWidget.h"
#include "SpectrumManager.generated.h"


UCLASS()
class SYNRTCQT_API ASpectrumManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpectrumManager();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumBands = 96;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SampleRate = 48000;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float SampleConversionFactor = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float NumHopFrames = 480;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 BoundaryThreshold;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BoundarySmooth = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float ExecutionTime = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 FFTSize = 8192;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoContinousSpectrum = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoSpectrumProcessing = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool DoSampleProcessing = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<float> CompiledSpectrum;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap<int32, float> StartAndEnds;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<int32> BoundaryKeys;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	URadialSliderWidget* SliderWidget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TArray<URTCQTAnalyzer*> SpectrumAnalyzers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TMap<FName,URTCQTAnalyzer*> NamedAnalyzers;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TArray<UAnalyzerSettings*> AnalyzersSettings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced)
	TObjectPtr<USampler> Sampler;


	TUniquePtr<Audio::TSlidingBuffer<float>> SlidingFloatBuffer;
	TUniquePtr<Audio::TSlidingBuffer<float>> UnrealSlidingBuffer;

    TArray<float> FloatWindowBuffer;

	UFUNCTION(BlueprintCallable)
	void CreateAnalyzers();


	UFUNCTION(BlueprintCallable)
	void SmoothBoundary( TArray<float>& SpectrumData, int32 Index, int32 ThresholdWidth );

	UFUNCTION(BlueprintCallable)
	void SmoothSpectrum(TArray<float>& CurrentCQT);

	UFUNCTION(BlueprintCallable)
	void GetSampleConversionFactor(float InSampleRate);


	UFUNCTION(BlueprintCallable)
	TArray<int32> GetStartEndKeys();

	UFUNCTION(BlueprintCallable)
	void CheckLength();

	UFUNCTION(BlueprintCallable)
	void AnalyzeAudio(const TArray<float>& AudioData);

	UFUNCTION(BlueprintCallable)
	void UnrealAnalyzeAudio(const TArray<float>& AudioData);


	UPROPERTY(BlueprintAssignable);
	FOnSpectrumDelegate OnSpectrumUpdatedEvent;


	void FireOnSpectrumUpdatedEvent(const int Index, const float Value, const float Max);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	



};
