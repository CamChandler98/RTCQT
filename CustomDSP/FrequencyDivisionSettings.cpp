#include "FrequencyDivisionSettings.h"
#include "GenericPlatform/GenericPlatformMath.h"


UFrequencyDivisionSettings::UFrequencyDivisionSettings()
{

}

UStartEndFreqs:: UStartEndFreqs()
{

}
void UFrequencyDivisionSettings::GetCenterFrequencies()
{
    for(int i = 0 ; i < ProportionArray.Num(); i++)
    {   
        UStartEndFreqs* CurrentFrequencyBand = ProportionArray[i];

        float CurrentStartFrequency =  CurrentFrequencyBand -> StartFrequency;
        float CurrentEndFrequency = CurrentFrequencyBand -> EndFrequency;
        float CurrentProportion = CurrentFrequencyBand -> Proportion;
        int CurrentNumBands = FGenericPlatformMath::FloorToInt(static_cast<float>(NumBands) * CurrentProportion);

        FFrequencyDivision CurrentDivision = FFrequencyDivision();

        CurrentDivision.Init(CurrentStartFrequency,CurrentEndFrequency,CurrentNumBands, BandwidthStretch);

        CurrentDivision.FillDivisionSettings();

        CombinedCenterFrequencies.Append(CurrentDivision.DivisionSettings);
    }
}