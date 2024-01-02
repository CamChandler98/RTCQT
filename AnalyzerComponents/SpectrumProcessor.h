// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpectrumProcessorSettings.h"
#include "../Widget/Utility/FloatPropertyInterface.h"
#include "../Widget/Utility/BoolPropertyInterface.h"

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
	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing",  meta = (ClampMin = 0, ClampMax = 15, ShortName = "Smoothing"))
	int32 SmoothingWindowSize = 2;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,  Category = "Spectrum Processing", meta = (ClampMin = 0.0, ClampMax = 1.0, ShortName = "Interp"))
	float InterpolationFactor = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing",  meta = (ClampMin = 0.0, ClampMax = 8.0, ShortName = "Scale"))
	float ScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing",  meta = (ClampMin = 0.0, ClampMax = 16.0, ShortName = "PostScale"))
	float PostScaleMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" , meta = (ClampMin = -1.0, ClampMax = 10.0, ShortName = "Quiet"))
	float QuietMultiplier = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" , meta = (ClampMin = 1.0, ClampMax = 8.0, ShortName = "Peak"))
	float PeakExponentMultiplier = 1.25;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" , meta = (ClampMin = 0.0, ClampMax = 1.0, ShortName = "Focus"))
	float FocusExponentMultiplier = .01;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" , meta = (ClampMin = 0.0, ClampMax = 1.0, ShortName = "QLimit"))
	float QuietThreshold = .5;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" , meta = (ClampMin = 0.0, ClampMax = 1.0, ShortName = "HiBoost"))
	float HighFrequencyBoostFactor = .5 ;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing",  meta = (ClampMin = -80.0, ClampMax = 80.0, ShortName = "NoiseFloor"))
	float NoiseFloorDB = -60;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Interpolate")) 
	bool doInterpolate = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "CubicInterp")) 
	bool doCubicInterpolation = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Smooth")) 
	bool doSmooth = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Median")) 
	bool doMedianSmooth = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Focus")) 
	bool doFocusExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing", meta = (ShortName = "Peak")) 
	bool doPeakExp = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Norm")) 
	bool doNormalize = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Scale")) 
	bool doScale = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "PostScale")) 
	bool doPostScale = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Quiet")) 
	bool doSupressQuiet = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere ,Category = "Spectrum Processing", meta = (ShortName = "Clamp")) 
	bool doClamp = true;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spectrum Processing" , Instanced)
    TObjectPtr<USpectrumSettings> Settings;

	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Sample Processor", Instanced)
	TObjectPtr<USpectrumToggles> Toggles;


	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing",  Instanced)
	TArray<UFloatPropertyInterface*> WidgetInterfaces;


	UPROPERTY(BlueprintReadWrite, EditAnywhere , Category = "Spectrum Processing",  Instanced)
	TArray<UBoolPropertyInterface*> ToggleInterfaces;

	void SetSettings(USpectrumSettings* InSettings);
    void SetParams();
	void SetArraySize(int32 InSize);

	void ProcessSpectrum(TArray<float>& CurrentCQT);

	void InterpolateSpectrum(TArray<float>& CurrentCQT, bool bDoCubicInterpolation = true);

	void SmoothSpectrum(TArray<float>& CurrentCQT);

	void MedianSmoothSpectrum(TArray<float>& CurrentCQT);

	void NormalizeSpectrum(TArray<float>& CurrentCQT, float InNoiseFloorDB);

	void SupressQuiet(TArray<float>& CurrentCQT, float ScalingFactor);


	void ScaleSpectrum(TArray<float>& CurrentCQT, float ScalingFactor);

	void PostScaleSpectrum(TArray<float>& CurrentCQT, float ScalingFactor);

	void ExponentiateSpectrum(TArray<float>& CurrentCQT, float Exponent);

	void ExponentiateFocusedSpectrum(TArray<float>& CurrentCQT, TArray<bool>& InFocusIndices, float Exponent, float Focus);

};
