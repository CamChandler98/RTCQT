// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimeCQTManager.h"
#include "ConstantQAnalyzer.h"
#include "DSP/ConstantQ.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/DeinterleaveView.h"
#include "SampleBuffer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ARealTimeCQTManager::ARealTimeCQTManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	



}  
// Called when the game starts or when spawned
void ARealTimeCQTManager::BeginPlay()
{
	Super::BeginPlay();

	defaultSettings.BandWidthStretch = BandWidthStretch;
	defaultSettings.NumBandsPerOctave = NumBandsPerOctave;
	defaultSettings.KernelLowestCenterFreq = KernelLowestCenterFreq;
	defaultSettings.FFTSize = fftSize ;
	defaultSettings.WindowType = Audio::EWindowType::Hann;
    defaultSettings.NumBands = NumBands;
    NumHopFrames = FMath::Max(1,NumHopSamples);
    NumHopSamples = NumHopFrames * NumChannels;
    NumWindowSamples = fftSize * NumChannels;


    
    outCQT.AddZeroed(NumBands);
    CenterFrequencies.AddZeroed(NumBands);
    SampleIndices.AddZeroed(NumBands);

    GetCenterFrequencies();
    GetSampleIndices();

	ConstantQAnalyzer = MakeUnique<Audio::FConstantQAnalyzer>(defaultSettings, sampleRate);
    SlidingBuffer = MakeUnique<Audio::TSlidingBuffer<uint8> >(NumWindowSamples + NumPaddingSamples,NumHopSamples);
    SlidingFloatBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(fftSize, NumHopFrames);
    HighPassFilter.Init(sampleRate, 1, Audio::EBiquadFilter::Highpass, HighPassCutoffFrequency, HighPassBandWidth, HighPassGain );
    LowPassFilter.Init(sampleRate, 1, Audio::EBiquadFilter::Lowpass, LowPassCutoffFrequency , LowPassBandWidth, LowPassGain );
}

// Called every frame
void ARealTimeCQTManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void ARealTimeCQTManager::GetCQT()
{
        for(int32 i = 0; i < outCQT.Num(); i ++ ){

            FireOnSpectrumUpdatedEvent(i,outCQT[i]);
        
        }
}
void ARealTimeCQTManager::anaylze(TArray<uint8> byteArray)
{
    if(canProcesses){
    const int32 SampleSize = sizeof(int32); // 16-bit PCM samples

    const int32 SamplesPerChannel = byteArray.Num() / (NumChannels * SampleSize);
    const int32 PaddedSamplesPerChannel = SamplesPerChannel + NumPaddingSamples;

    TArray<uint8> PaddedPCMStream;
    PaddedPCMStream.Reserve(PaddedSamplesPerChannel * NumChannels * SampleSize);
    PaddedPCMStream.AddZeroed(NumPaddingSamples * NumChannels * SampleSize);
    PaddedPCMStream.Append(byteArray);

    TArray<float> floatStream= combineStream(PaddedPCMStream, 2);

    PCMToFloat(floatStream, inAmp);
    }
}

TArray<float>  ARealTimeCQTManager::combineStream(const TArray<uint8> interleavedStream, int numChannels)
{
    canProcesses = false;
	UE_LOG(LogTemp, Warning, TEXT("The length of the stream is: %d"), interleavedStream.Num());
     // Compute the size of each sample in bytes
    int sampleSize = bitsPerSample / 8;

    // Compute the number of samples in the interleaved stream
    int numSamples = interleavedStream.Num() / (numChannels * sampleSize);

    // Reserve space for the output array
    TArray<float> summedSamples;
    summedSamples.Reserve(numSamples);

    // Sum the samples for each channel
    for (int i = 0; i < numSamples; i++)
    {
        // Compute the offset of the current sample
        int sampleOffset = i * numChannels * sampleSize;

        // Sum the sample values for each channel
        float sampleSum = 0.f;
        for (int j = 0; j < numChannels; j++)
        {
            // Compute the offset of the current channel within the sample
            int channelOffset = j * sampleSize;

            // Extract the sample value for the current channel
            if(isFloatFormat)
            {
                uint8 sampleValue = 0;
                for (int k = 0; k < sampleSize; k++)
                {
                    sampleValue += interleavedStream[sampleOffset + channelOffset + k] << (8 * k);
                }

                // Convert the uint8 sample value to float and normalize it to the range [-1, 1]
                float normalizedSampleValue = static_cast<float>(sampleValue) / 127.5f - 1.0f;
                float absSampleValue = FMath::Abs(normalizedSampleValue);


                // Add the sample value for the current channel to the sum
                sampleSum += normalizedSampleValue ;
            }
            else
            {

                uint8 sampleValue = 0;
                for (int k = 0; k < sampleSize; k++)
                {
                    sampleValue += interleavedStream[sampleOffset + channelOffset + k] << (8 * k);
                }

                // Normalize the sample value to the range [-1, 1]
                // sampleValue /= ((1 << (bitsPerSample - 1)) - 1);

                float normalizedSampleValue = static_cast<float>(sampleValue) / 127.5f - 1.0f;

                // Add the sample value for the current channel to the sum

                sampleSum +=  sampleValue;

              
                
            }
        }

        // Store the sum of the sample values for all channels in the output array
        summedSamples.Add(sampleSum/numChannels);
    }

    // Return the summed samples
	//(LogTemp, Warning, TEXT("The length is: %d"), summedSamples.Num());
    if(doSmoothSignal)
        {

            const TArrayView< float> SampleView(summedSamples.GetData(), summedSamples.Num());
            float avg = Audio::ArrayGetAverageValue(SampleView);
            if(avg > SMALL_NUMBER){
                
                SmoothSignal(SampleView, summedSamples, summedSamples.Num()/4);
            }
        }
    return summedSamples;

}

float ARealTimeCQTManager::ComputePolynomialCurve(float x, const TArray<float>& Coefficients)
{
    const int32 NumCoefficients = Coefficients.Num();
    float y = 0.0f;

    for (int32 i = 0; i < NumCoefficients; i++)
    {
        y += Coefficients[i] * FMath::Pow(x, i);
    }

    return y;
}


void ARealTimeCQTManager::PCMToFloat(const TArray<float>& PCMStream, TArray<float>& OutAmplitudes)
{


    TArray<float> OutSpectrum;
    OutSpectrum.AddZeroed(NumBands);


    // Iterate over sliding windows
    for (const TArray<float>& Window : Audio::TAutoSlidingWindow<float>(*SlidingFloatBuffer, PCMStream , FloatWindowBuffer, false))
    {   

        inAmp = Window;
        AmplitudeSampleProcessing(inAmp);


        ConstantQAnalyzer -> CalculateCQT(inAmp.GetData(), outCQT);
        CQTProcessing();
        for(int32 i = 0; i < outCQT.Num(); i ++ ){

            FireOnSpectrumUpdatedEvent(i,outCQT[i]);
        
        }
    }
        canProcesses = true;

}
void ARealTimeCQTManager::AmplitudeSampleProcessing(TArray<float>& inAmplitude ){

        if(doAbsAmp)
        {

            Audio::ArrayAbsInPlace(inAmplitude);
        }
        if(doLowpassFilter)
        {
            TArrayView<const float> AmpView(inAmplitude.GetData(), inAmplitude.Num());
            LowPassFilter.ProcessAudio(AmpView.GetData(), AmpView.Num(), inAmplitude.GetData());      
        }
        if(doHighpassFilter)
        {
            TArrayView<const float> AmpView(inAmplitude.GetData(), inAmplitude.Num());
            const TArray<float> TestView = inAmplitude;

            HighPassFilter.ProcessAudio(AmpView.GetData(), AmpView.Num(), inAmplitude.GetData());

            int32 DiffIndex = FindDifference(TestView, inAmplitude);
            UE_LOG(LogTemp, Warning, TEXT("DiffIndex %d"),  DiffIndex);
            if(DiffIndex > -1){
                int32 IndexInSpectrum = FindClosestValue(SampleIndices, DiffIndex);
            UE_LOG(LogTemp, Warning, TEXT("SpecIndex %d"),  IndexInSpectrum);
                FireOnGiveDifferenceUpdatedEvent(IndexInSpectrum);
            }
            else
            {
                FireOnGiveDifferenceUpdatedEvent(DiffIndex);

            }

        }


        Audio::ArrayMultiplyByConstantInPlace(inAmplitude, gainFactor);
        Audio::ArrayMultiplyByConstantInPlace(inAmplitude, 1.f / FMath::Sqrt(static_cast<float>(2))); 



}
void ARealTimeCQTManager::CQTProcessing()
{
    if(doLowpassFilter)
    {
        // TArrayView<const float> lowFliter(outCQT.GetData(), outCQT.Num());
        // LowPassFilter.ProcessAudioFrame(lowFliter.GetData(), outCQT.GetData());
    }
    currentCQT = outCQT;
    const int32 NumBins = oldCQT.Num();

// Interpolate between the two spectra
    for (int32 BinIndex = 0; BinIndex < NumBins; BinIndex++)
    {
        // Calculate the difference between the two bins
        const float BinDiff = FMath::Abs(currentCQT[BinIndex] - oldCQT[BinIndex]);

        // Calculate the interpolation factor for this bin
        const float BinInterpFactor = FMath::Clamp(BinDiff * InterpolationFactor, 0.f, 1.f);

                float oldValue;
                float newValue; 
                if (BinIndex > 0) 
                {
                 // Access the element at i-1 if i > 0
                    oldValue = oldCQT[BinIndex - 1];
                } 
                else {
                // Handle the case when i == 0
                    oldValue = oldCQT[BinIndex]/2; // or some other value
                }
                if (BinIndex == currentCQT.Num() - 1) 
                {
                 // Access the element at i-1 if i > 0
                    newValue = currentCQT[BinIndex]/2;
                } 
                else {
                // Handle the case when i == 0
                    newValue = currentCQT[BinIndex + 1]; // or some other value
                }

        // Interpolate the bin value
        const float InterpolatedValue = FMath::Lerp(oldCQT[BinIndex], currentCQT[BinIndex],   BinInterpFactor);

        // const float InterpolatedValue = FMath::CubicInterp(oldValue, oldCQT[BinIndex], currentCQT[BinIndex], newValue, BinInterpFactor);
        // Do something with the interpolated value, e.g. store it in a new array
        // interpolatedCQT[BinIndex] = InterpolatedValue;

        outCQT[BinIndex] = InterpolatedValue;
    }

        oldCQT = currentCQT;

        
        TArray<float> SmoothedCQT;

        

        if(doSmooth){                
        SmoothedCQT.SetNumUninitialized(outCQT.Num());
            const int32 WindowSize = smoothingWindowSize; // adjust window size as desired
            const float ScaleFactor = 1.0f / static_cast<float>(WindowSize);

            for (int32 i = 0; i < outCQT.Num(); i++)
            {
                float Sum = 0.0f;
                int32 Count = 0;

                for (int32 j = -WindowSize / 2; j <= WindowSize / 2; j++)
                {
                    int32 Index = FMath::Clamp(i + j, 0, outCQT.Num() - 1);
                    Sum += outCQT[Index];
                    Count++;
                }

                SmoothedCQT[i] = Sum * ScaleFactor;
            }
        }
        else{
            SmoothedCQT = outCQT;
        }

        if(doNormalize){
            float MinValue = TNumericLimits<float>::Max();
            float MaxValue = TNumericLimits<float>::Min();

            for (int32 i = 0; i < SmoothedCQT.Num(); i++)
            {
                MinValue = FMath::Min(MinValue, SmoothedCQT[i]);
                MaxValue = FMath::Max(MaxValue, SmoothedCQT[i]);
            }
            Audio::ArraySubtractByConstantInPlace(SmoothedCQT, NoiseFloorDB);


            // Normalize the SmoothedCQT array between 0 and 1
            const float CQTRange = MaxValue - NoiseFloorDB;
            if(CQTRange > SMALL_NUMBER)
            {
			    const float Scaling = 1.f / CQTRange;
			    Audio::ArrayMultiplyByConstantInPlace(SmoothedCQT, Scaling);

            }
            // else{
            //     if (SmoothedCQT.Num() > 0)
            //     {
            //         FMemory::Memset(SmoothedCQT.GetData(), 0, sizeof(float) * SmoothedCQT.Num());
            //     }
            // }
            // Clamp the values in the SmoothedCQT array between 0 and 1
		    Audio::ArrayClampInPlace(SmoothedCQT, 0.f, 1.f);

        }

        // Clamp the values in the SmoothedCQT array between 0 and 1
        if(doClamp){
            for (int32 i = 0; i < SmoothedCQT.Num(); i++)
            {
                SmoothedCQT[i] = FMath::Clamp(SmoothedCQT[i], 0.0f, 1.0f);
            }
        }
        if(doScalePeaks){

        for (int32 i = 0; i < SmoothedCQT.Num(); i++)
        {
            SmoothedCQT[i] = UKismetMathLibrary::MultiplyMultiply_FloatFloat((SmoothedCQT[i] * scaleMultiplier), peakExponentMultiplier);
        }
        }
        if(doSupressQuiet)
        {
            ScaleArray(SmootheCQT);
        }
        outCQT = SmoothedCQT;


}

void ARealTimeCQTManager::ApplyLowpassFilter(const TArray<float>& InSpectrum, float CutoffFrequency, float SampleRate, TArray<float>& OutSpectrum)
{

    OutSpectrum.SetNumUninitialized(InSpectrum.Num());
    LowPassFilter.ProcessAudio(InSpectrum.GetData(), InSpectrum.Num(), OutSpectrum.GetData());
}




void ARealTimeCQTManager::ScaleArray(TArray<float>& InputArray, float ScalingFactor)
{
    for (int i = 0; i < InputArray.Num(); i++)
    {
        float CurrentValue = InputArray[i];
        float DistanceFromZero = FMath::Abs(CurrentValue);
        float ScaledValue = FMath::Sign(CurrentValue) * FMath::Pow(DistanceFromZero, ScalingFactor);
        InputArray[i] = ScaledValue;
    }
}

void ARealTimeCQTManager::SmoothSignal(const TArrayView<float>& InSignal, TArray<float>& OutSignal, int32 WindowSize)
{
    // Create a temporary array to hold the smoothed signal
    TArray<float> TempSignal;
    TempSignal.SetNumUninitialized(InSignal.Num());

    // Calculate the sum of the signal values within the window
    float WindowSum = 0.0f;
    for (int32 i = 0; i < WindowSize; i++)
    {
        WindowSum += InSignal[i];
    }

    // Apply the moving average filter to the signal
    for (int32 i = 0; i < InSignal.Num() - WindowSize; i++)
    {
        TempSignal[i + WindowSize / 2] = WindowSum / WindowSize;

        // Subtract the value that is falling out of the window and add the new value coming in
        WindowSum -= InSignal[i];
        WindowSum += InSignal[i + WindowSize];
    }

    // Copy the smoothed signal to the output array
    OutSignal = TempSignal;
}


void ARealTimeCQTManager::GetCenterFrequencies()
{
    for(int32 i = 0; i < NumBands; i++){
        // UE_LOG(LogTemp, Warning, TEXT("Band Index: %d"), i);
        // UE_LOG(LogTemp, Warning, TEXT("NumBands: %f"), NumBandsPerOctave);
        float CenterFrequency =  KernelLowestCenterFreq * FMath::Pow(2.f, static_cast<float>(i) / NumBandsPerOctave);
        CenterFrequencies[i] = CenterFrequency;
    }
}

void ARealTimeCQTManager::GetSampleIndices()
{
    for(int32 i = 0; i < NumBands; i++){
        float CenterFrequency = CenterFrequencies[i];

        int32 SampleIndex = CenterFrequency * (((fftSize - NumHopFrames) )/sampleRate) * (sampleRate/1000) * 2;

        SampleIndices[i] = SampleIndex;


    }
}

int32 ARealTimeCQTManager::FindDifference(const TArray<float> Original, const TArray<float> Alter)
{
    for(int32 i = 0; i < Original.Num(); i++)
    {
        if(FMath::RoundFromZero(Original[i]) != FMath::RoundFromZero(Alter[i]))
        {
            UE_LOG(LogTemp, Warning, TEXT("Origin %f"), Original[4000]);
            UE_LOG(LogTemp, Warning, TEXT("Alter %f"), Alter[4000]);


            return i;
        }
    }

    return -1;
}

int32 ARealTimeCQTManager::FindClosestValue(const TArray<int32>& Array, int32 TargetValue)
{
    int32 ClosestValue = Array[0];
    int32 ClosestDistance = FMath::Abs(ClosestValue - TargetValue);
    int32 ClosestValueIndex = 0;
    for (int32 i = 1; i < Array.Num(); i++)
    {
        int32 CurrentValue = Array[i];
        int32 CurrentDistance = FMath::Abs(CurrentValue - TargetValue);
        
        if (CurrentDistance < ClosestDistance)
        {
            ClosestValue = CurrentValue;
            ClosestValueIndex = i;
            ClosestDistance = CurrentDistance;
        }
    }
    
    return ClosestValueIndex;
}


void ARealTimeCQTManager::UpdateHighPassFilter(){
    HighPassFilter.SetFrequency(HighPassCutoffFrequency);
    HighPassFilter.SetBandwidth(HighPassBandWidth);
    HighPassFilter.SetGainDB(HighPassGain);

}

void ARealTimeCQTManager::UpdateLowPassFilter(){
    LowPassFilter.SetFrequency(LowPassCutoffFrequency);
    LowPassFilter.SetBandwidth(LowPassBandWidth);
    LowPassFilter.SetGainDB(LowPassGain);

}

void ARealTimeCQTManager::FireOnSpectrumUpdatedEvent(const int index, const int value)
{
     FUpdateData newData;
     newData.index = index;
     newData.value = value;
     OnSpectrumUpdatedEvent.Broadcast(newData);
}

void ARealTimeCQTManager::FireOnGiveDifferenceUpdatedEvent(const int index){
     OnGiveDifferenceEvent.Broadcast(index);
}

// OverlapCQTAnalyzer = MakeShared<ConstantQAnalyzer>(sampleRate, numBins, initFrequency, numBinsPerOctave, windowSize, 0.5f, -50.f);
