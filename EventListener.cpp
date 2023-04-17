// Fill out your copyright notice in the Description page of Project Settings.


#include "EventListener.h"

// Sets default values for this component's properties
UEventListener::UEventListener()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UEventListener::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

TArray<float> UEventListener::passArray(const TArray<float>& inArr){
	return inArr;
}
// Called every frame
void UEventListener::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{


	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

