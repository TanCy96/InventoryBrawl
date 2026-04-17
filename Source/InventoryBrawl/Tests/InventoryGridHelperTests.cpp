#include "Misc/AutomationTest.h"

#include "Inventory/InventoryGridHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryShapeValidationTest,
	"InventoryBrawl.Inventory.ShapeValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryShapeValidationTest::RunTest(const FString& Parameters)
{
	{
		const TArray<FIntPoint> ValidLShape = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1)};
		FText Error;
		TestTrue(TEXT("Connected L-shape should validate"), FInventoryGridHelper::ValidateShape(ValidLShape, Error));
	}

	{
		const TArray<FIntPoint> DisconnectedShape = {FIntPoint(0, 0), FIntPoint(2, 0)};
		FText Error;
		TestFalse(TEXT("Disconnected cells should fail"), FInventoryGridHelper::ValidateShape(DisconnectedShape, Error));
	}

	{
		const TArray<FIntPoint> HollowSquare = {
			FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0),
			FIntPoint(0, 1),                    FIntPoint(2, 1),
			FIntPoint(0, 2), FIntPoint(1, 2), FIntPoint(2, 2)
		};
		FText Error;
		TestFalse(TEXT("Hollow shapes should fail"), FInventoryGridHelper::ValidateShape(HollowSquare, Error));
	}

	{
		const TArray<FIntPoint> Tee = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0), FIntPoint(1, 1)};
		const TArray<FIntPoint> Rotated = FInventoryGridHelper::RotateShape(Tee, EInventoryRotation::Degrees90);
		const TArray<FIntPoint> Expected = {FIntPoint(1, 0), FIntPoint(0, 1), FIntPoint(1, 1), FIntPoint(1, 2)};
		TestEqual(TEXT("Rotation should normalize shape"), Rotated, Expected);
	}

	{
		const TArray<FIntPoint> Cells = {FIntPoint(3, 0)};
		TestFalse(TEXT("Out-of-bounds cell should fail"), FInventoryGridHelper::AreCellsWithinBounds(Cells, FIntPoint(3, 3)));
	}

	{
		const TArray<FIntPoint> A = {FIntPoint(0, 0), FIntPoint(1, 0)};
		const TArray<FIntPoint> B = {FIntPoint(1, 0), FIntPoint(1, 1)};
		TestTrue(TEXT("Overlap should be detected"), FInventoryGridHelper::DoCellsOverlap(A, B));
	}

	return true;
}
