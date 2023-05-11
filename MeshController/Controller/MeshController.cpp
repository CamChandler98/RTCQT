// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AMeshController::AMeshController()

{

	#if WITH_EDITOR
	#define LOCTEXT_NAMESPACE "Custom Detail"
	static const FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);

	//Change "Actor" for the type of your Class(eg. Actor, Pawn, CharacterMovementComponent)
	//Change "MySection" to the name of Desired Section
	TSharedRef<FPropertySection> Section = PropertyModule.FindOrCreateSection("Actor", "MySection", LOCTEXT("MySection", "MySection"));

	//You can add multiples categories to be tracked by this section
	Section->AddCategory("Functions");
	#undef LOCTEXT_NAMESPACE
	#endif

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>("Root Scene Component");
	RootComponent = SceneRootComponent;;
	FString ColorString = FString(TEXT("R=223,G=0,B=0,A=1"));
	Color.InitFromString(ColorString);
	
	Arrow = CreateDefaultSubobject<UArrowComponent>("Arrow");
	Arrow -> bIsScreenSizeScaled = false;
	Arrow -> bIsEditorOnly = true;
	Arrow -> ArrowLength = CalculateTotalLength(NumberOfMeshes, MeshScale, Padding);
	Arrow -> ArrowColor = Color.QuantizeRound();

	Arrow -> SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void AMeshController::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMeshController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMeshController::SpawnMeshes(int32 Number)
{

}

void AMeshController::SpawnMeshesInLine(int32 Number, float InPadding)
{
	FVector ForwardVector = this -> GetActorForwardVector();
	FVector Location = this -> GetActorLocation();
	FVector SpawnScale(MeshScale);
	FQuat SpawnRotation = GetActorQuat();

	ESpawnActorCollisionHandlingMethod Collision = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector PaddingVector(Padding);
	FActorSpawnParameters Parameters = FActorSpawnParameters(); 
	for(int32 i = 1; i <= Number; i++)
	{
		FVector IndexVector(static_cast<float>(i));

		FVector Spacing = UKismetMathLibrary::Multiply_VectorVector(IndexVector,PaddingVector);

		FVector ForwardSpacing = UKismetMathLibrary::Multiply_VectorVector(ForwardVector,Spacing);

		FVector SpawnLocation = UKismetMathLibrary::Add_VectorVector(ForwardSpacing,Location);


		FTransform Transform(SpawnRotation, SpawnLocation,SpawnScale);

		ASoundMesh* CurrentMesh = GetWorld()->SpawnActorDeferred<ASoundMesh>(

			ASoundMesh::StaticClass()
			, Transform
			, this
			, nullptr
			, Collision
			);

			CurrentMesh -> AttachToActor(this, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));

			CurrentMesh -> Init(Mesh, Material, SpawnScale);

			CurrentMesh -> SetColor(Color);
			
			VisualizationMeshes.Add(CurrentMesh);

			UGameplayStatics::FinishSpawningActor(CurrentMesh, Transform);
	}
}

void AMeshController::ChangePadding(float InPadding)
{
	if(InPadding == Padding){
		return;
	}

	float Diff = FMath::Abs(InPadding - Padding);

	bool doIncrease = InPadding > Padding;


	for(int32 i = 0; i < VisualizationMeshes.Num(); i++)
	{
		UStaticMeshComponent* CurrentMesh = VisualizationMeshes[i] -> GetStaticMeshComponent();

		FVector CurrentLocation = CurrentMesh -> GetRelativeLocation();
		FVector NewLocation = CurrentLocation;


		float CurrentXLocation = CurrentLocation.X;
		float Offset = Diff * i;

		// UE_LOG(LogTemp, Warning, TEXT("index: %d"), i);
		// UE_LOG(LogTemp, Warning, TEXT("offset: %f"), Off);




		if(doIncrease)
		{

			float NewXLocation = CurrentXLocation + Offset;
			NewLocation.X = NewXLocation;

		}
		else
		{

			float NewXLocation = CurrentXLocation - Offset;
			NewLocation.X = NewXLocation;

		}

		CurrentMesh -> SetRelativeLocation(NewLocation);
	}

	Padding = InPadding;
}

void AMeshController::UpdateMesh(int32 Index, double Value)
{

}

void AMeshController::TestSpawn()
{
	SpawnMeshesInLine(NumberOfMeshes, Padding);
}

void AMeshController::TestIncreasePadding()
{
	float NewPadding = Padding + 25;
	ChangePadding(NewPadding);
}

void AMeshController::TestDecreasePadding()
{
	float NewPadding = Padding - 25;
	ChangePadding(NewPadding);

}

float AMeshController::CalculateTotalLength(int NumberOfObjects, float Scale, float InPadding)
{
    // Assuming ObjectScale.Z is the length of the object
    float TotalLength = NumberOfObjects * Scale;

    // Add padding. There will be one less padding space than the number of objects.
    if (NumberOfObjects > 1) 
    {
        TotalLength += Padding * (NumberOfObjects - 1);
    }

    return TotalLength;
}