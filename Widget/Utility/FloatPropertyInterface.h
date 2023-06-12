#pragma once

#include "CoreMinimal.h"
// #include "AudioRadialSlider.h"
#include "../RadialSliderWidget.h"
#include "FloatPropertyInterface.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UFloatPropertyInterface : public UObject
{

GENERATED_BODY()
public:

	UFloatPropertyInterface();


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Name;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ShortName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString MinText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString MaxText;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IsMinNumeric;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IsMaxNumeric;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float MinValue;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float MaxValue;

    UObject* PropertyParent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IsInteger;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D ControlRange;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Value;


	UPROPERTY(BlueprintReadWrite, EditAnywhere,  Instanced)
    URadialSliderWidget* RadialSlider;

    FNumericProperty* PropertyPtr;
    
    void Init(UObject* InParent, FNumericProperty* InPropertyPtr, FString InName);



    UFUNCTION(BlueprintCallable)
    void SetValue(float InValue);
	
};