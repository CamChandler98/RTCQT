#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MeshAgent.generated.h"


UCLASS()
class SYNRTCQT_API AMeshAgent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeshAgent();
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// virtual void PreEditChange(FProperty* ChangedProperty) override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
