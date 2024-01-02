#include "FrequencyDivision.h"
#include "GenericPlatform/GenericPlatformMath.h"

void FFrequencyDivision::Init(int InStartFrequency, int InEndFrequency, int InNumBands, float InBandwidthStretch)
{
    StartFrequency = InStartFrequency;
    EndFrequency = InEndFrequency;
    NumBands = InNumBands;
    BandWidthStretch = InBandwidthStretch;

    GetBandsPerOctave();
}

void FFrequencyDivision::GetBandsPerOctave()
{
    int32 Index = NumBands - 1;
	float LogBase =  FGenericPlatformMath::LogX(10.0f, 2.0f);
	float LogFactor = FGenericPlatformMath::LogX(10.0f,((EndFrequency )/(StartFrequency)));

	float OutBandsPerOctave = (static_cast<float>(Index) * LogBase)/(LogFactor);
    BandsPerOctave = static_cast<int>(OutBandsPerOctave);
}

float FFrequencyDivision::GetConstantQCenterFrequency(const int32 InBandIndex, const float InBaseFrequency, const float InBandsPerOctave)
{


    check(InBandsPerOctave > 0.f);

    float frequency = InBaseFrequency * FMath::Pow(2, static_cast<float>(InBandIndex) / InBandsPerOctave);
    return frequency;
}

float FFrequencyDivision::GetConstantQBandWidth(const float InBandCenter, const float InBandsPerOctave, const float InBandWidthStretch)
{
    check(InBandsPerOctave > 0.f);
    return InBandWidthStretch * InBandCenter * (FMath::Pow(2.f, 1.f / InBandsPerOctave) - 1.f);
}

void FFrequencyDivision::FillDivisionSettings()
{
    GetBandsPerOctave();
    for(int i = 0; i < NumBands; i++)
    {
        FFrequencyBandSettings CurrentBandSettings = FFrequencyBandSettings();
        
        CurrentBandSettings.CenterFreq = GetConstantQCenterFrequency(i, StartFrequency, BandsPerOctave);
        CurrentBandSettings.BandWidth = GetConstantQBandWidth(CurrentBandSettings.CenterFreq, BandsPerOctave, BandWidthStretch);

        DivisionSettings.Add(CurrentBandSettings);
    }
}