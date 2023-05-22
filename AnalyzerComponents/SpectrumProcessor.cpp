// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectrumProcessor.h"
#include "DSP/FloatArrayMath.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values for this component's properties
USpectrumProcessor::USpectrumProcessor()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.

	PrimaryComponentTick.bCanEverTick = true;
	Settings =  CreateDefaultSubobject<USpectrumSettings>(TEXT("Settings"));

	// ...
}

void USpectrumProcessor::ProcessSpectrum(TArray<float>& CurrentCQT, FSpectrumToggles Toggles)
{
	if(Toggles.DoInterpolate)
	{
		InterpolateSpectrum(CurrentCQT, Toggles.DoCubicInterpolation);
	}
	if(Toggles.DoSmooth)
	{
		SmoothSpectrum(CurrentCQT);
	}
	if(Toggles.DoNormalize)
	{
		NormalizeSpectrum(CurrentCQT, NoiseFloorDB);
	}
	if(Toggles.DoSupressQuiet)
	{
		SupressQuiet(CurrentCQT, QuietMultiplier);
	}
	if(Toggles.DoScale)
	{
		ScaleSpectrum(CurrentCQT, ScaleMultiplier);
	}
	if(Toggles.DoFocusExp)
	{
		ExponentiateFocusedSpectrum(CurrentCQT, FocusIndices, PeakExponentMultiplier, FocusExponentMultiplier);
	}
	else if(Toggles.DoPeakExp)
	{
		ExponentiateSpectrum(CurrentCQT, PeakExponentMultiplier);
	}


}

void USpectrumProcessor::SetSettings(USpectrumSettings* InSettings)
{
	
	Settings -> SmoothingWindowSize = InSettings -> SmoothingWindowSize;
	
	Settings -> InterpolationFactor = InSettings -> InterpolationFactor ;
	
	Settings -> ScaleMultiplier = InSettings -> ScaleMultiplier;
	
	Settings -> QuietMultiplier = InSettings -> QuietMultiplier;
	
	Settings ->  PeakExponentMultiplier = InSettings -> PeakExponentMultiplier;
	
	Settings -> FocusExponentMultiplier = InSettings -> FocusExponentMultiplier;
}

void USpectrumProcessor::SetParams()
{
	
	SmoothingWindowSize = Settings -> SmoothingWindowSize;
	
	InterpolationFactor =  Settings -> InterpolationFactor;
	
	ScaleMultiplier = Settings -> ScaleMultiplier;
	
	QuietMultiplier = Settings -> QuietMultiplier;
	
	PeakExponentMultiplier = Settings ->  PeakExponentMultiplier;
	
	FocusExponentMultiplier = Settings -> FocusExponentMultiplier;
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

    const int32 NumBins = PreviousCQT.Num();

	// Interpolate between the two spectra
    for (int32 BinIndex = 0; BinIndex < NumBins; BinIndex++)
    {
        // Calculate the difference between the two bins
        const float BinDiff = FMath::Abs(CurrentCQT[BinIndex] - PreviousCQT[BinIndex]);

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
                if (BinIndex == CurrentCQT.Num() - 1) 
                {
                 // Access the element at i-1 if i > 0
                    NewValue = CurrentCQT[BinIndex]/1.1;
                } 
                else {
                // Handle the case when i == 0
                    NewValue = CurrentCQT[BinIndex + 1]; // or some other value
                }

        // Interpolate the bin value
        const float LinearInterpolatedValue = FMath::Lerp(PreviousCQT[BinIndex], CurrentCQT[BinIndex],   BinInterpFactor);

        const float CubicInterpolatedValue = FMath::CubicInterp(OldValue, PreviousCQT[BinIndex], CurrentCQT[BinIndex], NewValue, BinInterpFactor);
        // Do something with the interpolated value, e.g. store it in a new array
        // interpolatedCQT[BinIndex] = InterpolatedValue;

		if(bDoCubicInterpolation)
		{
       	 CurrentCQT[BinIndex] = CubicInterpolatedValue;
		}
		else
		{
       	 CurrentCQT[BinIndex] = LinearInterpolatedValue;

		}
    }

	PreviousCQT = CurrentCQT;
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
		const float Scaling = 1.f / CQTRange;
		Audio::ArrayMultiplyByConstantInPlace(CurrentCQT, Scaling);

	}
	else{
	    if (CurrentCQT.Num() > 0)
	    {
	        FMemory::Memset(CurrentCQT.GetData(), 0, sizeof(float) * CurrentCQT.Num());
	    }
	}
	// Clamp the values in the CurrentCQT array between 0 and 1
	Audio::ArrayClampInPlace(CurrentCQT, 0.f, 1.f);
}


void USpectrumProcessor::SupressQuiet(TArray<float>& CurrentCQT, float ScalingFactor)
{
	for (int i = 0; i < CurrentCQT.Num(); i++)
	{
		float CurrentValue = CurrentCQT[i];
		float DistanceFromZero = FMath::Abs(CurrentValue);
		float ScaledValue = FMath::Sign(CurrentValue) * FMath::Pow(DistanceFromZero, ScalingFactor);
		CurrentCQT[i] = ScaledValue;
	}
}


void USpectrumProcessor::ScaleSpectrum(TArray<float>& CurrentCQT, float ScalingFactor)
{
		Audio::ArrayMultiplyByConstantInPlace(CurrentCQT, ScalingFactor);
}

void USpectrumProcessor::ExponentiateSpectrum(TArray<float>& CurrentCQT, float Exponent)
{
			for (int32 i = 0; i < CurrentCQT.Num(); i++)
            {
                CurrentCQT[i] = UKismetMathLibrary::MultiplyMultiply_FloatFloat((CurrentCQT[i]), Exponent);
            }
}

void USpectrumProcessor::ExponentiateFocusedSpectrum(TArray<float>& CurrentCQT, const TArray<bool>& InFocusIndices, float Exponent, float Focus)
{
	float Bonus = Exponent + Focus;

	for (int32 i = 0; i < CurrentCQT.Num(); i++)
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
