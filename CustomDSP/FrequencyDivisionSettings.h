#pragma once
#include "CoreMinimal.h"
#include "FrequencyDivision.h"
#include "FrequencyDivisionSettings.generated.h"

UCLASS(Blueprintable, EditInlineNew)

class SYNRTCQT_API UStartEndFreqs : public UObject
{
    GENERATED_BODY()

    public:
    UStartEndFreqs();
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float StartFrequency;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float EndFrequency;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    float Proportion;
};

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UFrequencyDivisionSettings  : public UObject
{
    GENERATED_BODY()


    public:
    UFrequencyDivisionSettings();
    UPROPERTY(EditAnywhere, Instanced)
    TArray<UStartEndFreqs*> ProportionArray;

    UPROPERTY(EditAnywhere)
    TArray<FFrequencyDivision> FrequencyDivisionArray;

    UPROPERTY(EditAnywhere)
    int NumBands;

    UPROPERTY(EditAnywhere)
    float BandwidthStretch;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    TArray<FFrequencyBandSettings> CombinedCenterFrequencies;

    
    UFUNCTION(BlueprintCallable)
    void GetCenterFrequencies();

  
};