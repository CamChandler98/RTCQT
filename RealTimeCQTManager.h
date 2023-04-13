// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConstantQAnalyzer.h"
#include "DSP/AlignedBuffer.h"
#include "DSP/ConstantQ.h"
#include "DSP/SlidingWindow.h"
#include "DSP/EnvelopeFollower.h"
#include "RealTimeCQTManager.generated.h"


UCLASS()
class SYNRTCQT_API ARealTimeCQTManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARealTimeCQTManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void anaylze(TArray<uint8> byteArray);
	void PCMToFloat(const TArray<uint8>& PCMStream, TArray<float>& OutAmplitudes);
	void PCMToAmplitude(const TArray<uint8>& PCMStream, TArray<float>& OutAmplitudes);
	void cqtProcessing(const TArray<float> audioData);
	TArray<float> combineStream(TArray<uint8> interleavedStream, int numChannels, int bitsPerSample);
	float ComputePolynomialCurve(float x, const TArray<float>& Coefficients);
	
	Audio::FConstantQAnalyzerSettings defaultSettings = Audio::FConstantQAnalyzerSettings();



	TUniquePtr<Audio::FConstantQAnalyzer> ConstantQAnalyzer;
	TUniquePtr<Audio::TSlidingBuffer<uint8> > SlidingBuffer;
	TUniquePtr<Audio::TSlidingBuffer<float> > SlidingFloatBuffer;
	TUniquePtr<Audio::FAttackReleaseSmoother> Smoother;
	Audio::FInlineEnvelopeFollower Envelop;


	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float sampleRate = 176400.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 fftSize = 8192;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float NumBandsPerOctave = 12;	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float KernelLowestCenterFreq = 40.0 ;		
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float BandWidthStretch = 1.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float NoiseFloorDB = -60.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<float> outCQT;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumChannels = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumPaddingSamples = 1024;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 initWindowSize = 7;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float analysisPeriod = 0.1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float peakExponentMultiplier = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<float> outAmp;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doSmooth = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doNormalize = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doClamp = true;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doScalePeaks = true;


    TArray<float> smoothAmplitudes;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumHopFrames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumHopSamples;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumWindowSamples;


private:

};
