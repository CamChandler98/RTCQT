#include "RTCQTPaletteComponent.h"

#include "HAL/PlatformTime.h"
#include "RTCQTMusicLabManager.h"
#include "RTCQTPalettePresets.h"
#include "RTCQTVisualizer.h"

URTCQTPaletteComponent::URTCQTPaletteComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

const URTCQTPalettePresets* URTCQTPaletteComponent::GetPresets() const
{
	return PresetAsset ? PresetAsset.Get() : GetDefault<URTCQTPalettePresets>();
}

ARTCQTVisualizer* URTCQTPaletteComponent::GetVisualizer() const
{
	const auto* Manager = Cast<ARTCQTMusicLabManager>(GetOwner());
	return IsValid(Manager) ? Manager->Visualizer.Get() : nullptr;
}

bool URTCQTPaletteComponent::CheckSetup()
{
	auto* Visualizer = GetVisualizer();
	FLinearColor Low, High;
	if (!IsValid(Visualizer) || !Visualizer->GetAuthoredPaletteColors(Low, High)
		|| !Visualizer->GetDisplayedPaletteColors(Low, High))
	{
		LastError = TEXT("Palette needs a Music Lab visualizer with LowBandColor and HighBandColor");
		return false;
	}
	if (OwnedVisualizer.IsValid() && OwnedVisualizer.Get() != Visualizer)
	{
		LastError = TEXT("Visualizer changed; disable and re-enable palette controls");
		return false;
	}
	return GetPresets()->ValidatePresets(LastError);
}

void URTCQTPaletteComponent::SetPaletteEnabled(bool bNewEnabled)
{
	if (bNewEnabled)
	{
		if (!CheckSetup()) return;
		bPaletteEnabled = true;
		return;
	}
	StopBlend();
	if (auto* Visualizer = OwnedVisualizer.Get()) Visualizer->ClearRuntimePaletteColors();
	OwnedVisualizer.Reset();
	bPaletteEnabled = false;
	SelectedPaletteId = NAME_None;
	LastError.Reset();
}

bool URTCQTPaletteComponent::SelectPalette(FName Id)
{
	return SelectPaletteAt(Id, FPlatformTime::Seconds());
}

bool URTCQTPaletteComponent::SelectPaletteAt(FName Id, double Now)
{
	if (!bPaletteEnabled) { LastError = TEXT("Palette controls are disabled"); return false; }
	if (!CheckSetup()) return false;
	const auto* Preset = GetPresets()->Presets.FindByPredicate([Id](const auto& Item) { return Item.Id == Id; });
	if (!Preset) { LastError = TEXT("Unknown palette name"); return false; }
	if (SelectedPaletteId == Id && !bResetOnCompletion
		&& TargetLow == Preset->LowColor && TargetHigh == Preset->HighColor) return true;
	return StartBlend(Id, Preset->LowColor, Preset->HighColor, false, Now);
}

bool URTCQTPaletteComponent::NextPalette()
{
	if (!bPaletteEnabled) { LastError = TEXT("Palette controls are disabled"); return false; }
	if (!CheckSetup()) return false;
	const auto& Presets = GetPresets()->Presets;
	const int32 Current = Presets.IndexOfByPredicate([this](const auto& Item) { return Item.Id == SelectedPaletteId; });
	return SelectPalette(Presets[(Current + 1) % Presets.Num()].Id);
}

bool URTCQTPaletteComponent::ResetPalette()
{
	return ResetPaletteAt(FPlatformTime::Seconds());
}

bool URTCQTPaletteComponent::ResetPaletteAt(double Now)
{
	if (!bPaletteEnabled) { LastError = TEXT("Palette controls are disabled"); return false; }
	if (!CheckSetup()) return false;
	FLinearColor Low, High;
	if (!GetVisualizer()->GetAuthoredPaletteColors(Low, High)) return false;
	return StartBlend(NAME_None, Low, High, true, Now);
}

bool URTCQTPaletteComponent::StartBlend(FName Id, const FLinearColor& Low, const FLinearColor& High, bool bReset, double Now)
{
	// Sample the existing transition at the same instant before choosing its replacement.
	if (bBlending && !AdvanceBlend(Now)) return false;
	auto* Visualizer = GetVisualizer();
	FLinearColor DisplayedLow, DisplayedHigh;
	if (!Visualizer->GetDisplayedPaletteColors(DisplayedLow, DisplayedHigh)
		|| !Visualizer->SetRuntimePaletteColors(DisplayedLow, DisplayedHigh))
	{
		LastError = TEXT("Could not apply palette to the visualizer material");
		return false;
	}
	FromLow = DisplayedLow;
	FromHigh = DisplayedHigh;
	TargetLow = Low;
	TargetHigh = High;
	BlendStartedAt = Now;
	SelectedPaletteId = Id;
	bResetOnCompletion = bReset;
	OwnedVisualizer = Visualizer;
	bBlending = true;
	LastError.Reset();
	SetComponentTickEnabled(true);
	return true;
}

bool URTCQTPaletteComponent::AdvanceBlend(double Now)
{
	if (!bBlending) return true;
	auto* Visualizer = OwnedVisualizer.Get();
	if (!bPaletteEnabled || !IsValid(Visualizer) || Visualizer != GetVisualizer())
	{
		StopBlend();
		LastError = TEXT("Palette transition lost its enabled visualizer");
		return false;
	}
	const double T = FMath::Clamp((Now - BlendStartedAt) / BlendDurationSeconds, 0.0, 1.0);
	const float Alpha = static_cast<float>(T * T * (3.0 - 2.0 * T));
	const FLinearColor Low = T >= 1.0 ? TargetLow : FMath::LerpStable(FromLow, TargetLow, Alpha);
	const FLinearColor High = T >= 1.0 ? TargetHigh : FMath::LerpStable(FromHigh, TargetHigh, Alpha);
	if (!Visualizer->SetRuntimePaletteColors(Low, High))
	{
		StopBlend();
		LastError = TEXT("Palette transition material is no longer compatible");
		return false;
	}
	if (T >= 1.0)
	{
		if (bResetOnCompletion) Visualizer->ClearRuntimePaletteColors();
		StopBlend();
	}
	return true;
}

void URTCQTPaletteComponent::StopBlend()
{
	bBlending = false;
	bResetOnCompletion = false;
	SetComponentTickEnabled(false);
}

void URTCQTPaletteComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bPaletteEnabled && !CheckSetup()) bPaletteEnabled = false;
}

void URTCQTPaletteComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	SetPaletteEnabled(false);
	Super::EndPlay(Reason);
}

void URTCQTPaletteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceBlend(FPlatformTime::Seconds());
}
