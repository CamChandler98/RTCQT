// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimeCQTManager.h"
#include "ConstantQAnalyzer.h"
#include "DSP/ConstantQ.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/DeinterleaveView.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ARealTimeCQTManager::ARealTimeCQTManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	



	defaultSettings.BandWidthStretch = BandWidthStretch;
	defaultSettings.NumBandsPerOctave = NumBandsPerOctave;
	defaultSettings.KernelLowestCenterFreq = KernelLowestCenterFreq;
	defaultSettings.FFTSize = fftSize ;
	defaultSettings.WindowType = Audio::EWindowType::Hann;

    NumHopFrames = FMath::Max(1, FMath::RoundToInt( sampleRate * analysisPeriod));
    NumHopSamples = NumHopFrames * NumChannels;
    NumWindowSamples = fftSize * NumChannels;

	ConstantQAnalyzer = MakeUnique<Audio::FConstantQAnalyzer>(defaultSettings, sampleRate);
    SlidingBuffer = MakeUnique<Audio::TSlidingBuffer<uint8> >(NumWindowSamples + NumPaddingSamples,NumHopSamples);
    Smoother = MakeUnique<Audio::FAttackReleaseSmoother>(sampleRate, 1,  10.0f, 100.0f, false );
    Envelop.SetMode(Audio::EPeakMode::RootMeanSquared);
}  
// Called when the game starts or when spawned
void ARealTimeCQTManager::BeginPlay()
{
	Super::BeginPlay();
	
	
}

// Called every frame
void ARealTimeCQTManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARealTimeCQTManager::anaylze(TArray<uint8> byteArray)
{
		outCQT.Reset();
		// wildOutCQT.Reset();
		// TArray<float> inSamples = combineStream(byteArray, 2, 16);
		PCMToAmplitude(byteArray, outAmp);
		// Equal power sum. assuming incoherrent signals.


		// outCQT.Reserve(wildOutCQT.Num());

		// Audio::ArrayMinMaxNormalize(wildOutCQT, outCQT);
}

TArray<float>  ARealTimeCQTManager::combineStream(TArray<uint8> interleavedStream, int numChannels, int bitsPerSample)
{
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

        // Store the sum of the sample values for all channels in the output array
        summedSamples.Add(sampleSum);
    }

    // Return the summed samples
	UE_LOG(LogTemp, Warning, TEXT("The length is: %d"), summedSamples.Num());
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

void ARealTimeCQTManager::PCMToAmplitude(const TArray<uint8>& PCMStream, TArray<float>& OutAmplitudes)
{
    const int32 SampleSize = sizeof(int16); // 16-bit PCM samples
    const int32 SamplesPerChannel = PCMStream.Num() / (NumChannels * SampleSize);
    const int32 PaddedSamplesPerChannel = SamplesPerChannel + NumPaddingSamples;

    TArray<uint8> PaddedPCMStream;
    PaddedPCMStream.Reserve(PaddedSamplesPerChannel * NumChannels * SampleSize);
    PaddedPCMStream.AddZeroed(NumPaddingSamples * NumChannels * SampleSize);
    PaddedPCMStream.Append(PCMStream);

    
    TArray<uint8> WindowBuffer;
    WindowBuffer.AddZeroed(NumWindowSamples);
    OutAmplitudes.Reset();
    OutAmplitudes.AddZeroed(PaddedSamplesPerChannel);
    // Iterate over sliding windows
    for (const TArray<uint8>& Window : Audio::TAutoSlidingWindow<uint8>(*SlidingBuffer, PaddedPCMStream , WindowBuffer, true))
    {
        // Iterate over samples in window
        for (int32 SampleIndex = 0; SampleIndex < NumWindowSamples; SampleIndex++)
        {
            float Amplitude = 0.0f;

            // Iterate over channels in window
            for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
            {
                const int16* SampleDataPtr = reinterpret_cast<const int16*>(Window.GetData() + ((SampleIndex + NumPaddingSamples) * NumChannels + ChannelIndex) * SampleSize);
                const float SampleValue =  static_cast<float>(*SampleDataPtr) / 32768.0f;

                Amplitude += SampleValue;
            }

            Amplitude /= NumChannels;
            OutAmplitudes.Add(Amplitude);
        }
        // Audio::ArrayMultiplyByConstantInPlace(OutAmplitudes, 1.f / FMath::Sqrt(static_cast<float>(NumChannels)));
        
        ConstantQAnalyzer -> CalculateCQT(outAmp.GetData(), outCQT);

        TArray<float> SmoothedCQT;
        if(doSmooth){
                
            SmoothedCQT.SetNumUninitialized(outCQT.Num());

            const int32 WindowSize = initWindowSize; // adjust window size as desired
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

            // Normalize the SmoothedCQT array between 0 and 1
            for (int32 i = 0; i < SmoothedCQT.Num(); i++)
            {
                SmoothedCQT[i] = (SmoothedCQT[i] - NoiseFloorDB);
            }

            const float ConstantQRange = MaxValue - NoiseFloorDB;

            if (ConstantQRange > SMALL_NUMBER)
            {
                const float Scaling = 1.2f / ConstantQRange;
                Audio::ArrayMultiplyByConstantInPlace(SmoothedCQT, Scaling);
            }
        }

        // Clamp the values in the SmoothedCQT array between 0 and 1
        if(doClamp){
            for (int32 i = 0; i < SmoothedCQT.Num(); i++)
            {
                SmoothedCQT[i] = FMath::Clamp(SmoothedCQT[i], 0.0f, 1.3f);
            }
        }
        if(doScalePeaks)
        for (int32 i = 0; i < SmoothedCQT.Num(); i++)
        {
            SmoothedCQT[i] = UKismetMathLibrary::MultiplyMultiply_FloatFloat(SmoothedCQT[i], peakExponentMultiplier);
        }

        outCQT = SmoothedCQT;
      
    }
}