#include "BoolPropertyInterface.h"


UBoolPropertyInterface::UBoolPropertyInterface()
{
}

void  UBoolPropertyInterface::Init(UObject* InParent, FBoolProperty* InPropertyPtr, FString InName)
{
    Name = InName;
    PropertyParent = InParent;
    PropertyPtr = InPropertyPtr;


    const FString* ShortNameString = PropertyPtr -> FindMetaData("ShortName");

    ShortName = ShortNameString -> Left(ShortNameString -> Len());

    void *Data = PropertyPtr ->ContainerPtrToValuePtr<void>(PropertyParent);

    Value = PropertyPtr -> GetPropertyValue(Data);
}



void UBoolPropertyInterface::SetValue(bool InValue)
{   

    // float OutputValue = RadialSlider -> Slider -> GetValue(Value);

    void *Data = PropertyPtr -> ContainerPtrToValuePtr<void>(PropertyParent);

    PropertyPtr -> SetPropertyValue(Data,InValue);
}
