#if WITH_DEV_AUTOMATION_TESTS

#include "RTCQTResponseProcessor.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTResponseBasics,
	"RTCQT.D09.Response.BasicCalculations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTResponseBasics::RunTest(const FString& Parameters)
{
	FRTCQTResponseParameters Settings;
	TestEqual(TEXT("Silence stays at baseline"), FRTCQTResponseProcessor::ProcessBand(0.0f, Settings, 0), 0.0f, 1.e-6f);
	TestEqual(TEXT("Default midpoint is quadratic"), FRTCQTResponseProcessor::ProcessBand(0.55f, Settings, 0), 0.25f, 1.e-6f);
	TestEqual(TEXT("Ceiling reaches peak"), FRTCQTResponseProcessor::ProcessBand(1.0f, Settings, 0), 1.0f, 1.e-6f);
	Settings.BandWeights = {0.5f};
	TestEqual(TEXT("Weight precedes exponent"), FRTCQTResponseProcessor::ProcessBand(0.55f, Settings, 0), 0.0625f, 1.e-6f);
	TestEqual(TEXT("Missing weight is one"), FRTCQTResponseProcessor::ProcessBand(0.55f, Settings, 1), 0.25f, 1.e-6f);
	Settings.ResponseExponent = 0.0f;
	TestEqual(TEXT("Invalid exponent does not turn silence into a peak"), FRTCQTResponseProcessor::ProcessBand(0.0f, Settings, 0), 0.0f, 1.e-6f);
	TestEqual(TEXT("Invalid exponent is neutral"), FRTCQTResponseProcessor::ProcessBand(0.55f, Settings, 0), 0.25f, 1.e-6f);
	Settings.NoiseFloor = Settings.InputCeiling;
	TestEqual(TEXT("Invalid window has zero response"), FRTCQTResponseProcessor::ProcessBand(1.0f, Settings, 0), 0.0f, 1.e-6f);

	Settings = FRTCQTResponseParameters();
	const TArray<float> Inputs = {0.0f, 0.55f, 1.0f};
	TArray<float> Outputs = {99.0f, 98.0f, 97.0f, 96.0f};
	FRTCQTResponseProcessor::ProcessBands(Inputs, Settings, Outputs);
	if (!TestEqual(TEXT("One response per input, replacing prior output"), Outputs.Num(), 3)) return false;
	TestEqual(TEXT("Batch midpoint"), Outputs[1], 0.25f, 1.e-6f);
	FRTCQTResponseProcessor::ProcessBands(TConstArrayView<float>(), Settings, Outputs);
	TestTrue(TEXT("Empty input clears prior output"), Outputs.IsEmpty());
	TestEqual(TEXT("Bounded mapping"), FRTCQTResponseProcessor::MapResponse(0.25f, 1.0f, 5.0f), 2.0f, 1.e-6f);
	const float Max = std::numeric_limits<float>::max();
	TestEqual(TEXT("Extreme bounds retain finite midpoint"), FRTCQTResponseProcessor::MapResponse(0.5f, -Max, Max), 0.0f, 1.e-6f);
	return true;
}

#endif
