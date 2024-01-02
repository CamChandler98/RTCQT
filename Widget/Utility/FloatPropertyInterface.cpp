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

    const FString* ShortNameString = PropertyPtr -> FindMetaData("ShortName");

    MinText = MinString -> Left(MinString -> Len());
    MaxText = MaxString -> Left(MaxString -> Len());

    IsMinNumeric = MinString -> IsNumeric();
    IsMaxNumeric = MaxString -> IsNumeric();


    ShortName = ShortNameString -> Left(ShortNameString -> Len());

    
    
    // FString* MinString;
    // MinString -> Append(MinStringPtr, MinStringPtr -> Len());
    // FString* MaxString;

     MinValue = FCString::Atof(*MinText);
     MaxValue = FCString::Atof(*MaxText);



    FVector2D Range = FVector2D(MinValue,MaxValue);

    ControlRange = Range;

    UUserWidget* SpectrumWidget = UUserWidget::CreateWidgetInstance(*this -> GetWorld(), URadialSliderWidget::StaticClass(), WidgetName );
    
    RadialSlider = static_cast<URadialSliderWidget*>(SpectrumWidget);

    RadialSlider -> AddSlider();


    if(!RadialSlider -> Slider)
    {
        // UE_LOG(LogTemp, Error, TEXT("No Slider Found"));
    }
    else
    {

        
        void *Data = PropertyPtr ->ContainerPtrToValuePtr<void>(PropertyParent);
        IsInteger = PropertyPtr -> IsInteger();
        if(IsInteger)
        {

            int64 IntValue = PropertyPtr -> GetSignedIntPropertyValue(Data);
            Value = static_cast<float>(IntValue);
        }
        else
        {
            Value = PropertyPtr -> GetFloatingPointPropertyValue(Data);
        }
        RadialSlider -> Slider -> OnValueChanged.AddDynamic(this, &UFloatPropertyInterface::SetValue);
    }


}



void UFloatPropertyInterface::SetValue(float InValue)
{   

    // float OutputValue = RadialSlider -> Slider -> GetValue(Value);

    void *Data = PropertyPtr ->ContainerPtrToValuePtr<void>(PropertyParent);
    if(PropertyPtr -> IsInteger())
    {
        PropertyPtr->SetIntPropertyValue(Data, static_cast<int64>(InValue));   
    }
    else
    {

        PropertyPtr->SetFloatingPointPropertyValue(Data, InValue);
    }
}


void UFloatPropertyInterface::SyncValue()
{   

    // float OutputValue = RadialSlider -> Slider -> GetValue(Value);

  
        void *Data = PropertyPtr ->ContainerPtrToValuePtr<void>(PropertyParent);
        IsInteger = PropertyPtr -> IsInteger();
        if(IsInteger)
        {

            int64 IntValue = PropertyPtr -> GetSignedIntPropertyValue(Data);
            Value = static_cast<float>(IntValue);
        }
        else
        {
            Value = PropertyPtr -> GetFloatingPointPropertyValue(Data);
        }
        
}
