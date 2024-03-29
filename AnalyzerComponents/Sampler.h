// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sampler.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API USampler : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USampler();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool CanProcess  = true;

	UFUNCTION(BlueprintCallable)
	TArray<float> ConvertPCMToFloat(const TArray<uint8> InterleavedStream, int NumChannels, int BitsPerSample, float GainFactor, bool DoGain = true, bool DownMix = true, bool DirectFloat = false, bool AltFloat = false);

	UFUNCTION(BlueprintCallable)
	TArray<float> UnrealPCMToFloat(const TArray<uint8> InterleavedStream, int NumChannels, float Gain);
	
	UFUNCTION(BlueprintCallable)
	TArray<float> GetAudioData();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> OutAudioData;



protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
