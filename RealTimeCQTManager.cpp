// Fill out your copyright notice in the Description page of Project Settings.


#include "RealTimeCQTManager.h"
#include "./CoreDSP/ConstantQAnalyzer.h"
#include "./CoreDSP/ConstantQ.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/DeinterleaveView.h"
#include "SampleBuffer.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values
ARealTimeCQTManager::ARealTimeCQTManager()
{

    #if WITH_EDITOR
	#define LOCTEXT_NAMESPACE "Custom Detail"
	static const FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);

	//Change "Actor" for the type of your Class(eg. Actor, Pawn, CharacterMovementComponent)
	//Change "MySection" to the name of Desired Section
	TSharedRef<FPropertySection> FFTSection = PropertyModule.FindOrCreateSection("Actor", "FFT", LOCTEXT("FFT", "FFT"));
	TSharedRef<FPropertySection> PeakSection = PropertyModule.FindOrCreateSection("Actor", "Peak", LOCTEXT("Peak", "Peak"));
	TSharedRef<FPropertySection> ProcessingSection = PropertyModule.FindOrCreateSection("Actor", "Processing", LOCTEXT("Processing", "Processing"));
	TSharedRef<FPropertySection> OutputSection = PropertyModule.FindOrCreateSection("Actor", "Output", LOCTEXT("Output", "Output"));

	//You can add multiples categories to be tracked by this section
	FFTSection->AddCategory("FFT Settings");
    PeakSection -> AddCategory("Peak Settings");
    ProcessingSection -> AddCategory("Sample Processing");
    ProcessingSection -> AddCategory("Spectrum Processing");
    OutputSection->AddCategory("Values");
    OutputSection->AddCategory("Array");

	#undef LOCTEXT_NAMESPACE
	#endif

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}  
// Called when the game starts or when spawned
void ARealTimeCQTManager::BeginPlay()
{
    FocusIndices.AddZeroed(NumBands);
	Super::BeginPlay();

	defaultSettings.BandWidthStretch = BandWidthStretch;
	defaultSettings.NumBandsPerOctave = NumBandsPerOctave;
	defaultSettings.KernelLowestCenterFreq = KernelLowestCenterFreq;
	defaultSettings.FFTSize = fftSize;
	defaultSettings.WindowType = Audio::EWindowType::Blackman;
    defaultSettings.NumBands = NumBands;

    focusSettings.FocusStart = FocusStart;						
    focusSettings.FocusMin = FocusMin;
    focusSettings.FocusMax = FocusMax;

    focusSettings.LogNormal = LogNormal;
    focusSettings.LogFast = LogFast;
    focusSettings.LogSlow = LogSlow;

    focusSettings.doStupid = doStupid;

    PeakPickerSettings.NumPreMax = NumPreMax;
    PeakPickerSettings.NumPostMax = NumPostMax;
    PeakPickerSettings.NumPreMean = NumPreMean;
    PeakPickerSettings.NumPostMean = NumPostMean;
    PeakPickerSettings.NumWait = NumWait;
    PeakPickerSettings.MeanDelta = MeanDelta;






    NumHopFrames = FMath::Max(1,NumHopFrames);
    NumHopSamples = NumHopFrames * NumChannels;
    NumWindowSamples = fftSize * NumChannels;

	COLAHopSize = GetCOLAHopSizeForWindow(defaultSettings.WindowType, fftSize);
    
    outCQT.AddZeroed(NumBands);
    CenterFrequencies.AddZeroed(NumBands);
    SampleIndices.AddZeroed(NumBands);



	ConstantQAnalyzer = MakeUnique<Audio::FConstantQAnalyzer>(defaultSettings, focusSettings, sampleRate);
    SlidingBuffer = MakeUnique<Audio::TSlidingBuffer<uint8> >(NumWindowSamples + NumPaddingSamples,NumHopSamples);
    SlidingFloatBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(fftSize, NumHopFrames);
    PeakPicker = MakeUnique<Audio::FPeakPicker>(PeakPickerSettings);
    HighPassFilter.Init(sampleRate, 1, Audio::EBiquadFilter::Highpass, HighPassCutoffFrequency, HighPassBandWidth, HighPassGain );
    LowPassFilter.Init(sampleRate, 1, Audio::EBiquadFilter::Lowpass, LowPassCutoffFrequency , LowPassBandWidth, LowPassGain);
    
    GetCenterFrequencies();
    GetSampleIndices();

    MinCenterFreq = CenterFrequencies[0];
    MaxSampleIndex = getMaxValue(SampleIndices);
    MaxCenterFreq = getMaxValue(CenterFrequencies);
    MeanCenterFreq = getMeanValue(CenterFrequencies);
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
    // const int32 SampleSize = sizeof(int32); // 16-bit PCM samples

    // const int32 SamplesPerChannel = byteArray.Num() / (NumChannels * SampleSize);
    // const int32 PaddedSamplesPerChannel = SamplesPerChannel + NumPaddingSamples;

    // TArray<uint8> PaddedPCMStream;
    // PaddedPCMStream.Reserve(PaddedSamplesPerChannel * NumChannels * SampleSize);
    // PaddedPCMStream.AddZeroed(NumPaddingSamples * NumChannels * SampleSize);
    // PaddedPCMStream.Append(byteArray);

    TArray<float> floatStream= combineStream(byteArray, 2);

    PCMToFloat(floatStream, inAmp);
    }
}

TArray<float>  ARealTimeCQTManager::combineStream(const TArray<uint8> interleavedStream, int numChannels)
{
    canProcesses = false;
	// UE_LOG(LogTemp, Warning, TEXT("The length of the stream is: %d"), interleavedStream.Num());
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

                // float normalizedSampleValue = static_cast<float>(sampleValue) / 127.5f - 1.0f;
                float normalizedSampleValue = (static_cast<float>(sampleValue) * gainFactor) /  127.5f - 1.0f;


                // Add the sample value for the current channel to the sum

                sampleSum +=  normalizedSampleValue;

              
                
            }
        }

        // Store the sum of the sample values for all channels in the output array
        summedSamples.Add(sampleSum);
    }



    AmplitudeSampleProcessing(summedSamples);
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
        // AmplitudeSampleProcessing(inAmp);


        ConstantQAnalyzer -> CalculateCQT(inAmp.GetData(), outCQT);
        CQTProcessing();
        MaxSpectrum =  getMaxValue(outCQT);
        for(int32 i = 0; i < outCQT.Num(); i ++ ){

            FireOnSpectrumUpdatedEvent(i,outCQT[i]);
        }
    }
        canProcesses = true;

}
void ARealTimeCQTManager::AmplitudeSampleProcessing(TArray<float>& inAmplitude )
{

        if(doLowpassFilter)
        {
            TArrayView<const float> AmpView(inAmplitude.GetData(), inAmplitude.Num());
            LowPassFilter.ProcessAudio(AmpView.GetData(), AmpView.Num(), inAmplitude.GetData());      
        }
        if(doHighpassFilter)
        {
            TArrayView<const float> AmpView(inAmplitude.GetData(), inAmplitude.Num());

            OriginalAmp = AmpView;
            HighPassFilter.ProcessAudio(AmpView.GetData(), AmpView.Num(), inAmplitude.GetData());
            TArrayView<const float> Alter(inAmplitude.GetData(), inAmplitude.Num());
            AlterAmp = Alter;


        }
        if(doAbsAmp)
        {

            Audio::ArrayAbsInPlace(inAmplitude);
        }
        // Audio::ArrayMultiplyByConstantInPlace(inAmplitude, gainFactor);
        Audio::ArrayMultiplyByConstantInPlace(inAmplitude, 1.f / FMath::Sqrt(static_cast<float>(2))); 



}


void ARealTimeCQTManager::CQTProcessing()
{

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
                    oldValue = oldCQT[BinIndex]/1.1; // or some other value
                }
                if (BinIndex == currentCQT.Num() - 1) 
                {
                 // Access the element at i-1 if i > 0
                    newValue = currentCQT[BinIndex]/1.1;
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


        if(doPeakDetection)
        {
            PeakPicker -> PickPeaks(outCQT, PeakIndices);
        }
        oldCQT = currentCQT;

        
        TArray<float> SmoothedCQT = outCQT;
        
        


        if(doSmooth)
        {           
                 
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


        if(doNormalize)
        {
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
        if(doSurpressQuiet)
        {
            ScaleArray(SmoothedCQT, QuietMultiplier);
        }
        if(doScale)
        {
            for (int32 i = 0; i < SmoothedCQT.Num(); i++)
            {
                SmoothedCQT[i] = SmoothedCQT[i] * scaleMultiplier;
            }

        }

        if(doPeakExp)
        {
            for (int32 i = 0; i < SmoothedCQT.Num(); i++)
            {
                float Bonus = 0;

            if(doFocusExp)
            {

                if(FocusIndices[i] == true)
                {
                    Bonus += FocusExponentMultiplier;
                }
            }   

                SmoothedCQT[i] = UKismetMathLibrary::MultiplyMultiply_FloatFloat((SmoothedCQT[i]), peakExponentMultiplier + Bonus);
            }

        }

        outCQT = SmoothedCQT;


}
void ARealTimeCQTManager::FocusExponentiation(TArray<float>& InputArray, float ScalingFactor )
{
    float InflectionPoint =  InputArray.Num() - InputArray.Num()/4;

    for(int32 i = 0 ; i < InputArray.Num(); i++){
        if(FocusIndices[i] == true){

            InputArray[i] = FMath::Pow(InputArray[i], ScalingFactor);

        }
    }
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
        float LogBase = LogNormal ;
		bool CanSwitch = true; 
		bool CanSlow = true;


    for(int32 i = 0; i < NumBands; i++){

        UE_LOG(LogTemp, Warning, TEXT("Band Index: %d"), i);

        float CenterFrequency;

        if(doStupid)
        {

            CenterFrequency = Audio::FPseudoConstantQ::GetStupidConstantQCenterFrequency(i, KernelLowestCenterFreq, NumBandsPerOctave, LogBase, NumBands);
        
        }
        else
        {
            CenterFrequency = Audio::FPseudoConstantQ::GetConstantQCenterFrequency(i, KernelLowestCenterFreq, NumBandsPerOctave);
        }



        if(CenterFrequency < FocusStart && CanSlow)
        {
				LogBase = LogFast;
                CanSlow = false;
        }
        else if(CenterFrequency >= FocusMin && CanSwitch)
        {
				LogBase = LogSlow;
				CanSwitch = false;
        }
        if(CenterFrequency > FocusMax)
        {
            LogBase = LogNormal;
        }
        if(i > 0 && CenterFrequency < CenterFrequencies[i - 1])
        {
            CenterFrequency = CenterFrequencies[i-1] + 50.0;
        }

        if(CenterFrequency >= FocusMin && CenterFrequency < FocusMax )
        {
            FocusIndices[i] = true;
        }

        // float CenterFrequency =  KernelLowestCenterFreq * FMath::Pow(2.f, static_cast<float>(i) / NumBandsPerOctave);

        CenterFrequencies[i] = CenterFrequency;
    }
}

void ARealTimeCQTManager::GetSampleIndices()
{
    float duration = ((((COLAHopSize) + 1) - (1 >> Audio::CeilLog2(NumHopFrames)))) /(sampleRate *  1000 * 2);
    for(int32 i = 0; i < NumBands; i++){
        float CenterFrequency = CenterFrequencies[i];

        int32 SampleIndex = CenterFrequency * duration * (sampleRate);

        SampleIndices[i] = SampleIndex/4;


    }
}

void ARealTimeCQTManager::FindDifference( const TArray<float>&  Original,   TArrayView<const float>  Alter, float DifferenceThreshold)
{   

    TestOriginalAmp = Original;
    TestAlterAmp = Alter;


    for(int32 i = 0; i < SampleIndices.Num(); i++)
    {   
        int32 SampleIndex = SampleIndices[i];
        float OriginalValue = Original[SampleIndex];
        float AlterValue = Alter[SampleIndex];


    }
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

int32 ARealTimeCQTManager::GetCOLAHopSizeForWindow(Audio::EWindowType InType, int32 WindowLength)
	{
		switch (InType)
		{
		case Audio::EWindowType::Hann:
		case Audio::EWindowType::Hamming:
			return FMath::FloorToInt((0.5f) * WindowLength);
			break;
		case Audio::EWindowType::Blackman:
			// Optimal overlap for any Blackman window is derived in this paper:
			// http://edoc.mpg.de/395068
			return FMath::FloorToInt((0.339f) * WindowLength);
			break;
		case Audio::EWindowType::None:
		default:
			return WindowLength;
			break;
		}
	}

float ARealTimeCQTManager::getMaxSpectrum()
{
   float max = MaxSpectrum;
    return max;
}
float ARealTimeCQTManager::getMaxValue(const TArray<float>& arr){
    float maxVal = arr[0];
    for (int i = 1; i < arr.Num(); i++) {
        if (arr[i] > maxVal) {
            maxVal = arr[i];
        }
    }
    return maxVal;
}
int ARealTimeCQTManager::getMaxValue(const TArray<int>& arr){
    int maxVal = arr[0];
    for (int i = 1; i < arr.Num(); i++) {
        if (arr[i] > maxVal) {
            maxVal = arr[i];
        }
    }
    return maxVal;
}

float ARealTimeCQTManager::getMeanValue(const TArray<float>& arr) {
    float sum = 0.0f;
    for (int i = 0; i < arr.Num(); i++) {
        sum += arr[i];
    }
    return sum / arr.Num();
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

void ARealTimeCQTManager::FireOnSpectrumUpdatedEvent(const int index, const float value)
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
