#pragma once

#include "CoreMinimal.h"
#include "BoolPropertyInterface.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UBoolPropertyInterface : public UObject
{

GENERATED_BODY()
public:

	UBoolPropertyInterface();


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ShortName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool Value;

    UObject* PropertyParent;

    FBoolProperty* PropertyPtr;
    

    void Init(UObject* InParent, FBoolProperty* InPropertyPtr, FString InName);

    UFUNCTION(BlueprintCallable)
    void SetValue(bool InValue);
	
};