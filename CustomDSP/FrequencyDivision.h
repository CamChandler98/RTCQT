#pragma once
#include "CoreMinimal.h"
#include "../CoreDSP/ConstantQ.h"
#include "FrequencyDivision.generated.h"

    USTRUCT(BlueprintType)
    struct FFrequencyBandSettings
    {
        GENERATED_BODY();

        UPROPERTY()
        float CenterFreq;
        UPROPERTY()
        float BandWidth;

        FFrequencyBandSettings()
        {   
            CenterFreq = 0;
            BandWidth = 0;
        }
    
    };
    
    USTRUCT(BlueprintType)
    struct FFrequencyDivision
    {

        GENERATED_BODY();

        // FName Name;
        int StartFrequency;
        int EndFrequency;
        int BandsPerOctave;
        int NumBands;
        float BandWidthStretch;

        TArray<FFrequencyBandSettings> DivisionSettings;


        void Init(int InStartFrequency, int InEndFrequency, int InNumBands, float InBandwidthStretch);

        void FillDivisionSettings();

        void GetBandsPerOctave();

        float GetConstantQCenterFrequency(const int32 InBandIndex, const float InBaseFrequency, const float InBandsPerOctave);

        float GetConstantQBandWidth(const float InBandCenter, const float InBandsPerOctave, const float InBandWidthStretch);

    };
    