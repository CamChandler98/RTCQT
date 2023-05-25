// Fill out your copyright notice in the Description page of Project Settings.


#include "SampleProcessorSettings.h"

void FSampleToggles::SetToggle(FName InPropertyName, bool InValue)
{
    UScriptStruct* SampleToggleClass = FSampleToggles::StaticStruct();
	
	FProperty* Property = SampleToggleClass -> FindPropertyByName(InPropertyName);

	TObjectPtr<FBoolProperty> BoolProperty = static_cast<FBoolProperty*>(Property);

    void *Data = Property->ContainerPtrToValuePtr<void>(this);

	BoolProperty -> SetPropertyValue(Data, InValue);
    
}


USampleSettings::USampleSettings()
{
}

