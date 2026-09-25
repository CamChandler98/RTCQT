#if WITH_DEV_AUTOMATION_TESTS

#include "RTCQTResponseProcessor.h"
#include "RTCQTResponseSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include <limits>

namespace RTCQTResponseTests
{
bool CheckClose(FAutomationTestBase& Test, const FString& Label, double Actual, double Expected)
{
	const double Tolerance = FMath::Max(1.e-6, 1.e-6 * FMath::Max(FMath::Abs(Actual), FMath::Abs(Expected)));
	return Test.TestTrue(FString::Printf(TEXT("%s: expected %.9g, got %.9g"), *Label, Expected, Actual),
		FMath::IsFinite(Actual) && FMath::Abs(Actual - Expected) <= Tolerance);
}

double ParseNumber(const TCHAR* Text)
{
	if (FCString::Stricmp(Text, TEXT("nan")) == 0) return std::numeric_limits<double>::quiet_NaN();
	if (FCString::Stricmp(Text, TEXT("inf")) == 0) return std::numeric_limits<double>::infinity();
	if (FCString::Stricmp(Text, TEXT("-inf")) == 0) return -std::numeric_limits<double>::infinity();
	return FCString::Atod(Text);
}

void CheckArray(FAutomationTestBase& Test, const FString& Label, TConstArrayView<float> Actual, TConstArrayView<float> Expected)
{
	if (!Test.TestEqual(Label + TEXT(" count"), Actual.Num(), Expected.Num())) return;
	for (int32 Index = 0; Index < Actual.Num(); ++Index)
	{
		CheckClose(Test, FString::Printf(TEXT("%s band %d"), *Label, Index), Actual[Index], Expected[Index]);
	}
}
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTResponseGoldenVectors,
	"RTCQT.D09.Response.GoldenVectors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTResponseGoldenVectors::RunTest(const FString& Parameters)
{
	using namespace RTCQTResponseTests;
	FString Csv;
	const FString Path = FPaths::ProjectDir() / TEXT("planning/test-vectors/response_golden_vectors.csv");
	if (!TestTrue(TEXT("Load authoritative D02 vectors"), FFileHelper::LoadFileToString(Csv, *Path))) return false;
	const FCsvParser Parser(MoveTemp(Csv));
	const auto& Rows = Parser.GetRows();
	if (!TestTrue(TEXT("Vector file contains header and cases"), Rows.Num() > 1)) return false;
	const TCHAR* Columns[] = {TEXT("case_id"), TEXT("input"), TEXT("noise_floor"), TEXT("input_ceiling"),
		TEXT("band_weight"), TEXT("response_exponent"), TEXT("output_min"), TEXT("output_max"),
		TEXT("expected_sanitized_input"), TEXT("expected_normalized"), TEXT("expected_weighted"),
		TEXT("expected_response"), TEXT("expected_output"), TEXT("notes")};
	if (!TestEqual(TEXT("Vector header width"), Rows[0].Num(), static_cast<int32>(UE_ARRAY_COUNT(Columns)))) return false;
	for (int32 Column = 0; Column < UE_ARRAY_COUNT(Columns); ++Column)
	{
		if (!TestEqual(TEXT("Vector column order"), FString(Rows[0][Column]), FString(Columns[Column]))) return false;
	}
	TSet<FString> Cases;
	int32 CheckedRows = 0;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const auto& Row = Rows[RowIndex];
		if (!TestEqual(TEXT("Vector row width"), Row.Num(), static_cast<int32>(UE_ARRAY_COUNT(Columns)))) return false;
		const FString Case = Row[0];
		if (!TestTrue(TEXT("Case identifier is nonempty and unique"), !Case.IsEmpty() && !Cases.Contains(Case))) return false;
		Cases.Add(Case);
		const float Input = static_cast<float>(ParseNumber(Row[1]));
		FRTCQTResponseParameters Settings;
		Settings.NoiseFloor = static_cast<float>(ParseNumber(Row[2]));
		Settings.InputCeiling = static_cast<float>(ParseNumber(Row[3]));
		if (Row[4][0] != 0) Settings.BandWeights.Add(static_cast<float>(ParseNumber(Row[4])));
		Settings.ResponseExponent = static_cast<float>(ParseNumber(Row[5]));
		const float Min = static_cast<float>(ParseNumber(Row[6]));
		const float Max = static_cast<float>(ParseNumber(Row[7]));
		const float Response = FRTCQTResponseProcessor::ProcessBand(Input, Settings, 0);
		CheckClose(*this, Case + TEXT(" response"), Response, ParseNumber(Row[11]));
		CheckClose(*this, Case + TEXT(" mapped output"), FRTCQTResponseProcessor::MapResponse(Response, Min, Max), ParseNumber(Row[12]));
		TestTrue(Case + TEXT(" normalized bounds"), Response >= 0.0f && Response <= 1.0f);

		// Neutral settings isolate the documented stages through the public API;
		// expected values still come from the CSV rather than a second implementation.
		FRTCQTResponseParameters StageSettings = Settings;
		StageSettings.ResponseExponent = 1.0f;
		CheckClose(*this, Case + TEXT(" weighted stage"), FRTCQTResponseProcessor::ProcessBand(Input, StageSettings, 0), ParseNumber(Row[10]));
		StageSettings.BandWeights.Reset();
		CheckClose(*this, Case + TEXT(" normalized stage"), FRTCQTResponseProcessor::ProcessBand(Input, StageSettings, 0), ParseNumber(Row[9]));
		CheckClose(*this, Case + TEXT(" isolated range mapping"),
			FRTCQTResponseProcessor::MapResponse(static_cast<float>(ParseNumber(Row[11])), Min, Max), ParseNumber(Row[12]));
		++CheckedRows;
	}
	TestEqual(TEXT("All 32 D02 vectors exercised in C++"), CheckedRows, 32);
	AddInfo(FString::Printf(TEXT("Checked %d D02 vectors: normalization, weighting, response, and range mapping."), CheckedRows));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTResponseBatch,
	"RTCQT.D09.Response.BatchAndWeights", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTResponseBatch::RunTest(const FString& Parameters)
{
	using namespace RTCQTResponseTests;
	FRTCQTResponseParameters Settings;
	Settings.NoiseFloor = 0.0f;
	Settings.BandWeights = {1.0f, 0.0f, 0.5f, 2.0f};
	const TArray<float> Inputs = {0.0f, 0.5f, 1.0f, 0.25f, 0.5f};
	const TArray<float> Expected = {0.0f, 0.0f, 0.25f, 0.25f, 0.25f};
	TArray<float> Outputs = {-9.0f, -8.0f, -7.0f, -6.0f, -5.0f, -4.0f};
	FRTCQTResponseProcessor::ProcessBands(Inputs, Settings, Outputs);
	CheckArray(*this, TEXT("Index-aligned short weights"), Outputs, Expected);
	Settings.BandWeights.Append({1.0f, 999.0f, -1.0f});
	FRTCQTResponseProcessor::ProcessBands(Inputs, Settings, Outputs);
	CheckArray(*this, TEXT("Extra weights ignored"), Outputs, Expected);
	CheckClose(*this, TEXT("Negative index uses missing-weight default"), FRTCQTResponseProcessor::ProcessBand(0.5f, Settings, -1), 0.25);
	CheckClose(*this, TEXT("Out-of-range index uses missing-weight default"), FRTCQTResponseProcessor::ProcessBand(0.5f, Settings, MAX_int32), 0.25);

	TArray<float> Shared = Inputs;
	FRTCQTResponseProcessor::ProcessBands(Shared, Settings, Shared);
	CheckArray(*this, TEXT("Input array reused as output"), Shared, Expected);
	Shared = Inputs;
	const TArray<float> SliceExpected = {0.25f, 0.0f, 0.015625f};
	FRTCQTResponseProcessor::ProcessBands(MakeArrayView(Shared.GetData() + 1, 3), Settings, Shared);
	CheckArray(*this, TEXT("Overlapping input slice"), Shared, SliceExpected);
	FRTCQTResponseProcessor::ProcessBands(Inputs, Settings, Settings.BandWeights);
	CheckArray(*this, TEXT("Weight array reused as output"), Settings.BandWeights, Expected);
	const TArray<float> OriginalInputs = {0.0f, 0.5f, 1.0f, 0.25f, 0.5f};
	CheckArray(*this, TEXT("Separate input remains unchanged"), Inputs, OriginalInputs);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTResponseDeterminism,
	"RTCQT.D09.Response.OrderIndependence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTResponseDeterminism::RunTest(const FString& Parameters)
{
	using namespace RTCQTResponseTests;
	FRTCQTResponseParameters Settings;
	Settings.BandWeights = {0.5f, 2.0f, 1.0f, 0.0f, 1.0f};
	const TArray<float> Inputs = {0.0f, 0.125f, 0.55f, 0.775f, 1.0f, 1.4f};
	TArray<float> Baseline;
	FRTCQTResponseProcessor::ProcessBands(Inputs, Settings, Baseline);
	FRTCQTResponseParameters OtherSettings;
	OtherSettings.NoiseFloor = 0.8f;
	OtherSettings.ResponseExponent = 0.5f;
	for (int32 Index : {5, 3, 1, 4, 2, 0})
	{
		FRTCQTResponseProcessor::ProcessBand(0.9f, OtherSettings, Index);
		const float Repeated = FRTCQTResponseProcessor::ProcessBand(Inputs[Index], Settings, Index);
		TestTrue(TEXT("Permuted calls with intervening settings return exactly the same float"), Repeated == Baseline[Index]);
	}
	TArray<float> Changed = Inputs;
	Changed[0] = std::numeric_limits<float>::max();
	TArray<float> Actual;
	FRTCQTResponseProcessor::ProcessBands(Changed, Settings, Actual);
	for (int32 Index = 1; Index < Actual.Num(); ++Index)
	{
		TestTrue(TEXT("Another band's peak cannot renormalize this band"), Actual[Index] == Baseline[Index]);
	}
	Changed[0] = -std::numeric_limits<float>::max();
	FRTCQTResponseProcessor::ProcessBands(Changed, Settings, Actual);
	for (int32 Index = 1; Index < Actual.Num(); ++Index)
	{
		TestTrue(TEXT("Another band's minimum cannot renormalize this band"), Actual[Index] == Baseline[Index]);
	}
	FRTCQTResponseProcessor::ProcessBands(Inputs, Settings, Actual);
	TestTrue(TEXT("Returning to the original input reproduces every float"), Actual == Baseline);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTResponseExtremeValues,
	"RTCQT.D09.Response.ExtremeValuesAndBounds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTResponseExtremeValues::RunTest(const FString& Parameters)
{
	using namespace RTCQTResponseTests;
	const float Max = std::numeric_limits<float>::max();
	const float Infinity = std::numeric_limits<float>::infinity();
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	FRTCQTResponseParameters Settings;
	Settings.NoiseFloor = 0.0f;
	Settings.InputCeiling = std::numeric_limits<float>::min() * 4.0f;
	CheckClose(*this, TEXT("Tiny valid input window midpoint"), FRTCQTResponseProcessor::ProcessBand(Settings.InputCeiling * 0.5f, Settings, 0), 0.25);
	CheckClose(*this, TEXT("Huge input with tiny window"), FRTCQTResponseProcessor::ProcessBand(Max, Settings, 0), 1.0);
	CheckClose(*this, TEXT("Negative huge input with tiny window"), FRTCQTResponseProcessor::ProcessBand(-Max, Settings, 0), 0.0);
	Settings.InputCeiling = 1.0f;
	for (float Invalid : {NaN, Infinity, -Infinity})
	{
		CheckClose(*this, TEXT("Non-finite sample becomes zero"), FRTCQTResponseProcessor::ProcessBand(Invalid, Settings, 0), 0.0);
		Settings.BandWeights = {Invalid};
		CheckClose(*this, TEXT("Non-finite weight disables band"), FRTCQTResponseProcessor::ProcessBand(1.0f, Settings, 0), 0.0);
		Settings.BandWeights.Reset();
		Settings.ResponseExponent = Invalid;
		CheckClose(*this, TEXT("Non-finite exponent becomes neutral"), FRTCQTResponseProcessor::ProcessBand(0.5f, Settings, 0), 0.5);
		Settings.ResponseExponent = 2.0f;
	}
	Settings.BandWeights = {Max};
	CheckClose(*this, TEXT("Huge finite weight saturates"), FRTCQTResponseProcessor::ProcessBand(0.5f, Settings, 0), 1.0);
	Settings.BandWeights.Reset();
	Settings.ResponseExponent = Max;
	CheckClose(*this, TEXT("Huge exponent remains finite"), FRTCQTResponseProcessor::ProcessBand(0.5f, Settings, 0), 0.0);
	Settings.ResponseExponent = std::numeric_limits<float>::min();
	for (float Input : {0.0f, 0.5f, 1.0f})
	{
		const float Response = FRTCQTResponseProcessor::ProcessBand(Input, Settings, 0);
		TestTrue(TEXT("Tiny positive exponent stays finite and bounded"), FMath::IsFinite(Response) && Response >= 0.0f && Response <= 1.0f);
	}
	Settings = FRTCQTResponseParameters();
	Settings.NoiseFloor = 2.0f;
	CheckClose(*this, TEXT("Floor clamps before window validation"), FRTCQTResponseProcessor::ProcessBand(1.0f, Settings, 0), 0.0);
	Settings.NoiseFloor = 0.0f;
	Settings.InputCeiling = -1.0f;
	CheckClose(*this, TEXT("Ceiling clamps before window validation"), FRTCQTResponseProcessor::ProcessBand(1.0f, Settings, 0), 0.0);

	const float Bounds[][2] = {{-Max, Max}, {Max, -Max}, {-Max, -1.0f}, {1.0f, Max}, {Max, Max}, {-Max, -Max}};
	for (const auto& Pair : Bounds)
	{
		const float Min = FMath::Min(Pair[0], Pair[1]);
		const float Upper = FMath::Max(Pair[0], Pair[1]);
		for (float Alpha : {NaN, -Infinity, Infinity, -1.0f, 0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 2.0f})
		{
			const float Output = FRTCQTResponseProcessor::MapResponse(Alpha, Pair[0], Pair[1]);
			TestTrue(TEXT("Extreme mapping stays finite and within ordered bounds"), FMath::IsFinite(Output) && Output >= Min && Output <= Upper);
		}
		TestTrue(TEXT("Lower endpoint preserved exactly"), FRTCQTResponseProcessor::MapResponse(0.0f, Pair[0], Pair[1]) == Min);
		TestTrue(TEXT("Upper endpoint preserved exactly"), FRTCQTResponseProcessor::MapResponse(1.0f, Pair[0], Pair[1]) == Upper);
	}
	for (float Invalid : {NaN, Infinity, -Infinity})
	{
		CheckClose(*this, TEXT("Non-finite direct mapping input uses baseline"), FRTCQTResponseProcessor::MapResponse(Invalid, 2.0f, 4.0f), 2.0);
		CheckClose(*this, TEXT("Non-finite output bounds use defaults"), FRTCQTResponseProcessor::MapResponse(0.5f, Invalid, Invalid), 0.5);
	}
	CheckClose(*this, TEXT("Negative mapping input clamps"), FRTCQTResponseProcessor::MapResponse(-1.0f, 2.0f, 4.0f), 2.0);
	CheckClose(*this, TEXT("Above-one mapping input clamps"), FRTCQTResponseProcessor::MapResponse(2.0f, 2.0f, 4.0f), 4.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTResponseParameterSnapshot,
	"RTCQT.D09.Response.SettingsSnapshot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTResponseParameterSnapshot::RunTest(const FString& Parameters)
{
	using namespace RTCQTResponseTests;
	URTCQTResponseSettings* Asset = NewObject<URTCQTResponseSettings>();
	Asset->NoiseFloor = 0.2f;
	Asset->InputCeiling = 0.8f;
	Asset->ResponseExponent = 3.0f;
	Asset->BandWeights = {0.5f, 2.0f};
	const FRTCQTResponseParameters Snapshot = Asset->MakeProcessorParameters();
	Asset->NoiseFloor = 0.9f;
	Asset->InputCeiling = 1.0f;
	Asset->ResponseExponent = 1.0f;
	Asset->BandWeights[0] = 100.0f;
	CheckClose(*this, TEXT("Settings snapshot retains independent values and weights"),
		FRTCQTResponseProcessor::ProcessBand(0.5f, Snapshot, 0), 0.015625);
	CheckClose(*this, TEXT("A fresh snapshot reflects changed authored settings"),
		FRTCQTResponseProcessor::ProcessBand(0.5f, Asset->MakeProcessorParameters(), 0), 0.0);
	return true;
}

#endif
