// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveSpectrumConfig.h"


void USaveSpectrumConfig::AddSettings(FName SettingsName, FSpectrumAnalyzerSettings NewSettings)
{
    SpectrumSettings.Add(SettingsName, NewSettings);
}

void USaveSpectrumConfig::Empty()
{
	SpectrumSettings.Empty();
}
void USaveSpectrumConfig::SaveSettingsFromProcessor(FName SettingsName, USpectrumProcessor* SpectrumProcessor)
{
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("SettignsName %s"), *SettingsName.ToString());

	FSpectrumAnalyzerSettings Settings = SpectrumSettings.FindOrAdd(SettingsName);

    Settings.SmoothingWindowSize = SpectrumProcessor -> SmoothingWindowSize;
	
	Settings.InterpolationFactor = SpectrumProcessor -> InterpolationFactor;
	
	Settings.ScaleMultiplier = SpectrumProcessor -> ScaleMultiplier;
	
	Settings.PostScaleMultiplier = SpectrumProcessor -> PostScaleMultiplier;

	Settings.QuietMultiplier = SpectrumProcessor -> QuietMultiplier;
	
	Settings. PeakExponentMultiplier = SpectrumProcessor -> PeakExponentMultiplier;
	Settings.FocusExponentMultiplier = SpectrumProcessor -> FocusExponentMultiplier;
	Settings.QuietThreshold = SpectrumProcessor -> QuietThreshold;
	
	SpectrumSettings.Add(SettingsName, Settings);
}

void USaveSpectrumConfig::LoadSettingsFromProcessor(FName SettingsName, USpectrumProcessor* SpectrumProcessor)
{
    UE_LOG(SpectrumSettingsLog, Warning, TEXT("SettignsName %s"), *SettingsName.ToString());

	FSpectrumAnalyzerSettings Settings = SpectrumSettings.FindOrAdd(SettingsName);

    SpectrumProcessor -> SmoothingWindowSize =  Settings.SmoothingWindowSize;
	SpectrumProcessor -> InterpolationFactor =  Settings.InterpolationFactor;
	SpectrumProcessor -> ScaleMultiplier =  Settings.ScaleMultiplier;
	SpectrumProcessor -> PostScaleMultiplier =  Settings.PostScaleMultiplier;
	SpectrumProcessor -> QuietMultiplier =  Settings.QuietMultiplier;
	SpectrumProcessor -> PeakExponentMultiplier =  Settings.PeakExponentMultiplier;
	SpectrumProcessor -> FocusExponentMultiplier =  Settings.FocusExponentMultiplier;
	SpectrumProcessor -> QuietThreshold =  Settings.QuietThreshold;
	
}
	