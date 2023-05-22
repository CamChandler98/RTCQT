// Fill out your copyright notice in the Description page of Project Settings.


#include "Sampler.h"

// Sets default values for this component's properties
USampler::USampler()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void USampler::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void USampler::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void USampler::ConvertPCMToFloat(const TArray<uint8> InterleavedStream, int NumChannels, int BitsPerSample, float GainFactor)
{
    int SampleSize = BitsPerSample / 8;

    int NumSamples = InterleavedStream.Num() / (NumChannels * SampleSize);

    TArray<float> SummedSamples;

    SummedSamples.Reserve(NumSamples);
	for (int i = 0; i < NumSamples; i++)
		{
			// Compute the offset of the current sample
			int SampleOffset = i * NumChannels * SampleSize;

			// Sum the sample values for each channel
			float SampleSum = 0.f;
			for (int j = 0; j < NumChannels; j++)
			{
				// Compute the offset of the current channel within the sample
				int ChannelOffset = j * SampleSize;

				// Extract the sample value for the current channel
	
					float SampleValue = 0.0f;
					FMemory::Memcpy(&SampleValue, &InterleavedStream[SampleOffset + ChannelOffset], sizeof(float));

					// Convert the uint8 sample value to float and normalize it to the range [-1, 1]
					// float NormalizedSampleValue = static_cast<float>(SampleValue ) / 127.5f - 1.0f;


					// Add the sample value for the current channel to the sum
					SampleSum += SampleValue;
				
			}

			// Store the sum of the sample values for all channels in the output array
			SummedSamples.Add(SampleSum);
		}
		
	OutAudioData = SummedSamples;
}

TArray<float> USampler::GetAudioData()
{
	return OutAudioData;
}

