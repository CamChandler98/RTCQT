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

		FUpdateData()
		{
			index = 0;
			value = 0.0 ;
		}
	};


UDELEGATE(BlueprintCallable)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpectrumUpdatedDelegate, FUpdateData, updateData);


UDELEGATE(BlueprintCallable)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGiveDifferenceDelegate, int, diffData);


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
	void SmoothSignal(const TArrayView<float>& InSignal, TArray<float>& OutSignal, int32 WindowSize);
	void ScaleArray(TArray<float>& InputArray, float ScalingFactor);
	void GetCenterFrequencies();
	void GetSampleIndices();
	int32 FindDifference(const TArray<float> Original, const TArray<float> Alter); 
	UFUNCTION(BlueprintCallable)
	void GetCQT();
	int32 FindClosestValue(const TArray<int32>& Array, int32 TargetValue);
	void ApplyLowpassFilter(const TArray<float>& InSpectrum, float CutoffFrequency, float SampleRate, TArray<float>& OutSpectrum);
	TArray<float> combineStream(const TArray<uint8> interleavedStream, int numChannels);
	float ComputePolynomialCurve(float x, const TArray<float>& Coefficients);
	


	Audio::FConstantQAnalyzerSettings defaultSettings = Audio::FConstantQAnalyzerSettings();



	TUniquePtr<Audio::FConstantQAnalyzer> ConstantQAnalyzer;
	TUniquePtr<Audio::TSlidingBuffer<uint8> > SlidingBuffer;
	TUniquePtr<Audio::TSlidingBuffer<float> > SlidingFloatBuffer;

	Audio::FBiquadFilter LowPassFilter;
	Audio::FBiquadFilter HighPassFilter;



	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumHopFrames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|FFT Settings")
	int32 NumHopSamples;

	int32 NumWindowSamples;

	UPROPERTY(BlueprintReadWrite ,EditAnywhere, Category = "RTCQT|FFT Settings")
	float sampleRate = 48000.0;

	UPROPERTY(BlueprintReadWrite ,EditAnywhere, Category = "RTCQT|FFT Settings")
	float NumPeriodFrames = 128;

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
	float HighPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float HighPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float HighPassGain= 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float LowPassCutoffFrequency = 100.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float LowPassBandWidth= 2.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	float LowPassGain= 0.0;

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
	float QuietMultiplier = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing" )
	float peakExponentMultiplier = 2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	bool doLowpassFilter = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "RTCQT|SampleProcessing")
	bool doHighpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing")
	bool doSmooth = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "RTCQT|CQTProcessing")
	bool doNormalize = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "RTCQT|CQTProcessing")
	bool doSurpressQuiet = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "RTCQT|CQTProcessing")
	bool doClamp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|SampleProcessing" )
	bool doAbsAmp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|SampleProcessing" )
	bool doSmoothSignal = true;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|CQTProcessing")
	bool doScalePeaks = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doBitDirect = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool isFloatFormat = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool doAltFloat = true;

	bool canProcesses = true;
    TArray<uint8> ChannelBuffer;


    TArray<uint8> WindowBuffer;
    TArray<uint8> TempBuffer;

    TArray<float> FloatWindowBuffer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<float> CenterFrequencies;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTCQT|Output Arrays" )
	TArray<int32> SampleIndices;
	



	UPROPERTY(BlueprintAssignable);
	FOnSpectrumUpdatedDelegate OnSpectrumUpdatedEvent;

	UPROPERTY(BlueprintAssignable);
	FOnGiveDifferenceDelegate OnGiveDifferenceEvent;

	UFUNCTION(CallInEditor, Category="RTCQT|SampleProcessing" )
	void UpdateHighPassFilter();
	UFUNCTION(CallInEditor, Category="RTCQT|SampleProcessing" )
	void UpdateLowPassFilter();


	void FireOnSpectrumUpdatedEvent(const int index, const int value);
	void FireOnGiveDifferenceUpdatedEvent(const int index);


private:

};
