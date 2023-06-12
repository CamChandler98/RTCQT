// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/RadialSlider.h"

// #include "AudioRadialSlider.h"

#include "RadialSliderWidget.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API URadialSliderWidget : public UUserWidget
{
	GENERATED_BODY()

	public:

	void AddSlider();
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class URadialSlider* Slider;

};
