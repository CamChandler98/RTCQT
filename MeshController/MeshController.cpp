// Fill out your copyright notice in the Description page of Project Settings.


#include "MeshController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

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
	PrimaryActorTick.bCanEverTick = false;

	SceneRootComponent = CreateDefaultSubobject<USceneComponent>("Root Scene Component");
	RootComponent = SceneRootComponent;;
	FString ColorString = FString(TEXT("R=223,G=0,B=0,A=1"));
	Color.InitFromString(ColorString);
	
	Arrow = CreateDefaultSubobject<UArrowComponent>("Arrow");
	Arrow -> bIsScreenSizeScaled = false;
	Arrow -> bIsEditorOnly = true;
	Arrow -> ArrowLength = CalculateTotalLength(NumberOfMeshes, MeshScale, Padding);
	FVector ArrowScale = Arrow -> GetRelativeScale3D();
	// ArrowScale.Z = 8.0;
	Arrow -> SetRelativeScale3D(ArrowScale);
	Arrow -> ArrowColor = Color.QuantizeRound();

	Arrow -> SetupAttachment(RootComponent);
}

// void AMeshController::PreEditChange(FProperty* ChangedProperty) 
// {
// 	FName PropertyName = (ChangedProperty != NULL) ? ChangedProperty -> GetFName() : NAME_None;

// 	if(PropertyName == GET_MEMBER_NAME_CHECKED(AMeshController, Padding)){
// 		FFloatProperty* Prop = static_cast<FFloatProperty*>(ChangedProperty);

// 		if(Prop -> GetFloatingPointPropertyValue(Prop))
// 		{
// 			float TargetPadding = Prop -> GetFloatingPointPropertyValue(Prop);
// 			UE_LOG(LogTemp, Warning, TEXT("Value %f"), TargetPadding);

// 			ChangePadding(TargetPadding, false);
// 		}
// 	}

// 	Super::PreEditChange(ChangedProperty) ;
// }

void AMeshController::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e) ;

	FName PropertyName = (e.Property != NULL) ? e.Property -> GetFName() : NAME_None;

	if(
		PropertyName == GET_MEMBER_NAME_CHECKED(AMeshController, Padding) || 
		PropertyName == GET_MEMBER_NAME_CHECKED(AMeshController, NumberOfMeshes) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMeshController, MeshScale)
	 )
	 {
		Arrow -> ArrowLength = CalculateTotalLength(NumberOfMeshes, MeshScale, Padding);
	 }

}


// Called when the game starts or when spawned
void AMeshController::BeginPlay()
{
	// FocusIndices = CQTManager -> FocusIndices;
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

	VisualizationMeshes.AddUninitialized(Number);

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

			VisualizationMeshes[i - 1] = CurrentMesh;

			UGameplayStatics::FinishSpawningActor(CurrentMesh, Transform);
			if(doFocus && FocusIndices[i - 1])
			{
				CurrentMesh -> SetColor(FocusColor);

			}
			else
			{

				CurrentMesh -> SetColor(Color);

			}
	}

}

void AMeshController::SpawnMeshesInCircle(int32 Number,  float InPadding, float Radius )
{
	FVector ForwardVector = this -> GetActorForwardVector();
	FVector Location = this -> GetActorLocation();
	FVector SpawnScale(MeshScale);


	float RadiusWithPadding = (static_cast<float>(Number) * InPadding ) + Radius;

	float Step = 360.0f / static_cast<float>(Number);

	// Step*=2;


	VisualizationMeshes.AddUninitialized(Number);

	ESpawnActorCollisionHandlingMethod Collision = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FVector PaddingVector(InPadding);
	FActorSpawnParameters Parameters = FActorSpawnParameters(); 

	for(int32 i = 1; i <= Number; i++)
	{

		float CurrentStep = static_cast<float>(i) * Step;


		float RotatorZ = CurrentStep;

		FRotator SpawnRotator = UKismetMathLibrary::MakeRotator(0.0f,RotatorZ, 0.0f);	


		FVector RotatedVector =  UKismetMathLibrary::GreaterGreater_VectorRotator(ForwardVector, SpawnRotator);

		FVector CircleVector = UKismetMathLibrary::Multiply_VectorFloat(RotatedVector, RadiusWithPadding);

		FVector SpawnLocation = UKismetMathLibrary::Add_VectorVector(CircleVector,Location);

		FQuat SpawnRotation = GetActorQuat();


		FTransform Transform(SpawnRotation, SpawnLocation,SpawnScale);

		ASoundMesh* CurrentMesh = GetWorld()->SpawnActorDeferred<ASoundMesh>(

			ASoundMesh::StaticClass()
			, Transform
			, this
			, nullptr
			, Collision
			);


			FAttachmentTransformRules TransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
			CurrentMesh -> AttachToActor(this, TransformRules);

			CurrentMesh -> Init(Mesh, Material, SpawnScale);

			VisualizationMeshes[i - 1] = CurrentMesh;

			UGameplayStatics::FinishSpawningActor(CurrentMesh, Transform);
			if(doFocus && FocusIndices[i - 1])
			{
				CurrentMesh -> SetColor(FocusColor);

			}
			else
			{

				CurrentMesh -> SetColor(Color);

			}

			FVector MeshLocation = CurrentMesh -> GetActorLocation();
			FRotator LookatRotator = UKismetMathLibrary::FindLookAtRotation(MeshLocation, Location);
			FRotator UpOffset = UKismetMathLibrary::MakeRotator(0.0,90.0,0.0);
			FRotator NewRotation = UKismetMathLibrary::ComposeRotators(UpOffset, LookatRotator);
			CurrentMesh -> SetActorRelativeRotation(NewRotation);


	}
}


void AMeshController::SpawnMeshesInSphere(int32 Number, float Radius)
{

	float Step = 360.0f / static_cast<float>(Number);

	FVector Location = this -> GetActorLocation();

	FQuat SpawnRotation = GetActorQuat();
	FVector SpawnScale(MeshScale);



	ESpawnActorCollisionHandlingMethod Collision = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FActorSpawnParameters Parameters = FActorSpawnParameters(); 

	VisualizationMeshes.AddUninitialized(Number);

	for (int32 i = 0; i < Number; i++)
    {
 		


        // Calculate the spherical coordinates

        FVector SphereLocation = FibSphere(i,Number,Radius);

        // Create the spawn location relative to the spawner's location
        FVector SpawnLocation = UKismetMathLibrary::Add_VectorVector(SphereLocation,Location);

		FTransform Transform(SpawnRotation, SpawnLocation,SpawnScale);

		ASoundMesh* CurrentMesh = GetWorld()->SpawnActorDeferred<ASoundMesh>(

		ASoundMesh::StaticClass()
		, Transform
		, this
		, nullptr
		, Collision
		);


		FAttachmentTransformRules TransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
		CurrentMesh -> AttachToActor(this, TransformRules);

		CurrentMesh -> Init(Mesh, Material, SpawnScale);

		VisualizationMeshes[i] = CurrentMesh;

		UGameplayStatics::FinishSpawningActor(CurrentMesh, Transform);
		if(doFocus && FocusIndices[i - 1])
		{
			CurrentMesh -> SetColor(FocusColor);

		}
		else
		{

			CurrentMesh -> SetColor(Color);

		}

		FVector MeshLocation = CurrentMesh -> GetActorLocation();

		FRotator LookatRotator = UKismetMathLibrary::FindLookAtRotation(MeshLocation, Location);
		FRotator UpOffset = UKismetMathLibrary::MakeRotator(0.0,90.0,0.0);

		FRotator NewRotation = UKismetMathLibrary::ComposeRotators(UpOffset, LookatRotator);

		CurrentMesh -> SetActorRelativeRotation(NewRotation);
		// CurrentMesh -> AddActorLocalRotation(UKismetMathLibrary::MakeRotator(0.0,180.0,0.0));

    }


}

void AMeshController::SpawnMeshesInSpiral(int32 Number, float Radius, float SpiralRadius)
{

	float Step = 360.0f / static_cast<float>(Number);

	FVector Location = this -> GetActorLocation();

	FQuat SpawnRotation = GetActorQuat();
	FVector SpawnScale(MeshScale);



	ESpawnActorCollisionHandlingMethod Collision = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	FActorSpawnParameters Parameters = FActorSpawnParameters(); 

	VisualizationMeshes.AddUninitialized(Number);

	for (int32 i = 0; i < Number; i++)
    {
 		
 // Calculate the inclination angle based on the current object's index
		float SpiralOffset = SpiralRadius * (i / static_cast<float>(Number));

	
        FVector SphereLocation = FibSpiral(i,Number,(Radius),SpiralOffset);


        // Create the spawn location relative to the spawner's location
        FVector SpawnLocation = UKismetMathLibrary::Add_VectorVector(SphereLocation,Location);

		FTransform Transform(SpawnRotation, SpawnLocation,SpawnScale);

		ASoundMesh* CurrentMesh = GetWorld()->SpawnActorDeferred<ASoundMesh>(

		ASoundMesh::StaticClass()
		, Transform
		, this
		, nullptr
		, Collision
		);


		FAttachmentTransformRules TransformRules(EAttachmentRule::KeepWorld, EAttachmentRule::KeepRelative, EAttachmentRule::KeepRelative, true);
		CurrentMesh -> AttachToActor(this, TransformRules);

		CurrentMesh -> Init(Mesh, Material, SpawnScale);

		VisualizationMeshes[i] = CurrentMesh;

		UGameplayStatics::FinishSpawningActor(CurrentMesh, Transform);
		if(doFocus && FocusIndices[i - 1])
		{
			CurrentMesh -> SetColor(FocusColor);

		}
		else
		{

			CurrentMesh -> SetColor(Color);

		}

		FVector MeshLocation = CurrentMesh -> GetActorLocation();

		FRotator LookatRotator = UKismetMathLibrary::FindLookAtRotation(MeshLocation, Location);
		FRotator UpOffset = UKismetMathLibrary::MakeRotator(0.0,90.0,0.0);

		FRotator NewRotation = UKismetMathLibrary::ComposeRotators(UpOffset, LookatRotator);

		CurrentMesh -> SetActorRelativeRotation(NewRotation);
		// CurrentMesh -> AddActorLocalRotation(UKismetMathLibrary::MakeRotator(0.0,180.0,0.0));

    }


}

FVector AMeshController::FibSphere(int32 i, int32 Number, float Radius)
{

	// Calculate the azimuth angle based on the current object's index 
	 float k = i + .5f;
     float phi = FMath::Acos(1.f - 2.f * k / Number);


     float theta = PI * (1 + FMath::Sqrt(5.0f)) * k;

     float x = FMath::Cos(theta) * FMath::Sin(phi);
     float y = FMath::Sin(theta) * FMath::Sin(phi);
     float z = FMath::Cos(phi);

     return FVector(x, y, z) * Radius;
}

FVector AMeshController::FibSpiral(int32 i, int32 Number, float Radius, float SpiralOffset)
{

	// Calculate the azimuth angle based on the current object's index 
	 float k = i + .5f;
     float phi = FMath::Acos(1.f - 2.f * k / Number);


     float theta = PI * (1 + FMath::Sqrt(5.0f)) * k;

     float x = (FMath::Cos(theta) * FMath::Sin(phi)) + SpiralOffset ;
     float y = (FMath::Sin(theta) * FMath::Sin(phi)) + SpiralOffset;
     float z = (FMath::Cos(phi)) + SpiralOffset;

     return FVector(x, y, z) * Radius;
}


void AMeshController::ChangePadding(float InPadding, bool SetPadding)
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

	if(SetPadding)
	{
		Padding = InPadding;
	}
}

void AMeshController::UpdateMesh(int32 Index, double Value)
{

}

void AMeshController::UpdateMeshZ(int32 Index, float Value)
{
	ASoundMesh* CurrentMesh = VisualizationMeshes[Index];

	float NewZScale = FMath::Max(Value * ScaleFactor, MinScale);
	// float SpectrumMax = CQTManager -> getMaxSpectrum();
	
	// float NewBrightness = UKismetMathLibrary::MapRangeClamped(NewZScale, 0, SpectrumMax, .1, 3.0);

	CurrentMesh -> SetZScale(NewZScale);
	// CurrentMesh -> SetBrightness(NewBrightness);
}

void AMeshController::UpdateMeshBrightness(int32 Index, float Value)
{
	ASoundMesh* CurrentMesh = VisualizationMeshes[Index];

	CurrentMesh -> SetBrightness(Value);
}
void AMeshController::TestSpawn()
{
	SpawnMeshesInLine(NumberOfMeshes, Padding);
}


void AMeshController::UpdateMeshColor(int32 Index, FLinearColor InColor)
{
	ASoundMesh* CurrentMesh = VisualizationMeshes[Index];
	CurrentMesh -> SetColor(InColor);

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
        TotalLength += Padding * (NumberOfObjects + 1);
    }

    return TotalLength;
}