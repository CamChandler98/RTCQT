// Fill out your copyright notice in the Description page of Project Settings.


#include "SampleProcessor.h"

// Sets default values for this component's properties
USampleProcessor::USampleProcessor()
{

	 LowPassFilter = MakeUnique<Audio::FBiquadFilter>();
	 HighPassFilter = MakeUnique<Audio::FBiquadFilter>();
	 LowShelfFilter = MakeUnique<Audio::FBiquadFilter>();
	 HighShelfFilter = MakeUnique<Audio::FBiquadFilter>();
	 Settings =  CreateDefaultSubobject<USampleSettings>(TEXT("Settings"));

	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USampleProcessor::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void USampleProcessor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void USampleProcessor::GetFilters(float SampleRate)
{
	HighPassFilter -> Init(SampleRate, 1, Audio::EBiquadFilter::Highpass, HighPassCutoffFrequency, HighPassBandWidth, HighPassGain );
	HighShelfFilter -> Init(SampleRate, 1, Audio::EBiquadFilter::HighShelf, HighPassCutoffFrequency, HighPassBandWidth, HighPassGain );
    LowPassFilter -> Init(SampleRate, 1, Audio::EBiquadFilter::LowShelf, LowPassCutoffFrequency , LowPassBandWidth, LowPassGain);
    LowPassFilter -> Init(SampleRate, 1, Audio::EBiquadFilter::Lowpass, LowPassCutoffFrequency , LowPassBandWidth, LowPassGain);
}

void USampleProcessor::SetSettings(USampleSettings* InSettings)
{
	
	 Settings -> HighPassCutoffFrequency = InSettings -> HighPassCutoffFrequency;
	
	 Settings -> HighPassBandWidth  = InSettings -> HighPassBandWidth;
	
	 Settings -> HighPassGain = InSettings -> HighPassGain;
	
	 Settings -> LowPassCutoffFrequency  = InSettings -> LowPassCutoffFrequency; 
	
	 Settings -> LowPassBandWidth = InSettings -> LowPassBandWidth;
	
	 Settings -> LowPassGain  = InSettings -> LowPassGain;
	
	 Settings -> GainFactor = InSettings -> GainFactor;
}


void USampleProcessor::SetParams()
{
	
	HighPassCutoffFrequency = Settings -> HighPassCutoffFrequency ; 
	
	HighPassBandWidth = Settings -> HighPassBandWidth ;
	
	HighPassGain = Settings -> HighPassGain;
	
	LowPassCutoffFrequency = Settings -> LowPassCutoffFrequency ; 
	
	LowPassBandWidth = Settings -> LowPassBandWidth;
	
	LowPassGain = Settings -> LowPassGain ;
	
	GainFactor = Settings -> GainFactor;
}

void USampleProcessor::UpdateLowPassFilter()
{
	LowPassFilter -> SetParams(Audio::EBiquadFilter::Lowpass, LowPassCutoffFrequency, LowPassBandWidth, LowPassGain);
	
}

void USampleProcessor::UpdateHighPassFilter()
{
	HighPassFilter -> SetParams(Audio::EBiquadFilter::Highpass, HighPassCutoffFrequency, HighPassBandWidth, HighPassGain);

}

void USampleProcessor::UpdateLowShelfFilter()
{

}

void USampleProcessor::UpdateHighShelfFilter()
{

}


void USampleProcessor::ProcessAudio(TArray<float>& AudioData)


{	
	if(Toggles -> DoLowPass)
	{
		
		TArrayView<const float> AudioView(AudioData.GetData(), AudioData.Num());
		LowPassFilter -> ProcessAudio(AudioView.GetData(), AudioView.Num(), AudioData.GetData());      
	}
	if(Toggles -> DoLowShelf)
	{

		TArrayView<const float> AudioView(AudioData.GetData(), AudioData.Num());
		LowShelfFilter -> ProcessAudio(AudioView.GetData(), AudioView.Num(), AudioData.GetData());   
	}
	if(Toggles -> DoHighPass)
	{

		TArrayView<const float> AudioView(AudioData.GetData(), AudioData.Num());
		HighPassFilter -> ProcessAudio(AudioView.GetData(), AudioView.Num(), AudioData.GetData());
	}
	if(Toggles -> DoHighShelf)
	{

		TArrayView<const float> AudioView(AudioData.GetData(), AudioData.Num());
		HighShelfFilter -> ProcessAudio(AudioView.GetData(), AudioView.Num(), AudioData.GetData());
	}
	if(GainFactor != 1)
	{
		Audio::ArrayMultiplyByConstantInPlace(AudioData, GainFactor); 
	}
	if(Toggles -> DoAbsAmp)
	{

		Audio::ArrayAbsInPlace(AudioData);
	}
	
	Audio::ArrayMultiplyByConstantInPlace(AudioData, 1.f / FMath::Sqrt(static_cast<float>(2))); 
}