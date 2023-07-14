#include "MeshReactor.h"

// Sets default values for this component's properties
UMeshReactor::UMeshReactor()
{


    // for ( TFieldIterator<UFunction> FIT ( GetClass(), EFieldIteratorFlags::IncludeSuper ); FIT; ++FIT) {

    //     UFunction* Function = *FIT;
    // 	FString CurrentNameString = Function->GetName();

	// 	FName CurrentName = FName(*CurrentNameString);

	// 	const FString* Category = Function -> FindMetaData("Category");

	// 	if(Category -> Equals(TEXT("React Function"), ESearchCase::IgnoreCase))
	// 	{
		
	// 		TObjectPtr<UFunctionToggle> CurrentToggle = NewObject<UFunctionToggle>(this, UFunctionToggle::StaticClass(),CurrentName);

	// 		CurrentToggle -> Name = CurrentName;
	// 		CurrentToggle -> Active = false;

	// 		FunctionToggles.Add(CurrentToggle);

	// 	}


    // }


	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UMeshReactor::BeginPlay()
{
	Super::BeginPlay();

	AActor* ActorOwner = GetOwner(); 

	Owner = Cast<ASoundMesh>(ActorOwner);


	// ...
	
}


// Called every frame
void UMeshReactor::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


void UMeshReactor::ReactOnValue(FReactionParams ReactionParams)
{

	struct FFuncParams
    {
        FReactionParams ReactionParams;
    } FuncParams;

	FuncParams.ReactionParams = ReactionParams;


		for(int32 i = 0; i < FunctionToggles.Num(); i++)
		{
			UFunctionToggle* CurrentToggle = FunctionToggles[i];

			if(CurrentToggle -> Active)
			{
				UFunction* function = this->FindFunction(CurrentToggle -> Name);

				if (function)
				{
					this->ProcessEvent(function, &FuncParams);
				}

			} 
		}
}

