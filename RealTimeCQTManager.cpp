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
	ConstantQAnalyzer = MakeUnique<Audio::FConstantQAnalyzer>(defaultSettings, sampleRate);
    SlidingBuffer = MakeUnique<Audio::TSlidingBuffer<uint8> >(NumWindowSamples + NumPaddingSamples,NumHopSamples);
    SlidingFloatBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(fftSize, NumHopFrames);
    BiquadFilter.Init(sampleRate, NumChannels/NumChannels, Audio::EBiquadFilter::Lowpass, cutoffFrequency, 2.f, 0.f );
}

// Called every frame
void ARealTimeCQTManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARealTimeCQTManager::anaylze(TArray<uint8> byteArray)
{
        PCMToFloat(byteArray, inAmp);
}

TArray<float>  ARealTimeCQTManager::combineStream(TArray<uint8> interleavedStream, int numChannels)
{
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
                float normalizedSampleValue = static_cast<float>(sampleValue) / 255.f * 2.f - 1.f;

                float absSampleValue = FMath::Abs(normalizedSampleValue);


                // Add the sample value for the current channel to the sum
                sampleSum += normalizedSampleValue ;
            }
            else
            {

                float sampleValue = 0.f;
                for (int k = 0; k < sampleSize; k++)
                {
                    sampleValue += interleavedStream[sampleOffset + channelOffset + k] << (8 * k);
                }

                // Normalize the sample value to the range [-1, 1]
                sampleValue /= ((1 << (bitsPerSample - 1)) - 1);

                // Add the sample value for the current channel to the sum
                sampleSum += sampleValue;

            }
        }

        // Store the sum of the sample values for all channels in the output array
        summedSamples.Add(sampleSum);
    }

    // Return the summed samples
	//(LogTemp, Warning, TEXT("The length is: %d"), summedSamples.Num());
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


void ARealTimeCQTManager::PCMToFloat(const TArray<uint8>& PCMStream, TArray<float>& OutAmplitudes)
{
    const int32 SampleSize = sizeof(int32); // 16-bit PCM samples
    const int32 SamplesPerChannel = PCMStream.Num() / (NumChannels * SampleSize);
    const int32 PaddedSamplesPerChannel = SamplesPerChannel + NumPaddingSamples;

    TArray<uint8> PaddedPCMStream;
    PaddedPCMStream.Reserve(PaddedSamplesPerChannel * NumChannels * SampleSize);
    PaddedPCMStream.AddZeroed(NumPaddingSamples * NumChannels * SampleSize);
    PaddedPCMStream.Append(PCMStream);

    TArray<float> floatStream= combineStream(PaddedPCMStream, NumChannels);



    TArray<float> eventSpectrum;
    eventSpectrum.AddZeroed(fftSize);
    // Iterate over sliding windows
    for (const TArray<float>& Window : Audio::TAutoSlidingWindow<float>(*SlidingFloatBuffer, floatStream , FloatWindowBuffer, true))
    {
        inAmp = Window;
        AmplitudeSampleProcessing(inAmp);

        if (eventSpectrum.Num() != outCQT.Num())
        {
            eventSpectrum.Reserve(outCQT.Num());
        }
        for (int32 i = 0; i < outCQT.Num(); i++)
        {
            eventSpectrum[i] = outCQT[i];
        }
        CQTProcessing();

        FireOnSpectrumUpdatedEvent(eventSpectrum);
    }
}
void ARealTimeCQTManager::AmplitudeSampleProcessing(TArray<float>& inAmplitude ){

        Audio::ArrayMultiplyByConstantInPlace(inAmplitude, gainFactor);
        if(doAbsAmp)
        {
            Audio::ArrayAbsInPlace(inAmplitude);
        }
        Audio::ArrayMultiplyByConstantInPlace(inAmplitude, 1.f / FMath::Sqrt(static_cast<float>(1)));
        if(doLowpassFilter)
        {
            ampFilter = inAmplitude; 
            BiquadFilter.ProcessAudio(ampFilter.GetData(), ampFilter.Num(), inAmplitude.GetData());
            outAmp = inAmplitude;
            ConstantQAnalyzer -> CalculateCQT(outAmp.GetData(), outCQT);
        }
        else{
            outAmp = inAmplitude;
            ConstantQAnalyzer -> CalculateCQT(outAmp.GetData(), outCQT);
        }
      
}
void ARealTimeCQTManager::CQTProcessing()
{

        currentCQT = outCQT;
        UE_LOG(LogTemp, Error, TEXT("Start of CQT Processiong outCqt is length %d"), outCQT.Num())
        
        currentTime = UGameplayStatics::GetAudioTimeSeconds(GetWorld()) ;
        deltaTime = (currentTime + expandTime) - lastUpdateTime;
        
        if(oldCQT.Num() > 0){
            for(int32 i = 0; i < currentCQT.Num() ; i++){
                float oldValue;
                float newValue; 
                if (i > 0) 
                {
                 // Access the element at i-1 if i > 0
                    oldValue = oldCQT[i - 1];
                } 
                else {
                // Handle the case when i == 0
                    oldValue = oldCQT[i]/2; // or some other value
                }
                if (i == currentCQT.Num() - 1) 
                {
                 // Access the element at i-1 if i > 0
                    newValue = currentCQT[i]/2;
                } 
                else {
                // Handle the case when i == 0
                    newValue = currentCQT[i + 1]; // or some other value
                }
                outCQT[i] = FMath::CubicInterp(oldValue, oldCQT[i], currentCQT[i], newValue, deltaTime);
            }
        }
        oldCQT = currentCQT;
        lastUpdateTime = currentTime;
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
            Audio::ArraySubtractByConstantInPlace(SmoothedCQT, MinValue);


            // Normalize the SmoothedCQT array between 0 and 1
            const float CQTRange = MaxValue - MinValue;
            if(CQTRange > SMALL_NUMBER)
            {
			    const float Scaling = 1.f / CQTRange;
			    Audio::ArrayMultiplyByConstantInPlace(SmoothedCQT, Scaling);

            }
            else{
                if (SmoothedCQT.Num() > 0)
                {
                    FMemory::Memset(SmoothedCQT.GetData(), 0, sizeof(float) * SmoothedCQT.Num());
                }
            }
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
        outCQT = SmoothedCQT;
}

void ARealTimeCQTManager::ApplyLowpassFilter(const TArray<float>& InSpectrum, float CutoffFrequency, float SampleRate, TArray<float>& OutSpectrum)
{

    OutSpectrum.SetNumUninitialized(InSpectrum.Num());
    BiquadFilter.ProcessAudio(InSpectrum.GetData(), InSpectrum.Num(), OutSpectrum.GetData());
}


void ARealTimeCQTManager::OverlapAdd(const TArray<float> AudioData){

    UE_LOG(LogTemp, Error, TEXT("Overlapping stream of size %d"), AudioData.Num())
    TArray<float> OutSpectrum;
    OutSpectrum.SetNumUninitialized(NumBands);
    FrameSpectrum.Reset();

    for (int32 i = 0; i < AudioData.Num(); i += NumHopFrames)
    {
        TArray<float> FrameData;
        // Compute the current frame
        FrameData.SetNumUninitialized(fftSize);
        FMemory::Memcpy(FrameData.GetData(), &AudioData[i], sizeof(float) * fftSize);

        // Compute the CQT spectrum for the current frame
        ConstantQAnalyzer->CalculateCQT(FrameData.GetData(), FrameSpectrum);
        UE_LOG(LogTemp, Error, TEXT("Frame Scpectrum size: %d"), FrameSpectrum.Num())
        const float ScaleFactor = 1.0f / static_cast<float>(fftSize); 


        // Add the frame spectrum to the output buffer using overlap-add
        for (int32 j = 0; j < FrameSpectrum.Num(); j++)
        {
            int32 Index = i + j;
            if (Index < OutSpectrum.Num())
            {
                OutSpectrum[Index] += FrameSpectrum[j];
            }
        }
    }

        outCQT = OutSpectrum;
        UE_LOG(LogTemp, Error, TEXT("outCqt overlap finished size: %d"), outCQT.Num())

}



void ARealTimeCQTManager::FireOnSpectrumUpdatedEvent(TArray<float> out){
     OnSpectrumUpdatedEvent.Broadcast(out);
}
// OverlapCQTAnalyzer = MakeShared<ConstantQAnalyzer>(sampleRate, numBins, initFrequency, numBinsPerOctave, windowSize, 0.5f, -50.f);
