#include "Misc/AutomationTest.h"

#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryComponentQueryTest,
	"InventoryBrawl.Inventory.ComponentQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryComponentQueryTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>();
	Inventory->SetGridSize(FIntPoint(5, 5));

	UInventoryItemDefinition* Definition = NewObject<UInventoryItemDefinition>();
	Definition->OccupiedCells = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1)};

	const FInventoryOperationResultData AddResult = Inventory->TryAddItem(Definition, FIntPoint(1, 1), EInventoryRotation::Degrees0);
	TestEqual(TEXT("Add should succeed"), AddResult.Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Inventory should contain one item"), Inventory->GetItems().Num(), 1);

	const TArray<FIntPoint> OccupiedCells = Inventory->GetOccupiedCellsForItem(AddResult.ItemId);
	const TArray<FIntPoint> ExpectedCells = {FIntPoint(1, 1), FIntPoint(2, 1), FIntPoint(1, 2)};
	TestEqual(TEXT("Occupied cells should be anchored into inventory space"), OccupiedCells, ExpectedCells);

	const FInventoryPlacementPreview OverlapPreview = Inventory->PreviewPlacement(
		Definition,
		FIntPoint(1, 1),
		EInventoryRotation::Degrees0,
		FGuid());
	TestEqual(TEXT("Preview should detect overlap"), OverlapPreview.Result, EInventoryOperationResult::Overlap);

	return true;
}
