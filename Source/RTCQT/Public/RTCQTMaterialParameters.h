#pragma once

#include "CoreMinimal.h"

/** Stable names consumed by the shared D12 material; no per-band material allocation. */
namespace RTCQTMaterialParameters
{
inline const FName EmissiveMin(TEXT("EmissiveMin"));
inline const FName EmissiveMax(TEXT("EmissiveMax"));
inline constexpr float MaxEmissiveIntensity = 100.0f;
}
