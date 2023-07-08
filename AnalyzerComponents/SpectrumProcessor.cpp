// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectrumProcessor.h"
#include "DSP/FloatArrayMath.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
USpectrumProcessor::USpectrumProcessor()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	PrimaryComponentTick.bCanEverTick = false;
	Settings =  CreateDefaultSubobject<USpectrumSettings>(TEXT("Settings"));

	// ...
}

void USpectrumProcessor::ProcessSpectrum(TArray<float>& CurrentCQT)
{
	if(Toggles -> DoNormalize)
	{
		NormalizeSpectrum(CurrentCQT, NoiseFloorDB);
	}
	if(Toggles -> DoSupressQuiet)
	{
		SupressQuiet(CurrentCQT, QuietMultiplier);
	}
	if(Toggles -> DoScale)
	{
		ScaleSpectrum(CurrentCQT, ScaleMultiplier);
	}
	if(Toggles -> DoFocusExp)
	{
		ExponentiateFocusedSpectrum(CurrentCQT, FocusIndices, PeakExponentMultiplier, FocusExponentMultiplier);
	}
	else if(Toggles -> DoPeakExp)
	{
		ExponentiateSpectrum(CurrentCQT, PeakExponentMultiplier);
	}
	if(Toggles -> DoMedianSmooth)
	{
		MedianSmoothSpectrum(CurrentCQT);
	}
	else if(Toggles -> DoSmooth)
	{
		SmoothSpectrum(CurrentCQT);
	}
	if(Toggles -> DoInterpolate)
	{
		InterpolateSpectrum(CurrentCQT, Toggles -> DoCubicInterpolation);
	}
	if(Toggles -> DoPostScale)
	{
		PostScaleSpectrum(CurrentCQT, PostScaleMultiplier);
	}
}

void USpectrumProcessor::SetSettings(USpectrumSettings* InSettings)
{
	
	Settings -> SmoothingWindowSize = InSettings -> SmoothingWindowSize;
	
	Settings -> InterpolationFactor = InSettings -> InterpolationFactor ;
	
	Settings -> ScaleMultiplier = InSettings -> ScaleMultiplier;
	Settings -> PostScaleMultiplier = InSettings -> PostScaleMultiplier;

	
	Settings -> QuietMultiplier = InSettings -> QuietMultiplier;
	
	Settings ->  PeakExponentMultiplier = InSettings -> PeakExponentMultiplier;
	
	Settings -> FocusExponentMultiplier = InSettings -> FocusExponentMultiplier;
	Settings -> QuietThreshold = InSettings -> QuietThreshold ;


	Settings -> HighFrequencyBoostFactor = InSettings -> HighFrequencyBoostFactor;

}

void USpectrumProcessor::SetParams()
{
	
	SmoothingWindowSize = Settings -> SmoothingWindowSize;
	
	InterpolationFactor =  Settings -> InterpolationFactor;
	
	ScaleMultiplier = Settings -> ScaleMultiplier;
	PostScaleMultiplier = Settings -> PostScaleMultiplier;

	
	QuietMultiplier = Settings -> QuietMultiplier;
	
	PeakExponentMultiplier = Settings ->  PeakExponentMultiplier;
	
	FocusExponentMultiplier = Settings -> FocusExponentMultiplier;

	QuietThreshold = Settings -> QuietThreshold ;


	HighFrequencyBoostFactor = Settings -> HighFrequencyBoostFactor;
}

void USpectrumProcessor::SetArraySize(int32 InSize)
{
	FocusIndices.AddZeroed(InSize);
	PreviousCQT.AddZeroed(InSize);
}

// Called when the game starts
void USpectrumProcessor::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void USpectrumProcessor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USpectrumProcessor::InterpolateSpectrum(TArray<float>& CurrentCQT, bool bDoCubicInterpolation)
{

	TArray<float> WorkCQT = CurrentCQT;
    const int32 NumBins = PreviousCQT.Num();

	// Interpolate between the two spectra
    for (int32 BinIndex = 0; BinIndex < NumBins; BinIndex++)
    {
        // Calculate the difference between the two bins
        const float BinDiff = FMath::Abs(WorkCQT[BinIndex] - PreviousCQT[BinIndex]);

        // Calculate the interpolation factor for this bin
        const float BinInterpFactor = FMath::Clamp(BinDiff * InterpolationFactor, 0.f, 1.f);

                float OldValue;
                float NewValue; 
                if (BinIndex > 0) 
                {
                 // Access the element at i-1 if i > 0
                    OldValue = PreviousCQT[BinIndex - 1];
                } 
                else {
                // Handle the case when i == 0
                    OldValue = PreviousCQT[BinIndex]/1.1; // or some other value
                }
                if (BinIndex == WorkCQT.Num() - 1) 
                {
                 // Access the element at i-1 if i > 0
                    NewValue = WorkCQT[BinIndex]/1.1;
                } 
                else {
                // Handle the case when i == 0
                    NewValue = WorkCQT[BinIndex + 1]; // or some other value
                }

        // Interpolate the bin value
        const float LinearInterpolatedValue = FMath::Lerp(PreviousCQT[BinIndex], WorkCQT[BinIndex],   BinInterpFactor);

        const float CubicInterpolatedValue = FMath::CubicInterp(OldValue, PreviousCQT[BinIndex], WorkCQT[BinIndex], NewValue, BinInterpFactor);


		if(bDoCubicInterpolation)
		{
       		 CurrentCQT[BinIndex] = CubicInterpolatedValue;
		}
		else
		{
       	 	CurrentCQT[BinIndex] = LinearInterpolatedValue;

		}
    }

	PreviousCQT = WorkCQT;
}

void USpectrumProcessor::SmoothSpectrum(TArray<float>& CurrentCQT)
{
	TArray<float> SmoothedCQT = CurrentCQT;
			
	const int32 WindowSize = SmoothingWindowSize; // adjust window size as desired
	const float ScaleFactor = 1.0f / static_cast<float>(WindowSize);

	for (int32 i = 0; i < CurrentCQT.Num(); i++)
	{
		float Sum = 0.0f;
		int32 Count = 0;

		for (int32 j = -WindowSize / 2; j <= WindowSize / 2; j++)
		{
			int32 Index = FMath::Clamp(i + j, 0, CurrentCQT.Num() - 1);
			Sum += CurrentCQT[Index];
			Count++;
		}

		SmoothedCQT[i] = Sum * ScaleFactor;
	}
	
	for (int32 i = 0; i < CurrentCQT.Num(); i++)
	{
		CurrentCQT[i] = SmoothedCQT[i];
	}
}

void USpectrumProcessor::MedianSmoothSpectrum(TArray<float>& CurrentCQT)
{
	TArray<float> SmoothedCQT = CurrentCQT;

	const int32 NumBins = CurrentCQT.Num();
	const int32 HalfWindowSize = SmoothingWindowSize / 2;

	for (int32 i = 0; i < NumBins; ++i)
	{
		TArray<float> WindowValues;

		for (int32 j = -HalfWindowSize; j <= HalfWindowSize; ++j)
		{
			int32 Index = FMath::Clamp(i + j, 0, NumBins - 1);
			WindowValues.Add(CurrentCQT[Index]);
		}

		// Sort the window values
		WindowValues.Sort();

		// Get the median value
		float Median = WindowValues[HalfWindowSize];

		// Assign the median value to the corresponding index in the smoothed array
		SmoothedCQT[i] = Median;
	}

	CurrentCQT = SmoothedCQT;
}

void USpectrumProcessor::NormalizeSpectrum(TArray<float>& CurrentCQT,  float InNoiseFloorDB)
{
	float MinValue = TNumericLimits<float>::Max();
	float MaxValue = TNumericLimits<float>::Min();

	for (int32 i = 0; i < CurrentCQT.Num(); i++)
	{
		MinValue = FMath::Min(MinValue, CurrentCQT[i]);
		MaxValue = FMath::Max(MaxValue, CurrentCQT[i]);
	}

	Audio::ArraySubtractByConstantInPlace(CurrentCQT, NoiseFloorDB);


	const float CQTRange = MaxValue - NoiseFloorDB;

	if(CQTRange > SMALL_NUMBER)
	{
		const float Scaling = 1.1f / CQTRange;
		Audio::ArrayMultiplyByConstantInPlace(CurrentCQT, Scaling);

	}
	else{
	    if (CurrentCQT.Num() > 0)
	    {
	        FMemory::Memset(CurrentCQT.GetData(), 0, sizeof(float) * CurrentCQT.Num());
	    }
	}
	// Clamp the values in the CurrentCQT array between 0 and 1
	Audio::ArrayClampInPlace(CurrentCQT, 0.f, 1.15f);
}


void USpectrumProcessor::SupressQuiet(TArray<float>& CurrentCQT, float ScalingFactor)
{
	for (int i = 0; i < CurrentCQT.Num(); i++)
	{
		float CurrentValue = CurrentCQT[i];

		float DistanceFromZero = FMath::Abs(CurrentValue);
		
		if(DistanceFromZero < QuietThreshold)
		{
			float ScaledValue = FMath::Sign(CurrentValue) * FMath::Pow(DistanceFromZero, ScalingFactor);
			CurrentCQT[i] = ScaledValue;
			
		}
		
	}
}


void USpectrumProcessor::ScaleSpectrum(TArray<float>& CurrentCQT, float ScalingFactor)
{
		if(Toggles -> DoBoostHighFrequency)
		{
			float EndScaleFactor = ScalingFactor + (ScalingFactor * HighFrequencyBoostFactor);


			Audio::ArrayFade(CurrentCQT, ScalingFactor, EndScaleFactor);
		}
		else
		{
			Audio::ArrayMultiplyByConstantInPlace(CurrentCQT, ScalingFactor);
		}
}

void USpectrumProcessor::PostScaleSpectrum(TArray<float>& CurrentCQT, float ScalingFactor)
{
		
	Audio::ArrayMultiplyByConstantInPlace(CurrentCQT, ScalingFactor);
	
}


void USpectrumProcessor::ExponentiateSpectrum(TArray<float>& CurrentCQT, float Exponent)
{
			for (int32 i = 0; i < CurrentCQT.Num(); i++)
            {
                CurrentCQT[i] = FMath::Pow(CurrentCQT[i], Exponent);
            }
}

void USpectrumProcessor::ExponentiateFocusedSpectrum(TArray<float>& CurrentCQT, TArray<bool>& InFocusIndices, float Exponent, float Focus)
{
	float Bonus = Exponent + Focus;

	for (int32 i = 0; i < InFocusIndices.Num(); i++)
	{
		if(InFocusIndices[i] == true)
		{
			CurrentCQT[i] = UKismetMathLibrary::MultiplyMultiply_FloatFloat((CurrentCQT[i]), Bonus);
		}
		else
		{

			CurrentCQT[i] = UKismetMathLibrary::MultiplyMultiply_FloatFloat((CurrentCQT[i]), Exponent);
		}
	}
}
