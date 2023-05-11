// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "SoundMesh.generated.h"

/**
 * 
 */
UCLASS()
class SYNRTCQT_API ASoundMesh : public AStaticMeshActor
{
	GENERATED_BODY()

	public:

	ASoundMesh();

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetMesh(UStaticMesh* NewMesh);

	// Function to change the material
	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetMaterial(UMaterialInterface* NewMaterial);

	// Function to change the scale
	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetScale(FVector NewScale);

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void Init(UStaticMesh* NewMesh, UMaterialInterface* NewMaterial, FVector NewScale);
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Default")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Default")
	TObjectPtr<UStaticMesh> Mesh;
	
};
