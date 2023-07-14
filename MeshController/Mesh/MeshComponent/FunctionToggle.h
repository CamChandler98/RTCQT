#pragma once

#include "CoreMinimal.h"
#include "FunctionToggle.generated.h"


UCLASS(Blueprintable, EditInlineNew)
class UFunctionToggle : public UObject
{
	GENERATED_BODY()

public:
	FName Name;

    bool Active;

	UFunctionToggle()
	{

	}

};