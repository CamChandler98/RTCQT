#include "RTCQTMusicLabControls.h"
#include "RTCQTMusicLabManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

TSharedRef<SWidget> URTCQTMusicLabControls::RebuildWidget()
{
	bPopulating = true;
	OptionIds.Reset();
	auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	auto* Width = WidgetTree->ConstructWidget<USizeBox>();
	Width->SetWidthOverride(580);
	auto* CanvasSlot = Canvas->AddChildToCanvas(Width);
	CanvasSlot->SetPosition(FVector2D(24, 24));
	CanvasSlot->SetAutoSize(true);
	auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Width->AddChild(Column);
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
	Status->SetWrapTextAt(560);
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

void URTCQTMusicLabControls::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
	Super::NativeTick(Geometry, DeltaTime);
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
