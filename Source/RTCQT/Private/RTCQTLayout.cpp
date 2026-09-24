#include "RTCQTLayout.h"

#include "Math/RotationMatrix.h"

namespace
{
double FiniteOrZero(double Value)
{
	return FMath::IsFinite(Value) ? Value : 0.0;
}

double NonNegativeOrZero(double Value)
{
	return FMath::IsFinite(Value) && Value >= 0.0 ? Value : 0.0;
}
}

FTransform FRTCQTLayoutTransform::ToTransform() const
{
	return FTransform(FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat(), Translation, Scale);
}

void FRTCQTLayout::Build(
	const FRTCQTLayoutSettings& Settings,
	int32 Count,
	TArray<FRTCQTLayoutTransform>& OutTransforms)
{
	OutTransforms.Reset();
	// D19 implements the other modes. An empty result lets the owner report failure.
	if (Count <= 0 || Settings.Mode != ERTCQTLayoutMode::Line)
	{
		return;
	}

	const FVector Origin(FiniteOrZero(Settings.Origin.X), FiniteOrZero(Settings.Origin.Y),
		FiniteOrZero(Settings.Origin.Z));
	const FVector Scale(NonNegativeOrZero(Settings.BaseScale.X), NonNegativeOrZero(Settings.BaseScale.Y),
		NonNegativeOrZero(Settings.BaseScale.Z));
	const double Spacing = NonNegativeOrZero(Settings.Spacing);
	const double CenterIndex = (static_cast<double>(Count) - 1.0) * 0.5;
	OutTransforms.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FRTCQTLayoutTransform& Transform = OutTransforms.AddDefaulted_GetRef();
		Transform.Translation = Origin + FVector((Index - CenterIndex) * Spacing, 0.0, 0.0);
		Transform.Scale = Scale;
	}
}
