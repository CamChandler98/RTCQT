// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectrumProcessorSettings.h"

DEFINE_LOG_CATEGORY(SpectrumSettingsLog);

void FSpectrumToggles::SetToggle(FName InPropertyName, bool InValue)
{
    UScriptStruct* SpectrumToggleClass = FSpectrumToggles::StaticStruct();
	
	FProperty* Property = SpectrumToggleClass -> FindPropertyByName(InPropertyName);

	TObjectPtr<FBoolProperty> BoolProperty = static_cast<FBoolProperty*>(Property);

    void *Data = Property->ContainerPtrToValuePtr<void>(this);

	BoolProperty -> SetPropertyValue(Data, InValue);
    
}

USpectrumSettings::USpectrumSettings()
{
}

void USpectrumSettings::LogParams()
{
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("SmoothingWindow: %d"), SmoothingWindowSize);
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("InterpolationFactor: %f"), InterpolationFactor);
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("ScaleMultiplier: %f"), ScaleMultiplier);
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("QuietMultiplier: %f"), QuietMultiplier);
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("PeakExponentMultiplier: %f"), PeakExponentMultiplier);
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("FocusExponentMultiplier: %f"), FocusExponentMultiplier);

}