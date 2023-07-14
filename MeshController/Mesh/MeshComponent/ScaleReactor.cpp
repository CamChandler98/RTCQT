#include "ScaleReactor.h"



UScaleReactor::UScaleReactor()
{



	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UScaleReactor::BeginPlay()
{
	Super::BeginPlay();

	AActor* ActorOwner = GetOwner(); 

	Owner = Cast<ASoundMesh>(ActorOwner);


	// ...
	
}


// Called every frame
void UScaleReactor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UScaleReactor::ReactOnValue(FReactionParams ReactionParams)
{
    Super::ReactOnValue(ReactionParams);

}

void UScaleReactor::ScaleAll(float InValue)
{
    FVector NewScale = Owner -> MeshComponent -> GetRelativeScale3D();

    NewScale.Z = InValue;
    NewScale.X = InValue;
    NewScale.Y = InValue;
    
    Owner -> MeshComponent -> SetRelativeScale3D(NewScale);
}

void UScaleReactor::ScaleZ(float InValue)
{
    FVector NewScale = Owner -> MeshComponent -> GetRelativeScale3D();

    NewScale.Z = InValue;
    
    Owner -> MeshComponent -> SetRelativeScale3D(NewScale);
}

void UScaleReactor::InverseScaleXY(float InValue, float MaxValue, float MinScale, float MaxScale, bool bXOnly)
{
    FVector NewScale = Owner -> MeshComponent -> GetRelativeScale3D();

    float NewValue = FMath::Clamp(InValue, MaxScale, MinScale);

    NewScale.X = InValue;
    
    
    if(!bXOnly)
    {
        NewScale.Y = InValue;
    }

    Owner -> MeshComponent -> SetRelativeScale3D(NewScale);

}