// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CQTAnalyzer.generated.h"

USTRUCT(BlueprintType)

struct FAnalyzerParameters
{	    
		GENERATED_BODY();

		public:
		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float Proportion;

	   	UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float BaseFrequency;

		UPROPERTY(BlueprintReadWrite, EditAnywhere)
		float EndFrequency;


		int NumBands;

		float BandsPerOctave;


		FAnalyzerParameters()
		:Proportion(0.f)
		,BaseFrequency(40.f)
		,EndFrequency(6000.0f)
		,NumBands(96)
		,BandsPerOctave(12)
		{

		}
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API UCQTAnalyzer : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCQTAnalyzer();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
