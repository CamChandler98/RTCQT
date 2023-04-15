// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimeCQTManager.h"
#include "ConstantQAnalyzer.h"
#include "DSP/ConstantQ.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/DeinterleaveView.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
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
    NumHopFrames = FMath::Max(1, FMath::RoundToInt( fftSize * analysisPeriod));
    NumHopSamples = NumHopFrames * NumChannels;
    NumWindowSamples = fftSize * NumChannels;

	ConstantQAnalyzer = MakeUnique<Audio::FConstantQAnalyzer>(defaultSettings, sampleRate);
    SlidingBuffer = MakeUnique<Audio::TSlidingBuffer<uint8> >(NumWindowSamples + NumPaddingSamples,NumHopSamples);
    SlidingFloatBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(fftSize + NumPaddingSamples, NumHopFrames);
    Smoother = MakeUnique<Audio::FAttackReleaseSmoother>(sampleRate, 1,  10.0f, 100.0f, false );
    Envelop.SetMode(Audio::EPeakMode::RootMeanSquared);
	
}

// Called every frame
void ARealTimeCQTManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARealTimeCQTManager::anaylze(TArray<uint8> byteArray)
{
		outCQT.Reset();

        if(doBitDirect)
        {
		    PCMToAmplitude(byteArray, outAmp);

        }
        else{
            PCMToFloat(byteArray, outAmp);

        }




}

TArray<float>  ARealTimeCQTManager::combineStream(TArray<uint8> interleavedStream, int numChannels, int bitsPerSample)
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

                float absSampleValue = FMath::Abs(normalizedSampleValue* gainFactor);


                // Add the sample value for the current channel to the sum
                sampleSum += absSampleValue ;
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

void ARealTimeCQTManager::PCMToAmplitude(const TArray<uint8>& PCMStream, TArray<float>& OutAmplitudes)
{

	//(LogTemp, Warning, TEXT("The length of the stream is: %d"), PCMStream.Num());

    const int32 SampleSize = sizeof(int16); // 16-bit PCM samples
    const int32 SamplesPerChannel = PCMStream.Num() / (NumChannels * SampleSize);
    const int32 PaddedSamplesPerChannel = SamplesPerChannel + NumPaddingSamples;

    TArray<uint8> PaddedPCMStream;
    PaddedPCMStream.Reserve(PaddedSamplesPerChannel * NumChannels * SampleSize);
    PaddedPCMStream.AddZeroed(NumPaddingSamples * NumChannels * SampleSize);
    PaddedPCMStream.Append(PCMStream);

    
    // Iterate over sliding windows
    for (const TArray<uint8>& Window : Audio::TAutoSlidingWindow<uint8>(*SlidingBuffer, PaddedPCMStream , WindowBuffer, true))
    {
				FMemory::Memset(OutAmplitudes.GetData(), 0, sizeof(float) * fftSize);



                TArrayView<const uint8> WindowView(Window);
                ChannelBuffer.Reset();
                ChannelBuffer.AddZeroed(Window.Num()/2);

                for(auto Channel : Audio::TAutoDeinterleaveView(WindowView, ChannelBuffer, NumChannels) )
                {
                    const int32 NumSamples = Channel.Values.Num();
                    TArray<float> SampleBuffer;
                    SampleBuffer.Reserve(NumSamples);

                    for (int32 i = 0; i < NumSamples; i++)
                    {
                        float SampleValue = static_cast<float>(Channel.Values[i]) / 32768.0f;
	            //  //(LogTemp, Warning, TEXT("The length of the float  is: %f"), SampleValue);

                        SampleBuffer.Add(SampleValue);
                    }

                    Audio::ArrayMixIn(SampleBuffer, OutAmplitudes);
                }


        // Audio::ArrayMultiplyByConstantInPlace(OutAmplitudes, 1.f / FMath::Sqrt(static_cast<float>(NumChannels)));
	    //(LogTemp, Warning, TEXT("The length of the amp array is: %d"), OutAmplitudes.Num());
        cqtProcessing(OutAmplitudes);
      
    }
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

    TArray<float> floatStream= combineStream(PaddedPCMStream, NumChannels, 32);



        
    // Iterate over sliding windows
    for (const TArray<float>& Window : Audio::TAutoSlidingWindow<float>(*SlidingFloatBuffer, floatStream , FloatWindowBuffer, true))
    {
        //(LogTemp, Warning, TEXT("Window here"));

        outAmp = Window;
        
        Audio::ArrayMultiplyByConstantInPlace(outAmp, 1.f / FMath::Sqrt(static_cast<float>(1)));
       cqtProcessing(outAmp);
      
    }
}

void ARealTimeCQTManager::cqtProcessing(const TArray<float> audioData)
{

        ConstantQAnalyzer -> CalculateCQT(audioData.GetData(), outCQT);
        currentCQT = outCQT;
        
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
                    oldValue = oldCQT[i - 1]/2; // or some other value
                }
                if (i == currentCQT.Num() - 1) 
                {
                 // Access the element at i-1 if i > 0
                    newValue = currentCQT[i + 1]/2;
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