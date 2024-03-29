// Fill out your copyright notice in the Description page of Project Settings.


#include "Sampler.h"

// Sets default values for this component's properties
USampler::USampler()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

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


TArray<float> USampler::ConvertPCMToFloat(const TArray<uint8> InterleavedStream, int NumChannels, int BitsPerSample, float GainFactor, bool DoGain, bool DownMix, bool DirectFloat, bool AltFloat)
{

    CanProcess= false;

    int32 SampleSize = BitsPerSample / 8;

	NumChannels = DownMix ? NumChannels : 1;
    int32 NumSamples = InterleavedStream.Num() / (NumChannels * SampleSize);

    TArray<float> SummedSamples;

    SummedSamples.Reserve(NumSamples);
	for (int32 i = 0; i < NumSamples; i++)
		{
			// Compute the offset of the current sample
			int32 SampleOffset = i * NumChannels * SampleSize;

			// Sum the sample values for each channel
			float SampleSum = 0.f;
			if(DownMix)
			{
				for (int32 j = 0; j < NumChannels; j++)
				{
					// Compute the offset of the current channel within the sample
					int32 ChannelOffset = j * SampleSize;

				
					if(DirectFloat)
					{

						float SampleValue = 0.0f;
						FMemory::Memcpy(&SampleValue, &InterleavedStream[SampleOffset + ChannelOffset], sizeof(float));
						SampleSum += SampleValue * GainFactor;
					
					}
					else if(AltFloat)
					{

						uint8 SampleValue = 0;
						for (int32 k = 0; k < SampleSize; k++)
						{
							SampleValue += InterleavedStream[SampleOffset + ChannelOffset + k] << (8 * k);
						}

						// Convert the uint8 sample value to float and normalize it to the range [-1, 1]

						float NormalizedSampleValue =( static_cast<float>(SampleValue) * GainFactor )/ 127.5f - 1.0f;
						SampleSum += NormalizedSampleValue;
					}
					else
					{

						float SampleValue = 0.f;
						for (int32 k = 0; k < SampleSize; k++)
						{
							SampleValue += InterleavedStream[SampleOffset + ChannelOffset + k] << (8 * k);
						}

						SampleValue *= GainFactor;

						// Normalize the sample value to the range [-1, 1]
						SampleValue /= ((1 << (BitsPerSample - 1)) - 1);

						// Add the sample value for the current channel to the sum
						SampleSum += SampleValue;
					}
					
				}
			}
			else
			{
				// float SampleValue = (static_cast<float>(InterleavedStream[i]) * GainFactor) / 32767.0f;
				float SampleValue = (static_cast<float>(InterleavedStream[SampleOffset]) * GainFactor) / 32767.0f;


				// float SampleValue = (static_cast<float>(InterleavedStream[i]) * GainFactor) / 127.5f - 1.0f;

				// float SampleValue = (static_cast<float>(InterleavedStream[i]) * GainFactor) / ((1 << (BitsPerSample - 1)) - 1);


				SampleSum = SampleValue;
			}
			// Store the sum of the sample values for all channels in the output array
			SummedSamples.Add(SampleSum);
		}
		
	OutAudioData = SummedSamples;

	return SummedSamples;
}

TArray<float> USampler::UnrealPCMToFloat(const TArray<uint8> InterleavedStream, int32 NumChannels, float Gain)
{
    int32 NumSamples = InterleavedStream.Num()/8;

    TArray<float> FloatSamples;

	FloatSamples.AddUninitialized(NumSamples);

	for (int32 SampleIndex = 0; SampleIndex < NumSamples; SampleIndex++)
	{
	
		FloatSamples[SampleIndex] = ((static_cast<float>(InterleavedStream[SampleIndex]) * Gain) / 32767.0f) ;

	}

	return FloatSamples;
}


TArray<float> USampler::GetAudioData()
{
	return OutAudioData;
}

