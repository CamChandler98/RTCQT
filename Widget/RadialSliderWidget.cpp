// Fill out your copyright notice in the Description page of Project Settings.


#include "RadialSliderWidget.h"

void URadialSliderWidget::AddSlider()
{
    Slider = WidgetTree -> ConstructWidget<URadialSlider>();
    Slider -> SetSliderHandleStartAngle(49.0f);
    Slider -> SetSliderHandleEndAngle(312.0f);
    Slider -> SetAngularOffset(359.5f);
    Slider -> SetStepSize(.0001f);

}