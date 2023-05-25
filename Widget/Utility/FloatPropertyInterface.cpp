#include "FloatPropertyInterface.h"


UFloatPropertyInterface::UFloatPropertyInterface()
{
}

void  UFloatPropertyInterface::Init(UObject* InParent, FNumericProperty* InPropertyPtr, FString InName)
{

    Name = InName;
    FName WidgetName = FName(*InName);
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

    UUserWidget* SpectrumWidget = UUserWidget::CreateWidgetInstance(*this -> GetWorld(), URadialSliderWidget::StaticClass(), WidgetName );
    
    RadialSlider = static_cast<URadialSliderWidget*>(SpectrumWidget);

    RadialSlider -> AddSlider();


    if(!RadialSlider -> Slider)
    {
        UE_LOG(LogTemp, Error, TEXT("No Slider Found"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Slider is here"));
        RadialSlider -> Slider -> SetOutputRange(Range);
        RadialSlider -> Slider -> OnValueChanged.AddDynamic(this, &UFloatPropertyInterface::SetValue);
    }


}



void UFloatPropertyInterface::SetValue(float Value)
{   

    float OutputValue = RadialSlider -> Slider -> GetOutputValue(Value);
    void *Data = PropertyPtr ->ContainerPtrToValuePtr<void>(PropertyParent);
    if(PropertyPtr -> IsInteger())
    {
        PropertyPtr->SetIntPropertyValue(Data, static_cast<int64>(OutputValue));   
    }
    else
    {

        PropertyPtr->SetFloatingPointPropertyValue(Data, OutputValue);
    }
}
