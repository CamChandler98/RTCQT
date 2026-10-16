#include "RTCQTMusicLabControls.h"
#include "RTCQTMusicLabManager.h"
#include "RTCQTPaletteComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "HAL/PlatformTime.h"

TSharedRef<SWidget> URTCQTMusicLabControls::RebuildWidget()
{
	bPopulating = true;
	OptionIds.Reset();
	CachedPalette.Reset();
	CachedPresetAsset.Reset();
	LastPaletteError.Reset();
	PaletteFeedbackUntil = 0;
	// This tree is entirely native; use a fresh transient tree on Slate rebuilds.
	WidgetTree = NewObject<UWidgetTree>(this);
	auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	auto* Width = WidgetTree->ConstructWidget<USizeBox>();
	Width->SetWidthOverride(580);
	auto* CanvasSlot = Canvas->AddChildToCanvas(Width);
	CanvasSlot->SetPosition(FVector2D(24, 24));
	CanvasSlot->SetAutoSize(true);
	auto* Background = WidgetTree->ConstructWidget<UBorder>();
	Background->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.92f));
	Background->SetPadding(FMargin(16));
	Width->AddChild(Background);
	auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Background->AddChild(Column);
	Dropdown = WidgetTree->ConstructWidget<UComboBoxString>();
	Column->AddChildToVerticalBox(Dropdown);
	if (Manager && Manager->Catalog)
	{
		TSet<FString> Seen;
		for (const auto& Track : Manager->Catalog->Tracks)
		{
			if (Seen.Contains(Track.Id) || !Manager->IsValidTrack(Track)) continue;
			// Ambiguous catalog IDs are rejected by the manager and omitted here.
			int32 Count = 0;
			for (const auto& Other : Manager->Catalog->Tracks) if (Other.Id == Track.Id) ++Count;
			if (Count != 1) continue;
			Seen.Add(Track.Id);
			FString Option = Track.Label;
			int32 Suffix = 2;
			while (OptionIds.Contains(Option)) Option = FString::Printf(TEXT("%s (%d)"), *Track.Label, Suffix++);
			OptionIds.Add(Option, Track.Id);
			Dropdown->AddOption(Option);
		}
	}
	Status = WidgetTree->ConstructWidget<UTextBlock>();
	Status->SetWrapTextAt(548);
	Column->AddChildToVerticalBox(Status)->SetPadding(FMargin(0, 8, 0, 12));
	auto* PlayButton = WidgetTree->ConstructWidget<UButton>();
	PlayLabel = WidgetTree->ConstructWidget<UTextBlock>();
	PlayLabel->SetText(FText::FromString(TEXT("Play / Pause")));
	PlayLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	PlayButton->AddChild(PlayLabel);
	Column->AddChildToVerticalBox(PlayButton)->SetPadding(FMargin(0, 0, 0, 6));
	PlayButton->OnClicked.AddDynamic(this, &URTCQTMusicLabControls::PlayPause);
	auto* RestartButton = WidgetTree->ConstructWidget<UButton>();
	auto* RestartLabel = WidgetTree->ConstructWidget<UTextBlock>();
	RestartLabel->SetText(FText::FromString(TEXT("Restart")));
	RestartLabel->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
	RestartButton->AddChild(RestartLabel);
	Column->AddChildToVerticalBox(RestartButton);
	RestartButton->OnClicked.AddDynamic(this, &URTCQTMusicLabControls::Restart);

	const auto Text = [this](const FString& Value)
	{
		auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(Value));
		auto Font = Label->GetFont();
		Font.Size = 16;
		Label->SetFont(Font);
		return Label;
	};
	Column->AddChildToVerticalBox(Text(TEXT("Palettes")))->SetPadding(FMargin(0, 16, 0, 8));
	PaletteEnabled = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("RTCQTPaletteEnable"));
	PaletteEnabled->AddChild(Text(TEXT("Enable palettes")));
	Column->AddChildToVerticalBox(PaletteEnabled)->SetPadding(FMargin(0, 0, 0, 8));
	PaletteEnabled->OnCheckStateChanged.AddDynamic(this, &URTCQTMusicLabControls::PaletteEnabledChanged);
	PaletteDropdown = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("RTCQTPaletteDropdown"));
	Column->AddChildToVerticalBox(PaletteDropdown)->SetPadding(FMargin(0, 0, 0, 8));
	PaletteDropdown->OnSelectionChanged.AddDynamic(this, &URTCQTMusicLabControls::PaletteSelected);
	auto* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	Column->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0, 0, 0, 10));
	const auto Action = [this, Actions, &Text](FName Name, const TCHAR* Label)
	{
		auto* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		auto* Caption = Text(Label);
		Caption->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		Button->AddChild(Caption);
		auto* Slot = Actions->AddChildToHorizontalBox(Button);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetPadding(FMargin(0, 0, 6, 0));
		return Button;
	};
	PaletteNextButton = Action(TEXT("RTCQTPaletteNext"), TEXT("Next palette"));
	PaletteResetButton = Action(TEXT("RTCQTPaletteReset"), TEXT("Reset palette"));
	PaletteNextButton->OnClicked.AddDynamic(this, &URTCQTMusicLabControls::PaletteNext);
	PaletteResetButton->OnClicked.AddDynamic(this, &URTCQTMusicLabControls::PaletteReset);
	ViewerPalettes = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("RTCQTViewerPalettes"));
	ViewerPalettes->AddChild(Text(TEXT("Viewer palettes")));
	Column->AddChildToVerticalBox(ViewerPalettes)->SetPadding(FMargin(0, 0, 0, 8));
	ViewerPalettes->OnCheckStateChanged.AddDynamic(this, &URTCQTMusicLabControls::ViewerPalettesChanged);
	PaletteStatus = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RTCQTPaletteStatus"));
	PaletteStatus->SetWrapTextAt(548);
	auto PaletteFont = PaletteStatus->GetFont();
	PaletteFont.Size = 16;
	PaletteStatus->SetFont(PaletteFont);
	Column->AddChildToVerticalBox(PaletteStatus);
	PaletteFeedback = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RTCQTPaletteFeedback"));
	PaletteFeedback->SetWrapTextAt(548);
	PaletteFeedback->SetFont(PaletteFont);
	Column->AddChildToVerticalBox(PaletteFeedback)->SetPadding(FMargin(0, 6, 0, 0));
	RefreshPaletteOptions();
	RefreshPaletteState();
	Dropdown->OnSelectionChanged.AddDynamic(this, &URTCQTMusicLabControls::Selected);
	bPopulating = false;
	return Super::RebuildWidget();
}

void URTCQTMusicLabControls::Selected(FString Option, ESelectInfo::Type Type)
{
	if (bPopulating || Type == ESelectInfo::Direct || !Manager) return;
	if (const auto* Id = OptionIds.Find(Option)) Manager->SelectTrack(*Id);
}

void URTCQTMusicLabControls::PlayPause() { if (Manager) Manager->TogglePlayPause(); }
void URTCQTMusicLabControls::Restart() { if (Manager) Manager->Restart(); }

URTCQTPaletteComponent* URTCQTMusicLabControls::GetPalette() const
{
	return IsValid(Manager) && IsValid(Manager->Palette) ? Manager->Palette.Get() : nullptr;
}

void URTCQTMusicLabControls::RefreshPaletteOptions()
{
	if (!PaletteDropdown) return;
	TGuardValue<bool> Guard(bUpdatingPaletteControls, true);
	PaletteOptionIds.Reset();
	PaletteDropdown->ClearOptions();
	PaletteOptionIds.Add(TEXT("Authored colors"), NAME_None);
	PaletteDropdown->AddOption(TEXT("Authored colors"));
	auto* Palette = GetPalette();
	if (Palette)
	{
		for (const auto& Preset : Palette->GetAvailablePalettes())
		{
			const FString Base = Preset.DisplayName.ToString();
			FString Option = Base;
			int32 Suffix = 2;
			while (PaletteOptionIds.Contains(Option)) Option = FString::Printf(TEXT("%s (%d)"), *Base, Suffix++);
			PaletteOptionIds.Add(Option, Preset.Id);
			PaletteDropdown->AddOption(Option);
		}
	}
	if (CachedPalette.Get() != Palette) LastViewerPaletteCount = Palette ? Palette->AcceptedViewerPaletteCount : 0;
	CachedPalette = Palette;
	CachedPresetAsset = Palette ? Palette->PresetAsset.Get() : nullptr;
}

void URTCQTMusicLabControls::RefreshPaletteState()
{
	if (!PaletteEnabled || !PaletteDropdown || !ViewerPalettes || !PaletteStatus || !PaletteFeedback) return;
	TGuardValue<bool> Guard(bUpdatingPaletteControls, true);
	auto* Palette = GetPalette();
	if (CachedPalette.Get() != Palette || (Palette && CachedPresetAsset.Get() != Palette->PresetAsset.Get())) RefreshPaletteOptions();
	const bool bAvailable = Palette != nullptr;
	const bool bEnabled = bAvailable && Palette->bPaletteEnabled;
	PaletteEnabled->SetIsEnabled(bAvailable);
	PaletteEnabled->SetIsChecked(bEnabled);
	PaletteDropdown->SetIsEnabled(bEnabled && PaletteOptionIds.Num() > 1);
	PaletteNextButton->SetIsEnabled(bEnabled && PaletteOptionIds.Num() > 1);
	PaletteResetButton->SetIsEnabled(bEnabled);
	ViewerPalettes->SetIsEnabled(bEnabled);
	ViewerPalettes->SetIsChecked(bAvailable && Palette->bViewerPalettesEnabled);
	FString Selected = TEXT("Authored colors");
	if (Palette)
	{
		for (const auto& Entry : PaletteOptionIds)
			if (Entry.Value == Palette->SelectedPaletteId) { Selected = Entry.Key; break; }
	}
	if (PaletteDropdown->GetSelectedOption() != Selected) PaletteDropdown->SetSelectedOption(Selected);
	if (!Palette) PaletteStatus->SetText(FText::FromString(TEXT("Palette controls unavailable")));
	else if (!bEnabled) PaletteStatus->SetText(FText::FromString(TEXT("Palettes disabled")));
	else
	{
		const FString State = Palette->bBlending ? TEXT("Blending to ") + Selected : TEXT("Palette: ") + Selected;
		const float Cooldown = Palette->GetViewerPaletteCooldownRemaining();
		const FString ViewerState = Palette->bViewerPalettesEnabled
			? (Cooldown > 0 ? FString::Printf(TEXT("Viewer palettes: ready in %.0f s"), FMath::CeilToFloat(Cooldown)) : TEXT("Viewer palettes: enabled"))
			: TEXT("Viewer palettes: locked");
		PaletteStatus->SetText(FText::FromString(State + TEXT("\n") + ViewerState));
	}
	if (Palette && Palette->AcceptedViewerPaletteCount != LastViewerPaletteCount)
	{
		LastViewerPaletteCount = Palette->AcceptedViewerPaletteCount;
		ShowPaletteFeedback(Selected + TEXT(" palette selected by viewer"), true);
	}
	if (Palette && Palette->LastError != LastPaletteError)
	{
		LastPaletteError = Palette->LastError;
		if (!LastPaletteError.IsEmpty()) ShowPaletteFeedback(LastPaletteError, false);
	}
	if (FPlatformTime::Seconds() >= PaletteFeedbackUntil) PaletteFeedback->SetText(FText::GetEmpty());
}

void URTCQTMusicLabControls::ShowPaletteFeedback(const FString& Message, bool bAccepted)
{
	if (!PaletteFeedback) return;
	PaletteFeedback->SetText(FText::FromString(Message));
	PaletteFeedback->SetColorAndOpacity(FSlateColor(bAccepted
		? FLinearColor(0.4f, 1.f, 0.7f, 1.f) : FLinearColor(1.f, 0.55f, 0.4f, 1.f)));
	PaletteFeedbackUntil = FPlatformTime::Seconds() + 2.0;
}

void URTCQTMusicLabControls::ShowPaletteResult(bool bAccepted)
{
	auto* Palette = GetPalette();
	if (!Palette) return;
	RefreshPaletteState();
	LastPaletteError = Palette->LastError;
	ShowPaletteFeedback(bAccepted ? (PaletteDropdown->GetSelectedOption() + TEXT(" selected")) : Palette->LastError, bAccepted);
}

void URTCQTMusicLabControls::PaletteSelected(FString Option, ESelectInfo::Type Type)
{
	if (bUpdatingPaletteControls || Type == ESelectInfo::Direct) return;
	auto* Palette = GetPalette();
	const auto* Id = PaletteOptionIds.Find(Option);
	if (Palette && Id) ShowPaletteResult(Id->IsNone() ? Palette->ResetPalette() : Palette->SelectPalette(*Id));
}

void URTCQTMusicLabControls::PaletteNext()
{
	if (auto* Palette = GetPalette()) { RefreshPaletteOptions(); ShowPaletteResult(Palette->NextPalette()); }
}

void URTCQTMusicLabControls::PaletteReset()
{
	if (auto* Palette = GetPalette()) ShowPaletteResult(Palette->ResetPalette());
}

void URTCQTMusicLabControls::PaletteEnabledChanged(bool bChecked)
{
	if (bUpdatingPaletteControls) return;
	if (auto* Palette = GetPalette())
	{
		Palette->SetPaletteEnabled(bChecked);
		RefreshPaletteOptions();
		RefreshPaletteState();
		LastPaletteError = Palette->LastError;
		const bool bAccepted = Palette->bPaletteEnabled == bChecked && Palette->LastError.IsEmpty();
		ShowPaletteFeedback(bAccepted
			? (bChecked ? TEXT("Palettes enabled") : TEXT("Palettes disabled")) : Palette->LastError,
			bAccepted);
	}
}

void URTCQTMusicLabControls::ViewerPalettesChanged(bool bChecked)
{
	if (bUpdatingPaletteControls) return;
	if (auto* Palette = GetPalette())
	{
		Palette->SetViewerPalettesEnabled(bChecked);
		RefreshPaletteState();
		ShowPaletteFeedback(bChecked ? TEXT("Viewer palettes enabled") : TEXT("Viewer palettes locked"), true);
	}
}

void URTCQTMusicLabControls::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
	RefreshPaletteState();
	if (!Manager || !Status) return;
	const TCHAR* State = Manager->PlaybackState == ERTCQTMusicPlaybackState::Playing ? TEXT("Playing")
		: Manager->PlaybackState == ERTCQTMusicPlaybackState::Paused ? TEXT("Paused") : TEXT("Stopped");
	Status->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s  %.2f s%s%s"),
		*Manager->CurrentTrack.Label, State, Manager->PlaybackPosition,
		Manager->LastError.IsEmpty() ? TEXT("") : TEXT("\n"), *Manager->LastError)));
	PlayLabel->SetText(FText::FromString(Manager->PlaybackState == ERTCQTMusicPlaybackState::Playing ? TEXT("Pause") : TEXT("Play")));
	for (const auto& Entry : OptionIds)
	{
		if (Entry.Value == Manager->CurrentTrack.Id && Dropdown->GetSelectedOption() != Entry.Key)
		{
			bPopulating = true;
			Dropdown->SetSelectedOption(Entry.Key);
			bPopulating = false;
			break;
		}
	}
}
