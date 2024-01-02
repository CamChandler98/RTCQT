// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectrumManager.h"
#include "DSP/FloatArrayMath.h"
#include "DSP/DeinterleaveView.h"
#include "Kismet/GameplayStatics.h"
#include <chrono>


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


	StartAndEnds.Reset();
	BoundaryKeys.Reset();

	Super::BeginPlay();

	CompiledSpectrum.AddZeroed(NumBands);
	FloatWindowBuffer.AddZeroed(FFTSize);
    SlidingFloatBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(FFTSize, NumHopFrames);
    UnrealSlidingBuffer = MakeUnique<Audio::TSlidingBuffer<float> >(FFTSize * 2, NumHopFrames * 2);
	
}

// Called every frame
void ASpectrumManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


void ASpectrumManager::AnalyzeAudio(const TArray<float>& AudioData)
{

	TArray<float> CCompiledSpectrum;
	// CCompiledSpectrum.AddZeroed(NumBands);

	auto StartTime = std::chrono::system_clock::now();
	for (const TArray<float>& Window : Audio::TAutoSlidingWindow<float>(*SlidingFloatBuffer, AudioData, FloatWindowBuffer, false))
    {   
		CCompiledSpectrum.Reset();
		
		for(int32 i = 0; i < SpectrumAnalyzers.Num(); i++)
		{
			TObjectPtr<URTCQTAnalyzer> CurrentAnalyzer = SpectrumAnalyzers[i];


			CurrentAnalyzer -> Analyze(Window, DoSampleProcessing, DoSpectrumProcessing);

			CCompiledSpectrum.Append(CurrentAnalyzer -> OutCQT);
		}

		SmoothSpectrum(CCompiledSpectrum);
		float ArrayMax = Audio::ArrayMaxAbsValue(CCompiledSpectrum);
		CompiledSpectrum = CCompiledSpectrum;


        for(int32 i = 0; i < CCompiledSpectrum.Num(); i++ )
		{
            FireOnSpectrumUpdatedEvent(i,CCompiledSpectrum[i], ArrayMax);
        }

		
    }

	auto EndTime = std::chrono::system_clock::now();

	std::chrono::duration<double> ElapsedSeconds = EndTime-StartTime;
	ExecutionTime = ElapsedSeconds.count();
	Sampler -> CanProcess = true;
}

void ASpectrumManager::UnrealAnalyzeAudio(const TArray<float>& AudioData)
{

	TArray<float> CCompiledSpectrum;
	CCompiledSpectrum.AddZeroed(NumBands);

	TArray<float> MonoBuffer;
	MonoBuffer.AddZeroed(FFTSize);
	TArray<float> ChannelBuffer;

	for (const TArray<float>& Window : Audio::TAutoSlidingWindow<float>(*UnrealSlidingBuffer, AudioData, FloatWindowBuffer, false))
    {   


			CCompiledSpectrum.Reset();
			Audio::TAutoDeinterleaveView<float> DeinterleaveView(Window, ChannelBuffer, 2 );
			for(auto Channel : DeinterleaveView)
			{
				Audio::ArrayMixIn(Channel.Values, MonoBuffer);		
			}

		for(int32 i = 0; i < SpectrumAnalyzers.Num(); i++)
		{
			TObjectPtr<URTCQTAnalyzer> CurrentAnalyzer = SpectrumAnalyzers[i];


			CurrentAnalyzer -> Analyze(MonoBuffer, DoSampleProcessing, DoSpectrumProcessing);

			CCompiledSpectrum.Append(CurrentAnalyzer -> OutCQT);
		}

		float ArrayMax = Audio::ArrayMaxAbsValue(CCompiledSpectrum);

		// FName AnalyzerName = AnalyzersSettings[0] -> Name;

        // for(int32 i = 0; i < CCompiledSpectrum.Num(); i++ )
		// {

        //     FireOnSpectrumUpdatedEvent(i,CCompiledSpectrum[i], ArrayMax, AnalyzerName);
        // }
		
		CompiledSpectrum = CCompiledSpectrum;
    }

	Sampler -> CanProcess = true;
}


void ASpectrumManager::CreateAnalyzers()
{
	SpectrumAnalyzers.SetNum(AnalyzersSettings.Num());

	int32 RunningTracker = 0;


	float ContinuousStartFreq = AnalyzersSettings[0] ->  CQTSettings -> StartingFrequency;
	int32 NextIndex;
	
	GetSampleConversionFactor(SampleRate);
	
	USaveSpectrumConfig* SavedSpectrumSettings =  Cast<USaveSpectrumConfig>(UGameplayStatics::LoadGameFromSlot(ManagerName, 0));

	ManagerSavedSettings = SavedSpectrumSettings;

	if(!SavedSpectrumSettings)
	{
		SavedSpectrumSettings =  Cast<USaveSpectrumConfig>(UGameplayStatics::CreateSaveGameObject(USaveSpectrumConfig::StaticClass()));
		ManagerSavedSettings = SavedSpectrumSettings;
		UE_LOG(SpectrumSettingsLog, Warning, TEXT("Save Not Found"));

	}
	else
	{
		UE_LOG(SpectrumSettingsLog, Warning, TEXT("Save Found"));
	}

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

		if(DoContinousSpectrum)
		{
			CQTSettings -> StartingFrequency = ContinuousStartFreq;

		}

		CQTSettings -> StartingFrequency *= SampleConversionFactor;

		CQTSettings -> EndingFrequency *= SampleConversionFactor;




		FName CurrentAnalyzerName = CurrentSettings -> Name;


		FString NameString = CurrentAnalyzerName.ToString() + "CQT";

		FName UpdatedName = FName(*NameString);

		TObjectPtr<URTCQTAnalyzer> CurrentAnalyzer = NewObject<URTCQTAnalyzer>(this,URTCQTAnalyzer::StaticClass(),UpdatedName);


		CurrentAnalyzer -> TotalBands = NumBands;
		CurrentAnalyzer -> GetParams(CQTSettings);
		CurrentAnalyzer -> GetCQTSettings();
		CurrentAnalyzer -> GetSpectrumProcessor(SpectrumSettings, SpectrumToggles, CurrentAnalyzerName);
		CurrentAnalyzer -> GetSampleProcessor(SampleSettings, SampleToggles, CurrentAnalyzerName);
		CurrentAnalyzer -> GenerateAnalyzer();

		if(SavedSpectrumSettings -> SpectrumSettings.Contains(CurrentAnalyzerName))
		{
		    UE_LOG(SpectrumSettingsLog, Warning, TEXT("SettignsName %s Found"), *CurrentAnalyzerName.ToString());
			ManagerSavedSettings -> LoadSettingsFromProcessor(CurrentAnalyzerName, CurrentAnalyzer -> SpectrumProcessor);

		}
		else
		{
		    UE_LOG(SpectrumSettingsLog, Warning, TEXT("SettignsName %s Not Found"), *CurrentAnalyzerName.ToString());
			ManagerSavedSettings -> SaveSettingsFromProcessor(CurrentAnalyzerName, CurrentAnalyzer -> SpectrumProcessor);
		}

		NamedAnalyzers.Add(CurrentAnalyzerName, CurrentAnalyzer);
		UGameplayStatics::SaveGameToSlot(SavedSpectrumSettings, ManagerName, 0);

		SpectrumAnalyzers[i] = CurrentAnalyzer;


		float StartFreq = CurrentAnalyzer -> _SFreq;
		float EndFreq = CurrentAnalyzer -> _EFreq;
		int32 ANumBands = CurrentAnalyzer -> _NumBands;

		StartAndEnds.FindOrAdd(RunningTracker, StartFreq);

		if(i == AnalyzersSettings.Num() - 1)
		{

			StartAndEnds.FindOrAdd(RunningTracker + (ANumBands - 1), EndFreq);
		}


		RunningTracker += ANumBands;



		AddOwnedComponent(CurrentAnalyzer);
		//get index as if the first element of the next analyzer was the next element of the previous analyzer
		NextIndex = CurrentAnalyzer -> _NumBands;
		float BandsPerOctave = CurrentAnalyzer -> _NumBandsPerOctave;

		//Calculate center 

		ContinuousStartFreq = Audio::FPseudoConstantQ::GetConstantQCenterFrequency(NextIndex, ContinuousStartFreq, BandsPerOctave );

	}



	CheckLength();
}

void ASpectrumManager::SaveProcessorSettings(FName SettingsName, USpectrumProcessor* SpectrumProcessor)
{


}

void ASpectrumManager::LoadAllProcessorSettings()
{
	USaveSpectrumConfig* SavedSpectrumSettings =  Cast<USaveSpectrumConfig>(UGameplayStatics::LoadGameFromSlot(ManagerName, 0));
	
	for(auto& Analyzer : NamedAnalyzers)
	{
		SavedSpectrumSettings -> LoadSettingsFromProcessor(Analyzer.Key, Analyzer.Value -> SpectrumProcessor);
	}
}


void ASpectrumManager::SaveAllProcessorSettings()
{
	USaveSpectrumConfig* SavedSpectrumSettings =  Cast<USaveSpectrumConfig>(UGameplayStatics::LoadGameFromSlot(ManagerName, 0));

	for(auto& Analyzer : NamedAnalyzers)
	{

		SavedSpectrumSettings -> SaveSettingsFromProcessor(Analyzer.Key, Analyzer.Value -> SpectrumProcessor);
	}


	UGameplayStatics::SaveGameToSlot(SavedSpectrumSettings, ManagerName, 0);
}


TArray<int32> ASpectrumManager::GetStartEndKeys()
{
	StartAndEnds.GetKeys(BoundaryKeys);
	return BoundaryKeys;
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


void ASpectrumManager::SmoothSpectrum(TArray<float>& CurrentCQT)
{
	TArray<float> SmoothedCQT = CurrentCQT;
			
	const int32 WindowSize = BoundarySmooth; // adjust window size as desired
	const float ScaleFactor = 1.0f / static_cast<float>(WindowSize);

	for (int32 i = 0; i < CurrentCQT.Num(); i++)
	{
		float Sum = 0.0f;
		int32 Count = 0;

		for (int32 j = -WindowSize / 2; j <= WindowSize / 2; j++)
		{
			int32 Index = FMath::Clamp(i + j, 0, CurrentCQT.Num() - 1);
			Sum += CurrentCQT[Index];
			Count++;
		}

		SmoothedCQT[i] = Sum * ScaleFactor;
	}
	
	for (int32 i = 0; i < CurrentCQT.Num(); i++)
	{
		CurrentCQT[i] = SmoothedCQT[i];
	}
}


void ASpectrumManager::SmoothBoundary(TArray<float>& SpectrumData, int32 Index, int32 ThresholdWidth )
{
	TArray<float> SmoothedSpectrum = SpectrumData;

	const int32 WindowSize = BoundarySmooth; // adjust window size as desired

	const float ScaleFactor = 1.0f / static_cast<float>(WindowSize);

	if(!SpectrumData.IsValidIndex(Index + (ThresholdWidth*2)))
	{
		return;
	}

	for(int32 i = Index; i < Index + (ThresholdWidth*2); i++)
	{
		float Sum = 0.0f;
		int32 Count = 0;

		for (int32 j = -WindowSize / 2; j <= WindowSize / 2; j++)
		{
			int32 CurrentIndex = FMath::Clamp(i + j, 0, SpectrumData.Num() - 1);
			Sum += SpectrumData[CurrentIndex];
			Count++;
		}

		SmoothedSpectrum[i] = Sum * ScaleFactor;
	}

	for (int32 i = Index; i < 2 * ThresholdWidth;  i++)
	{
		SpectrumData[i] = SmoothedSpectrum[i];
	}
}

void ASpectrumManager::GetSampleConversionFactor(float InSampleRate)
{
	SampleConversionFactor = InSampleRate / 24000;
}


void ASpectrumManager::FireOnSpectrumUpdatedEvent(const int Index, const float Value, const float Max)
{
     FSpectrumData NewData;
     NewData.Index = Index;
     NewData.Value = Value;
     NewData.Max = Max;


     OnSpectrumUpdatedEvent.Broadcast(NewData);

}