#include "RTCQTPalettePresets.h"

URTCQTPalettePresets::URTCQTPalettePresets()
{
	const auto Add = [this](const TCHAR* Id, const TCHAR* Label, FLinearColor Low, FLinearColor High)
	{
		auto& Preset = Presets.AddDefaulted_GetRef();
		Preset.Id = FName(Id);
		Preset.DisplayName = FText::FromString(Label);
		Preset.LowColor = Low;
		Preset.HighColor = High;
	};
	Add(TEXT("neon"), TEXT("Neon"), {0.01f, 0.65f, 1.f, 1.f}, {0.85f, 0.01f, 0.65f, 1.f});
	Add(TEXT("sunset"), TEXT("Sunset"), {1.f, 0.18f, 0.02f, 1.f}, {0.85f, 0.03f, 0.35f, 1.f});
	Add(TEXT("ice"), TEXT("Ice"), {0.02f, 0.35f, 0.85f, 1.f}, {0.6f, 0.95f, 1.f, 1.f});
	Add(TEXT("ember"), TEXT("Ember"), {0.55f, 0.01f, 0.005f, 1.f}, {1.f, 0.3f, 0.015f, 1.f});
}

bool URTCQTPalettePresets::ValidatePresets(FString& Error) const
{
	if (Presets.IsEmpty()) { Error = TEXT("Palette presets are empty"); return false; }
	TSet<FName> Ids;
	for (const auto& Preset : Presets)
	{
		if (Preset.Id.IsNone() || Ids.Contains(Preset.Id) || Preset.DisplayName.IsEmpty())
		{
			Error = TEXT("Palette presets need unique IDs and nonempty display names");
			return false;
		}
		for (const auto& Color : {Preset.LowColor, Preset.HighColor})
		{
			if (!FMath::IsFinite(Color.R) || !FMath::IsFinite(Color.G)
				|| !FMath::IsFinite(Color.B) || !FMath::IsFinite(Color.A))
			{
				Error = TEXT("Palette colors must be finite");
				return false;
			}
		}
		Ids.Add(Preset.Id);
	}
	Error.Reset();
	return true;
}
