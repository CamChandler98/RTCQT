#include "FloatPropertyInterface.h"


UFloatPropertyInterface::UFloatPropertyInterface()
{
}

void  UFloatPropertyInterface::Init(UObject* InParent, FFloatProperty* InPropertyPtr, FString InName)
{

    Name = InName;
    PropertyParent = InParent;

    PropertyPtr = InPropertyPtr;

    const FString* MinString = PropertyPtr -> FindMetaData("ClampMin");
    const FString* MaxString = PropertyPtr -> FindMetaData("ClampMax");

    // FString* MinString;
    // MinString -> Append(MinStringPtr, MinStringPtr -> Len());
    // FString* MaxString;

    float MinValue = FCString::Atof(**MinString);
    float MaxValue = FCString::Atof(**MaxString);


    FVector2D Range = FVector2D(MinValue,MaxValue);

    RadialSlider -> OutputRange = Range;

    RadialSlider -> OnValueChanged.AddDynamic(this, &UFloatPropertyInterface::SetValue);

}

void UFloatPropertyInterface::SetValue(float Value)
{
    void *Data = PropertyPtr ->ContainerPtrToValuePtr<void>(PropertyParent);
    PropertyPtr->SetFloatingPointPropertyValue(Data, Value);
}
