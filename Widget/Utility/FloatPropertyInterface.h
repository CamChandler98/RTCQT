#pragma once

#include "CoreMinimal.h"
#include "AudioRadialSlider.h"
#include "../RadialSliderWidget.h"
#include "FloatPropertyInterface.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UFloatPropertyInterface : public UObject
{

GENERATED_BODY()
public:

	UFloatPropertyInterface();

    FString Name;

    UObject* PropertyParent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere,  Instanced)
    URadialSliderWidget* RadialSlider;

    FNumericProperty* PropertyPtr;
    
    void Init(UObject* InParent, FNumericProperty* InPropertyPtr, FString InName);



    UFUNCTION()
    void SetValue(float Value);
	
};