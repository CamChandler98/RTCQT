#include "CoreMinimal.h"
#include "AudioRadialSlider.h"

#include "FloatPropertyInterface.generated.h"

UCLASS(Blueprintable, EditInlineNew)
class SYNRTCQT_API UFloatPropertyInterface : public UObject
{

GENERATED_BODY()
public:

	UFloatPropertyInterface();

    FString Name;

    UObject* PropertyParent;

    UAudioRadialSlider* RadialSlider;

    FFloatProperty* PropertyPtr;

    void Init(UObject* InParent, FFloatProperty* InPropertyPtr, FString InName);
    void SetValue(float Value);
	
};