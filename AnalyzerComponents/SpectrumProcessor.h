// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectrumProcessorSettings.h"
#include "SpectrumProcessor.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API USpectrumProcessor : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USpectrumProcessor();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<float> PreviousCQT;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Array" )
	TArray<bool> FocusIndices;
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing" )
	int32 SmoothingWindowSize = 7;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ClampMax = "1.0") , Category = "Spectrum Processing")
	float InterpolationFactor = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float ScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float QuietMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float PeakExponentMultiplier = 1.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float FocusExponentMultiplier = .01;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
	float NoiseFloorDB = -60;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doInterpolate = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doCubicInterpolation = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doSmooth = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doFocusExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing")
	bool doPeakExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doNormalize = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doScale = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doSupressQuiet = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing")
	bool doClamp = true;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" )
    TObjectPtr<USpectrumSettings> Settings;

	void SetSettings(USpectrumSettings* InSettings);
    void SetParams();

	void ProcessSpectrum(TArray<float>& CurrentCQT, FSpectrumToggles Toggles);

	void InterpolateSpectrum(TArray<float>& CurrentCQT, bool bDoCubicInterpolation = true);

	void SmoothSpectrum(TArray<float>& CurrentCQT);

	void NormalizeSpectrum(TArray<float>& CurrentCQT, float InNoiseFloorDB);

	void SupressQuiet(TArray<float>& CurrentCQT, float ScalingFactor);


	void ScaleSpectrum(TArray<float>& CurrentCQT, float ScalingFactor);

	void ExponentiateSpectrum(TArray<float>& CurrentCQT, float Exponent);

	void ExponentiateFocusedSpectrum(TArray<float>& CurrentCQT, const TArray<bool>& InFocusIndices, float Exponent, float Focus);

};
