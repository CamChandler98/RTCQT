// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "./CoreDSP/ConstantQAnalyzer.h"
#include "./CoreDSP/ConstantQ.h"
#include "DSP/AlignedBuffer.h"
#include "DSP/SlidingWindow.h"
#include "DSP/Filter.h"
#include "PeakPicker.h"
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


	void PCMToFloat(const TArray<float>& PCMStream, TArray<float>& OutAmplitudes);
	void CQTProcessing();
	void FocusExponentiation(TArray<float>& InputArray, float ScalingFactor);
	void AmplitudeSampleProcessing(TArray<float>& inAmplitude); 
	void SmoothSignal(const TArrayView<float>& InSignal, TArray<float>& OutSignal, int32 WindowSize);
	void ScaleArray(TArray<float>& InputArray, float ScalingFactor);
	void GetCenterFrequencies();
	void GetSampleIndices();
	void FindDifference( const TArray<float>& Original, TArrayView<const float> Alter, float DifferenceThreshold);

	float getMaxValue(const TArray<float>& arr);
	float getMaxSpectrum();
	int getMaxValue(const TArray<int>& arr);

	float getMeanValue(const TArray<float>& arr);



	UFUNCTION(BlueprintCallable)
	void anaylze(TArray<uint8> byteArray);
	UFUNCTION(BlueprintCallable)
	void GetCQT();

	int32 FindClosestValue(const TArray<int32>& Array, int32 TargetValue);
	void ApplyLowpassFilter(const TArray<float>& InSpectrum, float CutoffFrequency, float SampleRate, TArray<float>& OutSpectrum);
	TArray<float> combineStream(const TArray<uint8> interleavedStream, int numChannels);
	float ComputePolynomialCurve(float x, const TArray<float>& Coefficients);
	int32 GetCOLAHopSizeForWindow(Audio::EWindowType InType, int32 WindowLength);

	Audio::FConstantQAnalyzerSettings defaultSettings = Audio::FConstantQAnalyzerSettings();
	Audio::FPeakPickerSettings PeakPickerSettings = Audio::FPeakPickerSettings();
	Audio::FFocusSettings focusSettings = Audio::FFocusSettings();


	TUniquePtr<Audio::FPeakPicker> PeakPicker;
	TUniquePtr<Audio::FConstantQAnalyzer> ConstantQAnalyzer;
	TUniquePtr<Audio::TSlidingBuffer<uint8> > SlidingBuffer;
	TUniquePtr<Audio::TSlidingBuffer<float> > SlidingFloatBuffer;

	Audio::FBiquadFilter LowPassFilter;
	Audio::FBiquadFilter HighPassFilter;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFT Settings")
	int32 NumHopFrames;

	UPROPERTY(BlueprintReadWrite, EditAnywhere )
	int32 NumHopSamples;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	int32 COLAHopSize;


	int32 NumWindowSamples;

	UPROPERTY(BlueprintReadWrite ,EditAnywhere, Category = "FFT Settings")
	float sampleRate = 48000.0;


	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float deltaTime = 0.0;
	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float currentTime = 0.0;
	UPROPERTY(BlueprintReadWrite ,EditAnywhere)
	float lastUpdateTime = 0.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings" )
	int32 fftSize = 8192;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	int32 NumBands = 96;	
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float NumBandsPerOctave = 24;	

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float KernelLowestCenterFreq = 40.0 ;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float KernelHighestCenterFreq = 9000.0 ;	

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float BandWidthStretch = 1.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float NoiseFloorDB = -60.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	int32 NumChannels = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	bool doStupid = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float FocusStart = 140.0;						
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float FocusMin = 2000.0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float FocusMax = 6000.0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	int32 NumPreMax = 1440;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	int32 NumPostMax = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	int32 NumPreMean = 4800;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	int32 NumPostMean = 4801;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	int32 NumWait = 1440;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	float MeanDelta = 0.07f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Peak Settings")
	bool doPeakDetection = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float LogNormal = 2.f ;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float LogFast = 2.5 ;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	float LogSlow = .975f;

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
	float gainFactor = 1.5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doLowpassFilter = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processing")
	bool doHighpassFilter = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sample Processing" )
	bool doAbsAmp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sample Processing" )
	bool doSmoothSignal = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing" )
	int32 smoothingWindowSize = 7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float scaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float sigmoidScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float peakExponentMultiplier = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float FocusExponentMultiplier = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietThreshold = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doSmooth = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doFocusExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doPeakExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doNormalize = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doScale = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doSurpressQuiet = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doClamp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0") , Category = "Spectrum Processing")
	float InterpolationFactor = .5;



	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 NumPaddingSamples = 16;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "FFT Settings")
	int32 bitsPerSample = 16;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Values")
	float MinCenterFreq = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Values")
	float MaxCenterFreq = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Values")
	int32 MaxSampleIndex = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Values")
	float MeanCenterFreq = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Values")
	float MaxSpectrum = 1.0;


	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Array")
	TArray<float> outCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> currentCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> oldCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> outAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> inAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> OriginalAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> TestAlterAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> TestOriginalAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> AlterAmp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<bool> FocusIndices;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> CenterFrequencies;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<int32> SampleIndices;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<int32> PeakIndices;




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


	



	UPROPERTY(BlueprintAssignable);
	FOnSpectrumUpdatedDelegate OnSpectrumUpdatedEvent;

	UPROPERTY(BlueprintAssignable);
	FOnGiveDifferenceDelegate OnGiveDifferenceEvent;

	UFUNCTION(CallInEditor, Category="Sample Processing" )
	void UpdateHighPassFilter();
	UFUNCTION(CallInEditor, Category="Sample Processing" )
	void UpdateLowPassFilter();


	void FireOnSpectrumUpdatedEvent(const int index, const float value);
	void FireOnGiveDifferenceUpdatedEvent(const int index);


private:

};
