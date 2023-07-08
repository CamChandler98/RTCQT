#include "./MeshReactor.h"

#include "ScaleReactor.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API UScaleReactor : public UMeshReactor
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UScaleReactor();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame

    float InverseScaleMin = 0;
    float InverseScaleMax = 2; 

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


    virtual void ReactOnValue(float InValue, float MaxValue = 0.0f);


    void ScaleZ(float InValue);

    void ScaleAll(float InValue);

    void InverseScaleXY(float InValue, float MaxValue, float MinScale, float MaxScale,  bool bXOnly = false);

};
