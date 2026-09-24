#if WITH_DEV_AUTOMATION_TESTS

#include "RTCQTLayout.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include <limits>

namespace
{
bool Near(double Actual, double Expected)
{
	return FMath::IsFinite(Actual)
		&& FMath::Abs(Actual - Expected) <= 1.e-6 + 1.e-6 * FMath::Abs(Expected);
}

bool NearVector(const FVector& Actual, const FVector& Expected)
{
	return Near(Actual.X, Expected.X) && Near(Actual.Y, Expected.Y) && Near(Actual.Z, Expected.Z);
}

double ParseNumber(const TCHAR* Text)
{
	if (FCString::Stricmp(Text, TEXT("nan")) == 0) return std::numeric_limits<double>::quiet_NaN();
	if (FCString::Stricmp(Text, TEXT("inf")) == 0) return std::numeric_limits<double>::infinity();
	if (FCString::Stricmp(Text, TEXT("-inf")) == 0) return -std::numeric_limits<double>::infinity();
	return FCString::Atod(Text);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTLineGoldenVectors,
	"RTCQT.D08.Layout.GoldenVectors", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTLineGoldenVectors::RunTest(const FString& Parameters)
{
	FString Csv;
	const FString Path = FPaths::ProjectDir() / TEXT("planning/test-vectors/layout_golden_vectors.csv");
	if (!TestTrue(TEXT("Load authoritative D03 vectors"), FFileHelper::LoadFileToString(Csv, *Path))) return false;
	const FCsvParser Parser(MoveTemp(Csv));
	const FCsvParser::FRows& Rows = Parser.GetRows();
	TSet<FString> Cases;
	int32 CheckedTransforms = 0;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const auto& Row = Rows[RowIndex];
		if (Row.Num() < 2 || FCString::Strcmp(Row[1], TEXT("line")) != 0) continue;
		if (!TestEqual(TEXT("Vector column count"), Row.Num(), 26)) return false;
		const FString Case = Row[0];
		Cases.Add(Case);
		const auto VectorAt = [&Row](int32 Column)
		{
			return FVector(ParseNumber(Row[Column]), ParseNumber(Row[Column + 1]), ParseNumber(Row[Column + 2]));
		};
		FRTCQTLayoutSettings Settings;
		Settings.Spacing = static_cast<float>(ParseNumber(Row[3]));
		Settings.Radius = static_cast<float>(ParseNumber(Row[4]));
		Settings.Origin = VectorAt(5);
		Settings.BaseScale = VectorAt(8);
		TArray<FRTCQTLayoutTransform> Actual;
		Actual.AddDefaulted(); // Empty-output cases must discard prior results too.
		FRTCQTLayout::Build(Settings, FCString::Atoi(Row[2]), Actual);
		TestEqual(Case + TEXT(" count"), Actual.Num(), FCString::Atoi(Row[11]));
		if (FCString::Strlen(Row[12]) == 0) continue;
		const int32 Index = FCString::Atoi(Row[12]);
		if (!TestTrue(Case + TEXT(" index"), Actual.IsValidIndex(Index))) continue;
		const FRTCQTLayoutTransform& Item = Actual[Index];
		TestTrue(Case + TEXT(" translation"), NearVector(Item.Translation, VectorAt(13)));
		TestTrue(Case + TEXT(" forward"), NearVector(Item.Forward, VectorAt(16)));
		TestTrue(Case + TEXT(" up"), NearVector(Item.Up, VectorAt(19)));
		TestTrue(Case + TEXT(" scale"), NearVector(Item.Scale, VectorAt(22)));
		const FTransform Transform = Item.ToTransform();
		TestTrue(Case + TEXT(" finite transform"), Transform.IsValid());
		TestTrue(Case + TEXT(" converted translation"), NearVector(Transform.GetTranslation(), VectorAt(13)));
		TestTrue(Case + TEXT(" converted scale"), NearVector(Transform.GetScale3D(), VectorAt(22)));
		TestTrue(Case + TEXT(" converted forward"), NearVector(Transform.GetRotation().GetAxisX(), VectorAt(16)));
		TestTrue(Case + TEXT(" converted up"), NearVector(Transform.GetRotation().GetAxisZ(), VectorAt(19)));
		++CheckedTransforms;
	}
	TestEqual(TEXT("All seven D03 line cases ran"), Cases.Num(), 7);
	TestEqual(TEXT("All eleven D03 line transforms ran"), CheckedTransforms, 11);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTCQTLineRepeatability,
	"RTCQT.D08.Layout.RepeatabilityAndInvalidInputs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTCQTLineRepeatability::RunTest(const FString& Parameters)
{
	FRTCQTLayoutSettings Settings;
	Settings.BaseScale = FVector(0.5);
	TArray<FRTCQTLayoutTransform> Baseline;
	FRTCQTLayout::Build(Settings, 32, Baseline);
	TestEqual(TEXT("32 configured bands"), Baseline.Num(), 32);
	TestTrue(TEXT("First center"), NearVector(Baseline[0].Translation, FVector(-1550, 0, 0)));
	TestTrue(TEXT("Last center"), NearVector(Baseline.Last().Translation, FVector(1550, 0, 0)));
	TArray<FRTCQTLayoutTransform> Actual;
	FRTCQTLayoutSettings Changed = Settings;
	Changed.Origin = FVector(13, -7, 50);
	Changed.Spacing = 40;
	FRTCQTLayout::Build(Changed, 5, Actual);
	FRTCQTLayout::Build(Settings, 32, Actual);
	for (int32 Index = 0; Index < Actual.Num(); ++Index)
	{
		TestTrue(TEXT("History does not affect layout"), Actual[Index].ToTransform().Equals(Baseline[Index].ToTransform(), 0));
	}
	for (float Spacing : { -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN() })
	{
		Changed.Spacing = Spacing;
		FRTCQTLayout::Build(Changed, 32, Actual);
		TestEqual(TEXT("Invalid spacing preserves count"), Actual.Num(), 32);
		for (const auto& Item : Actual) TestTrue(TEXT("Invalid spacing collapses to origin"), NearVector(Item.Translation, Changed.Origin));
	}
	for (ERTCQTLayoutMode Mode : { ERTCQTLayoutMode::Circle, ERTCQTLayoutMode::FibonacciSphere })
	{
		Changed.Mode = Mode;
		FRTCQTLayout::Build(Changed, 32, Actual);
		TestTrue(TEXT("Deferred mode cannot reuse stale output"), Actual.IsEmpty());
	}
	return true;
}

#endif
