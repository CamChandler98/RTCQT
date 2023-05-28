// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectrumManager.h"

// Sets default values
ASpectrumManager::ASpectrumManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	Sampler = CreateDefaultSubobject<USampler>(TEXT("Sampler"));
	CompiledSpectrum.AddZeroed(NumBands);

	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASpectrumManager::BeginPlay()
{	
	CompiledSpectrum.AddZeroed(NumBands);
	FloatWindowBuffer.AddZeroed(FFTSize);
    SlidingFloatBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(FFTSize, NumHopFrames);
	
	CreateAnalyzers();
	Super::BeginPlay();

	
}

// Called every frame
void ASpectrumManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ASpectrumManager::AnalyzeAudio(const TArray<float>& AudioData)
{

	TArray<float> CCompiledSpectrum;
	CCompiledSpectrum.AddZeroed(NumBands);

	for (const TArray<float>& Window : Audio::TAutoSlidingWindow<float>(*SlidingFloatBuffer, AudioData, FloatWindowBuffer, false))
    {   
		CCompiledSpectrum.Reset();
		
		for(int32 i = 0; i < SpectrumAnalyzers.Num(); i++)
		{
			TObjectPtr<URTCQTAnalyzer> CurrentAnalyzer = SpectrumAnalyzers[i];


			CurrentAnalyzer -> Analyze(Window, DoSampleProcessing, DoSpectrumProcessing);

			CCompiledSpectrum.Append(CurrentAnalyzer -> OutCQT);
		}

		CompiledSpectrum = CCompiledSpectrum;

        for(int32 i = 0; i < CCompiledSpectrum.Num(); i++ ){

            FireOnSpectrumUpdatedEvent(i,CCompiledSpectrum[i]);
        }
    }
}



void ASpectrumManager::CreateAnalyzers()
{
	SpectrumAnalyzers.SetNum(AnalyzersSettings.Num());
	for(int32 i = 0; i < AnalyzersSettings.Num(); i++)
	{	
		TObjectPtr<UAnalyzerSettings> CurrentSettings = AnalyzersSettings[i];

		TObjectPtr<USpectrumSettings> SpectrumSettings = CurrentSettings -> SpectrumProcessorSettings;
		TObjectPtr<USampleSettings> SampleSettings = CurrentSettings -> SampleProcessorSettings;
		TObjectPtr<UCQTSettings> CQTSettings = CurrentSettings -> CQTSettings;

		TObjectPtr<USpectrumToggles> SpectrumToggles = CurrentSettings -> SpectrumToggles;
		TObjectPtr<USampleToggles> SampleToggles = CurrentSettings -> SampleToggles;

		
		CQTSettings -> SampleRate = SampleRate;
		CQTSettings -> UnrestrictedFFTSize = FFTSize;



		FName CurrentAnalyzerName = CurrentSettings -> Name;

		// FString WidgetString = CurrentAnalyzerName.ToString() + ""
		FString NameString = CurrentAnalyzerName.ToString() + "CQT";

		FName UpdatedName = FName(*NameString);

		// TObjectPtr<URTCQTAnalyzer> CurrentAnalyzer = CreateDefaultSubobject<URTCQTAnalyzer>(CurrentAnalyzerName);
		TObjectPtr<URTCQTAnalyzer> CurrentAnalyzer = NewObject<URTCQTAnalyzer>(this,URTCQTAnalyzer::StaticClass(),UpdatedName);


		CurrentAnalyzer -> TotalBands = NumBands;
		
		// CurrentAnalyzer -> SpectrumToggles = SpectrumToggles;
		// CurrentAnalyzer -> SampleToggles = SampleToggles;
		CurrentAnalyzer -> GetParams(CQTSettings);
		CurrentAnalyzer -> GetCQTSettings();
		CurrentAnalyzer -> GetSpectrumProcessor(SpectrumSettings, SpectrumToggles, CurrentAnalyzerName);
		CurrentAnalyzer -> GetSampleProcessor(SampleSettings, SampleToggles, CurrentAnalyzerName);
		CurrentAnalyzer -> GenerateAnalyzer();


		SpectrumAnalyzers[i] = CurrentAnalyzer;

		AddOwnedComponent(CurrentAnalyzer);
	}

	CheckLength();
}

void ASpectrumManager::CheckLength()
{
	int32 TargetLength = 0;

	for(int32 i = 0; i < SpectrumAnalyzers.Num(); i++)
	{
		TargetLength += SpectrumAnalyzers[i] -> OutCQT.Num();
	}

	CompiledSpectrum.Empty();
	CompiledSpectrum.AddZeroed(TargetLength);
	NumBands = TargetLength; 
}

void ASpectrumManager::FireOnSpectrumUpdatedEvent(const int index, const float value)
{
     FSpectrumData NewData;
     NewData.index = index;
     NewData.value = value;
     OnSpectrumUpdatedEvent.Broadcast(NewData);
}