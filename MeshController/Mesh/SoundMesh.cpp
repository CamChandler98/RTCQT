// Fill out your copyright notice in the Description page of Project Settings.


#include "SoundMesh.h"

ASoundMesh::ASoundMesh()
{
    // Create a static mesh component
    MeshComponent = GetStaticMeshComponent();
    SetMobility(EComponentMobility::Type::Movable);
    if(!Mesh)
    {
        // Load the Cube mesh
        Mesh = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'")).Object;

        // Set the component's mesh
        SetMesh(Mesh);

    }
    else
    {

        // MeshComponent -> SetStaticMesh(Mesh);

    }
}

void ASoundMesh::Init(UStaticMesh* NewMesh, UMaterialInterface* NewMaterial, FVector NewScale)
{   
    Mesh = NewMesh;
    MeshComponent -> SetStaticMesh(Mesh);

    MeshComponent -> SetMaterial(0,NewMaterial);

    MeshComponent -> SetRelativeScale3D(NewScale);


}


void ASoundMesh::SetMesh(UStaticMesh* NewMesh)
{
    Mesh = NewMesh;
    MeshComponent -> SetStaticMesh(NewMesh);
}

void ASoundMesh::SetMaterial(UMaterialInterface* NewMaterial)
{
    MeshComponent -> SetMaterial(0,NewMaterial);

} 

void ASoundMesh::SetScale(FVector NewScale)
{
    MeshComponent -> SetRelativeScale3D(NewScale);

}

void ASoundMesh::SetColor(FLinearColor NewColor)
{
    UMaterialInterface* CurrentMaterial = MeshComponent -> GetMaterial(0);
    UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(CurrentMaterial, this);

    DynMaterial -> SetVectorParameterValue("Color", NewColor);

    SetMaterial(DynMaterial);

}