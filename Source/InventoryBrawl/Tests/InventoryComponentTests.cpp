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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryTransferTest,
	"InventoryBrawl.Inventory.Transfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryTransferTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Source = NewObject<UInventoryComponent>();
	UInventoryComponent* Target = NewObject<UInventoryComponent>();
	Source->SetGridSize(FIntPoint(5, 5));
	Target->SetGridSize(FIntPoint(3, 3));

	UInventoryItemDefinition* Tee = NewObject<UInventoryItemDefinition>();
	Tee->OccupiedCells = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0), FIntPoint(1, 1)};

	const FInventoryOperationResultData AddResult = Source->TryAddItem(Tee, FIntPoint(0, 0), EInventoryRotation::Degrees0);
	TestEqual(TEXT("Seed add should succeed"), AddResult.Result, EInventoryOperationResult::Success);

	const FInventoryOperationResultData TransferResult = Source->TryTransferItem(AddResult.ItemId, Target, FIntPoint(0, 0), EInventoryRotation::Degrees90);
	TestEqual(TEXT("Transfer should succeed"), TransferResult.Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Source should be empty after transfer"), Source->GetItems().Num(), 0);
	TestEqual(TEXT("Target should contain transferred item"), Target->GetItems().Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInventoryMoveAndDiscardTest,
	"InventoryBrawl.Inventory.MoveAndDiscard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryMoveAndDiscardTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>();
	Inventory->SetGridSize(FIntPoint(4, 4));

	UInventoryItemDefinition* Bar = NewObject<UInventoryItemDefinition>();
	Bar->OccupiedCells = {FIntPoint(0, 0), FIntPoint(1, 0)};

	const FInventoryOperationResultData AddResult = Inventory->TryAddItem(Bar, FIntPoint(0, 0), EInventoryRotation::Degrees0);
	TestEqual(TEXT("Seed add should succeed"), AddResult.Result, EInventoryOperationResult::Success);

	TestEqual(
		TEXT("Move should succeed"),
		Inventory->TryMoveItem(AddResult.ItemId, FIntPoint(1, 1), EInventoryRotation::Degrees0).Result,
		EInventoryOperationResult::Success);
	TestEqual(
		TEXT("Rotate should succeed"),
		Inventory->TryRotateItem(AddResult.ItemId, EInventoryRotation::Degrees90).Result,
		EInventoryOperationResult::Success);
	TestEqual(
		TEXT("Discard should succeed"),
		Inventory->DiscardItem(AddResult.ItemId).Result,
		EInventoryOperationResult::Success);
	TestEqual(TEXT("Inventory should be empty after discard"), Inventory->GetItems().Num(), 0);

	return true;
}
