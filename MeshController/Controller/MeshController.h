// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/ArrowComponent.h"
#include "../../RealTimeCQTManager.h"
#include "../Mesh/SoundMesh.h"
#include "MeshController.generated.h"

UCLASS()
class SYNRTCQT_API AMeshController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMeshController();
	
	/** Please add a function description */
	UFUNCTION(BlueprintCallable)
	virtual void SpawnMeshes(int32 Number);


	UFUNCTION(CallInEditor, Category="Functions" )
	void TestSpawn();

	UFUNCTION(CallInEditor, Category="Functions" )
	void TestIncreasePadding();

	UFUNCTION(CallInEditor, Category="Functions" )
	void TestDecreasePadding();

	UFUNCTION(BlueprintCallable)
	void ChangePadding(float InPadding);

	UFUNCTION(BlueprintCallable)
	void SpawnMeshesInLine(int32 Number, float InPadding);

		/** Please add a function description */
	UFUNCTION(BlueprintCallable)
	void UpdateMesh(int32 Index, double Value);


	float CalculateTotalLength(int NumberOfObjects, float Scale, float InPadding);


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USceneComponent* SceneRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UArrowComponent* Arrow; 

		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	int32 NumberOfMeshes = 20;

		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	double MeshScale = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	float Padding = 10; 

		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	double ScaleFactor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TObjectPtr<UMaterialInterface>  Material;

		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	FLinearColor Color;

		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	FLinearColor FocusColor;


		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TArray<double> BandStrengths;

		/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category="Default")
	TObjectPtr<ARealTimeCQTManager> CQTManager;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category="Default")
	TObjectPtr<UStaticMesh>  Mesh;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category="Default")
	TArray<ASoundMesh*> VisualizationMeshes; 



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
