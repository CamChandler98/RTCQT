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

    Material = UMaterialInstanceDynamic::Create(NewMaterial, this);

    MeshComponent -> SetStaticMesh(Mesh);

    MeshComponent -> SetMaterial(0,Material);

    MeshComponent -> SetRelativeScale3D(NewScale);
    
}


void ASoundMesh::SetMesh(UStaticMesh* NewMesh)
{
    Mesh = NewMesh;
    MeshComponent -> SetStaticMesh(NewMesh);

}

void ASoundMesh::SetMaterial(UMaterialInstanceDynamic* NewMaterial)
{
    FName ColorName = FName(TEXT("Color"));
    FName Intensity = FName(TEXT("Intensity"));

    Brightness = NewMaterial -> K2_GetScalarParameterValue(Intensity);
    Color =  NewMaterial -> K2_GetVectorParameterValue(ColorName);

    MeshComponent -> SetMaterial(0,NewMaterial);

} 

void ASoundMesh::SetScale(FVector NewScale)
{
    MeshComponent -> SetRelativeScale3D(NewScale);

}


void ASoundMesh::SetZScale(const float NewZScale)
{


    FVector NewScale = MeshComponent -> GetRelativeScale3D();

    NewScale.Z = NewZScale;
    
    MeshComponent -> SetRelativeScale3D(NewScale);
    
}

void ASoundMesh::SetBrightness(const float InBrightness)
{


    Material -> SetScalarParameterValue("Intensity", InBrightness); 

    Brightness = InBrightness;



}

void ASoundMesh::SetColor(FLinearColor NewColor)
{

    Material -> SetVectorParameterValue("Color", NewColor);
    Material -> SetScalarParameterValue("Intensity", Brightness); 

    Color = NewColor;

}