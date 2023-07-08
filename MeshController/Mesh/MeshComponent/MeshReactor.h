#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../SoundMesh.h"

#include "FunctionToggle.h"

#include "MeshReactor.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SYNRTCQT_API UMeshReactor : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMeshReactor();


   ASoundMesh* Owner;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


    virtual void ReactOnValue(float InValue, float MaxValue = 0.0f);

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<UFunctionToggle*> FunctionToggles;

};
