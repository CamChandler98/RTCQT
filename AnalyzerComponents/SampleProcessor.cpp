// Fill out your copyright notice in the Description page of Project Settings.


#include "SampleProcessor.h"

// Sets default values for this component's properties
USampleProcessor::USampleProcessor()
{

	 LowPassFilter = MakeUnique<Audio::FBiquadFilter>();
	 HighPassFilter = MakeUnique<Audio::FBiquadFilter>();
	 LowShelfFilter = MakeUnique<Audio::FBiquadFilter>();
	 HighShelfFilter = MakeUnique<Audio::FBiquadFilter>();
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