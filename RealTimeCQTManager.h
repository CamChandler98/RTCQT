// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "ConstantQAnalyzer.h"
#include "DSP/AlignedBuffer.h"
#include "DSP/ConstantQ.h"
#include "DSP/SlidingWindow.h"
#include "DSP/Filter.h"
#include "RealTimeCQTManager.generated.h"

USTRUCT(BlueprintType)
struct FUpdateData
{	    
		GENERATED_BODY();
		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		int index;

	   	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float value;
	};


UDELEGATE(BlueprintCallable)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpectrumUpdatedDelegate, FUpdateData, updateData);

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
	void PCMToFloat(const TArray<float>& PCMStream, TArray<float>& OutAmplitudes);
	void CQTProcessing();
	void AmplitudeSampleProcessing(TArray<float>& inAmplitude); 
	void OverlapAdd(const TArray<float> AudioData);

	void ApplyLowpassFilter(const TArray<float>& InSpectrum, float CutoffFrequency, float SampleRate, TArray<float>& OutSpectrum);
	TArray<float> combineStream(const TArray<uint8> interleavedStream, int numChannels);
	float ComputePolynomialCurve(float x, const TArray<float>& Coefficients);
	
	Audio::FConstantQAnalyzerSettings defaultSettings = Audio::FConstantQAnalyzerSettings();



	TUniquePtr<Audio::FConstantQAnalyzer> ConstantQAnalyzer;
	TUniquePtr<Audio::TSlidingBuffer<uint8> > SlidingBuffer;
	TUniquePtr<Audio::TSlidingBuffer<float> > SlidingFloatBuffer;

	Audio::FBiquadFilter BiquadFilter;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumHopFrames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	int32 NumHopSamples;

	int32 NumWindowSamples;

	UPROPERTY(BlueprintReadWrite ,EditAnywhere, Category = "RTCQT|FFT Settings")
	float sampleRate = 48000.0;
	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float deltaTime = 0.0;
	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float currentTime = 0.0;
	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float lastUpdateTime = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings" )
	int32 fftSize = 8192;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	int32 NumBands = 96;	
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	float NumBandsPerOctave = 24;	
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	float KernelLowestCenterFreq = 40.0 ;		
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	float BandWidthStretch = 1.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	float NoiseFloorDB = -60.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float cutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	int32 NumChannels = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumPaddingSamples = 16;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 bitsPerSample = 16;


	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|Output Arrays")
	TArray<float> outCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<float> currentCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<float> oldCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<float> outAmp;
		UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<float> inAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<float> ampFilter;



	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|CQTProcessing" )
	int32 smoothingWindowSize = 7;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0") , Category = "RTCQT|CQTProcessing")
	float InterpolationFactor = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float gainFactor = 1.5;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing" )
	float scaleMultiplier = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing" )
	float peakExponentMultiplier = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	bool doLowpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing")
	bool doSmooth = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "RTCQT|CQTProcessing")
	bool doNormalize = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "RTCQT|CQTProcessing")
	bool doClamp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|SampleProcessing" )
	bool doAbsAmp = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing")
	bool doScalePeaks = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doBitDirect = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool isFloatFormat = true;

	bool canProcesses = true;
    TArray<uint8> ChannelBuffer;


    TArray<uint8> WindowBuffer;
    TArray<uint8> TempBuffer;

    TArray<float> FloatWindowBuffer;
	TArray<float> FrameSpectrum;

	UPROPERTY(BlueprintAssignable);
	FOnSpectrumUpdatedDelegate OnSpectrumUpdatedEvent;


void FireOnSpectrumUpdatedEvent(const int index, const int value);

private:

};
