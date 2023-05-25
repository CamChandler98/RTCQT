// Fill out your copyright notice in the Description page of Project Settings.


#include "RadialSliderWidget.h"

void URadialSliderWidget::AddSlider()
{
    Slider = WidgetTree -> ConstructWidget<UAudioRadialSlider>();
}