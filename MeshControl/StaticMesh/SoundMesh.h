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
	void SetMaterial(UMaterialInstanceDynamic* NewMaterial);

	// Function to change the scale
	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetScale(FVector NewScale);

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetZScale(const float NewZScale);

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetBrightness(const float InBrightness);

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void SetColor(FLinearColor NewColor);

	UFUNCTION(BlueprintCallable, Category = "Sound Mesh")
	void Init(UStaticMesh* NewMesh, UMaterialInterface* NewMaterial, FVector NewScale);
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Default")
	TObjectPtr<UStaticMeshComponent> MeshComponent;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Default")
	TObjectPtr<UStaticMesh> Mesh;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	TObjectPtr<UMaterialInstanceDynamic>  Material;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Default")
	FLinearColor Color;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Default")
	float Brightness = 1.0;
};
