// Fill out your copyright notice in the Description page of Project Settings.


#include "RTCQTAnalyzer.h"
#include "GenericPlatform/GenericPlatformMath.h"


	template<typename InType, typename OutType>
	OutType ConvertType(const TMap<InType, OutType>& InMap, const InType& InValue, const OutType& InDefaultValue)
	{
		checkf(InMap.Contains(InValue), TEXT("Unhandled value conversion"));

		if (InMap.Contains(InValue))
		{
			return InMap[InValue];
		}

		return InDefaultValue;
	}
	

	// EConstantQFFTSizeEnum to int
	int32 BPConstantQFFTSizeEnumToInt(EConstantQFFTSizeEnum InFFTSizeEnum)
	{
		static const TMap<EConstantQFFTSizeEnum, int32> FFTSizeMap = {
			{EConstantQFFTSizeEnum::Min, 		64},
			{EConstantQFFTSizeEnum::XXSmall, 	128},
			{EConstantQFFTSizeEnum::XSmall, 	256},
			{EConstantQFFTSizeEnum::Small,		512},
			{EConstantQFFTSizeEnum::Medium, 	1024},
			{EConstantQFFTSizeEnum::Large,		2048},
			{EConstantQFFTSizeEnum::XLarge, 	4096},
			{EConstantQFFTSizeEnum::XXLarge, 	8192},
			{EConstantQFFTSizeEnum::Max, 		16384}
		};

		return ConvertType(FFTSizeMap, InFFTSizeEnum, 2048);

	}

	// EFFTWindwoType to Audio::EWindowType
	Audio::EWindowType BPWindowTypeToAudioWindowType(EFFTWindowType InWindowType)
	{
		static const TMap<EFFTWindowType, Audio::EWindowType> WindowTypeMap = {
			{EFFTWindowType::None, 		Audio::EWindowType::None},
			{EFFTWindowType::Hamming, 	Audio::EWindowType::Hamming},
			{EFFTWindowType::Hann, 		Audio::EWindowType::Hann},
			{EFFTWindowType::Blackman,	Audio::EWindowType::Blackman}
		};

		return ConvertType(WindowTypeMap, InWindowType, Audio::EWindowType::None);
	}

	// ESpectrumType to Audio::ESpectrumType
	Audio::ESpectrumType BPSpectrumTypeToAudioSpectrumType(EAudioSpectrumType InSpectrumType)
	{
		static const TMap<EAudioSpectrumType, Audio::ESpectrumType> SpectrumTypeMap = {
			{EAudioSpectrumType::MagnitudeSpectrum,	Audio::ESpectrumType::MagnitudeSpectrum},
			{EAudioSpectrumType::PowerSpectrum, 	Audio::ESpectrumType::PowerSpectrum}
		};

		return ConvertType(SpectrumTypeMap, InSpectrumType, Audio::ESpectrumType::PowerSpectrum);
	}

	// EContantQNormalizationEnum to Audio::EPseudoConstantQNormalization
	Audio::EPseudoConstantQNormalization BPCQTNormalizationToAudioCQTNormalization(EConstantQNormalizationEnum InNormalization)
	{
		static const TMap<EConstantQNormalizationEnum, Audio::EPseudoConstantQNormalization> NormalizationMap = {
			{EConstantQNormalizationEnum::EqualAmplitude, 		Audio::EPseudoConstantQNormalization::EqualAmplitude},
			{EConstantQNormalizationEnum::EqualEuclideanNorm,	Audio::EPseudoConstantQNormalization::EqualEuclideanNorm},
			{EConstantQNormalizationEnum::EqualEnergy, 			Audio::EPseudoConstantQNormalization::EqualEnergy}
		};

		return ConvertType(NormalizationMap, InNormalization, Audio::EPseudoConstantQNormalization::EqualEnergy);
	}
UAnalyzerSettings::UAnalyzerSettings()
{
	 CQTSettings =  CreateDefaultSubobject<UCQTSettings>(TEXT("CQT Settings"));
}

// Sets default values for this component's properties
URTCQTAnalyzer::URTCQTAnalyzer()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.


	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void URTCQTAnalyzer::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void URTCQTAnalyzer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


Audio::FConstantQAnalyzerSettings URTCQTAnalyzer::GetCQTSettings()
{
	 int32 NumBands = GetNumBands(TotalBands, ParameterSettings -> Proportion, CeilProportion);
	 float BandsPerOctave = GetBandsPerOctave(ParameterSettings -> StartingFrequency, ParameterSettings -> EndingFrequency, NumBands );
	
	 OutCQT.AddZeroed(NumBands);
	 ConstantQSettings.BandWidthStretch = ParameterSettings -> BandWidthStretch;
	 ConstantQSettings.NumBands = NumBands;
	 ConstantQSettings.NumBandsPerOctave = BandsPerOctave;
	 ConstantQSettings.KernelLowestCenterFreq = ParameterSettings -> StartingFrequency;
	 ConstantQSettings.FFTSize = ParameterSettings -> UnrestrictedFFTSize;
	 ConstantQSettings.WindowType = BPWindowTypeToAudioWindowType(ParameterSettings -> WindowType);
			
	 ConstantQSettings.SpectrumType =  BPSpectrumTypeToAudioSpectrumType(ParameterSettings -> SpectrumType);
	 ConstantQSettings.Normalization = BPCQTNormalizationToAudioCQTNormalization(ParameterSettings -> CQTNormalization);


	return ConstantQSettings;
}

void URTCQTAnalyzer::GetSpectrumProcessor(USpectrumSettings* InSettings, FName InName)
{
	FString NameString = InName.ToString();
	FString Suffix = "Spectrum";

	FString ComponentNameString = NameString + Suffix;

	FName ComponentName = FName(*ComponentNameString);
	SpectrumProcessor = NewObject<USpectrumProcessor>(this, USpectrumProcessor::StaticClass(), ComponentName);



	SpectrumProcessor -> SetSettings(InSettings);

	SpectrumProcessor -> SetParams();
}

void URTCQTAnalyzer::GetSampleProcessor(USampleSettings* InSettings , FName InName)
{
	FString NameString = InName.ToString();
	FString Suffix = "Sample";

	FString ComponentNameString = NameString + Suffix;

	FName ComponentName = FName(*ComponentNameString);

	SampleProcessor = NewObject<USampleProcessor>(this, USampleProcessor::StaticClass(), ComponentName);

	SampleProcessor -> SetSettings(InSettings);

	SampleProcessor -> SetParams();
}

void URTCQTAnalyzer::GetParams(UCQTSettings* InSettings)
{
	ParameterSettings = InSettings;
}

int32 URTCQTAnalyzer::GetNumBands(int32 BandTotal, float Proportion, bool doCeil)
{
	if(doCeil)
	{

		return FGenericPlatformMath::CeilToInt(static_cast<float>(BandTotal) * Proportion);

	}
	else
	{
		return FGenericPlatformMath::FloorToInt(static_cast<float>(BandTotal) * Proportion);

	}
}

float URTCQTAnalyzer::GetBandsPerOctave(float BaseFrequency, float EndFrequency, int32 NumBands)
{

	int32 Index = NumBands - 1;
	float LogBase =  FGenericPlatformMath::LogX(2.0f,10.0f);
	float LogFactor = FGenericPlatformMath::LogX((EndFrequency/BaseFrequency), 10.0f);

	float BandsPerOctave = static_cast<float>(Index) * (LogBase/LogFactor);
	return BandsPerOctave;
}

void URTCQTAnalyzer::GenerateAnalyzer()
{
	Audio::FFocusSettings focusSettings = Audio::FFocusSettings();

	ConstantQAnalyzer = MakeUnique<Audio::FConstantQAnalyzer>(ConstantQSettings, focusSettings, ParameterSettings -> SampleRate);
}

void URTCQTAnalyzer::Analyze(TArray<float> AudioData, bool bProcessSamples, bool bProcessSpectrum)
{		
	if(bProcessSamples)
	{
		SampleProcessor -> ProcessAudio(AudioData, SampleToggles);
	}

	ConstantQAnalyzer -> CalculateCQT(AudioData.GetData(), OutCQT);

	if(bProcessSpectrum)
	{
		SpectrumProcessor -> ProcessSpectrum(AudioData, SpectrumToggles);
	}
}