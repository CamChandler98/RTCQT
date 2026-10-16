#include "RTCQTPaletteComponent.h"

#include "HAL/PlatformTime.h"
#include "RTCQTMusicLabManager.h"
#include "RTCQTPalettePresets.h"
#include "RTCQTStreamerBotComponent.h"
#include "RTCQTVisualizer.h"
#include "RTCQTLog.h"

URTCQTPaletteComponent::URTCQTPaletteComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

const URTCQTPalettePresets* URTCQTPaletteComponent::GetPresets() const
{
	return PresetAsset ? PresetAsset.Get() : GetDefault<URTCQTPalettePresets>();
}

TArray<FRTCQTPalettePreset> URTCQTPaletteComponent::GetAvailablePalettes() const
{
	FString Error;
	return GetPresets()->ValidatePresets(Error) ? GetPresets()->Presets : TArray<FRTCQTPalettePreset>();
}

ARTCQTVisualizer* URTCQTPaletteComponent::GetVisualizer() const
{
	const auto* Manager = Cast<ARTCQTMusicLabManager>(GetOwner());
	return IsValid(Manager) ? Manager->Visualizer.Get() : nullptr;
}

URTCQTStreamerBotComponent* URTCQTPaletteComponent::GetReceiver() const
{
	const auto* Manager = Cast<ARTCQTMusicLabManager>(GetOwner());
	return IsValid(Manager) ? Manager->StreamerBot.Get() : nullptr;
}

void URTCQTPaletteComponent::BindReceiver()
{
	auto* Receiver = GetReceiver();
	if (BoundReceiver.Get() != Receiver) UnbindReceiver();
	if (IsValid(Receiver))
	{
		Receiver->OnCommandReceived.AddUniqueDynamic(this, &URTCQTPaletteComponent::HandleChatCommand);
		BoundReceiver = Receiver;
	}
}

void URTCQTPaletteComponent::UnbindReceiver()
{
	if (auto* Receiver = BoundReceiver.Get())
		Receiver->OnCommandReceived.RemoveDynamic(this, &URTCQTPaletteComponent::HandleChatCommand);
	BoundReceiver.Reset();
}

void URTCQTPaletteComponent::HandleChatCommand(const FString& Command, const FString& Value)
{
	if (Command.Equals(TEXT("palette"), ESearchCase::CaseSensitive)) TryViewerPalette(Value);
}

void URTCQTPaletteComponent::SetViewerPalettesEnabled(bool bNewEnabled)
{
	bViewerPalettesEnabled = bNewEnabled;
}

double URTCQTPaletteComponent::ViewerCooldownRemainingAt(double Now) const
{
	return bHasAcceptedViewerPalette
		? FMath::Clamp(ViewerCooldownSeconds - (Now - LastViewerAcceptedAt), 0.0, ViewerCooldownSeconds) : 0.0;
}

float URTCQTPaletteComponent::GetViewerPaletteCooldownRemaining() const
{
	return static_cast<float>(ViewerCooldownRemainingAt(FPlatformTime::Seconds()));
}

bool URTCQTPaletteComponent::TryViewerPalette(const FString& Value)
{
	return TryViewerPaletteAt(Value, FPlatformTime::Seconds());
}

bool URTCQTPaletteComponent::TryViewerPaletteAt(const FString& Value, double Now)
{
	if (!bPaletteEnabled) { LastError = TEXT("Palette controls are disabled"); return false; }
	if (!bViewerPalettesEnabled) { LastError = TEXT("Viewer palettes are locked"); return false; }
	auto* Receiver = GetReceiver();
	if (!HasBegunPlay() || !IsValid(Receiver) || BoundReceiver.Get() != Receiver
		|| !Receiver->ConnectionStatus.Equals(TEXT("Ready"), ESearchCase::CaseSensitive))
	{
		LastError = TEXT("Palette receiver is not ready");
		return false;
	}
	const bool bCycle = Value.Equals(TEXT("next"), ESearchCase::CaseSensitive);
	if (!bCycle && !Value.Equals(TEXT("neon"), ESearchCase::CaseSensitive)
		&& !Value.Equals(TEXT("sunset"), ESearchCase::CaseSensitive)
		&& !Value.Equals(TEXT("ice"), ESearchCase::CaseSensitive)
		&& !Value.Equals(TEXT("ember"), ESearchCase::CaseSensitive))
	{
		LastError = TEXT("Unknown viewer palette value");
		return false;
	}
	if (!CheckSetup()) return false;
	const auto& Presets = GetPresets()->Presets;
	FName Id(*Value);
	if (bCycle)
	{
		const int32 Current = Presets.IndexOfByPredicate([this](const auto& Item) { return Item.Id == SelectedPaletteId; });
		Id = Presets[(Current + 1) % Presets.Num()].Id;
	}
	const auto* Preset = Presets.FindByPredicate([Id](const auto& Item) { return Item.Id == Id; });
	if (!Preset) { LastError = TEXT("Requested palette is not configured"); return false; }
	if (IsAlreadySelected(Id, Preset->LowColor, Preset->HighColor))
	{
		LastError = TEXT("Palette is already selected");
		return false;
	}
	if (ViewerCooldownRemainingAt(Now) > 0)
	{
		LastError = TEXT("Viewer palette cooldown");
		return false;
	}
	if (!SelectPaletteAt(Id, Now)) return false;
	bHasAcceptedViewerPalette = true;
	LastViewerAcceptedAt = Now;
	++AcceptedViewerPaletteCount;
	UE_LOG(LogRTCQT, Log, TEXT("RTCQT palette accepted: %s"), *Id.ToString());
	return true;
}

bool URTCQTPaletteComponent::IsAlreadySelected(FName Id, const FLinearColor& Low, const FLinearColor& High) const
{
	return SelectedPaletteId == Id && !bResetOnCompletion
		&& TargetLow == Low && TargetHigh == High;
}

bool URTCQTPaletteComponent::CheckSetup()
{
	auto* Visualizer = GetVisualizer();
	FLinearColor Low, High;
	// A loaded visualizer may create its MID after the manager's components begin play.
	// Validate authored configuration here; StartBlend checks the displayed material on use.
	if (!IsValid(Visualizer) || !Visualizer->GetAuthoredPaletteColors(Low, High))
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
		if (HasBegunPlay()) BindReceiver();
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
	if (IsAlreadySelected(Id, Preset->LowColor, Preset->HighColor)) return true;
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
	BindReceiver();
	bHasAcceptedViewerPalette = false;
	LastViewerAcceptedAt = 0;
	AcceptedViewerPaletteCount = 0;
	if (bPaletteEnabled && !CheckSetup()) bPaletteEnabled = false;
}

void URTCQTPaletteComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	UnbindReceiver();
	SetPaletteEnabled(false);
	bHasAcceptedViewerPalette = false;
	LastViewerAcceptedAt = 0;
	AcceptedViewerPaletteCount = 0;
	Super::EndPlay(Reason);
}

void URTCQTPaletteComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AdvanceBlend(FPlatformTime::Seconds());
}
