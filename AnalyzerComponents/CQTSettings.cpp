// Fill out your copyright notice in the Description page of Project Settings.


#include "CQTSettings.h"
// #include "AudioSynesthesiaLog.h"


UCQTSettings::UCQTSettings()
: Proportion(1.0f)
, EndingFrequency(6000.0)
{
}

#if WITH_EDITOR
FText UCQTSettings::GetAssetActionName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AssetSoundSynesthesiaConstantQNRTSettings", "Synesthesia NRT Settings (ConstantQ)");
}

UClass* UCQTSettings::GetSupportedClass() const
{
	return UConstantQNRTSettings::StaticClass();
}
#endif