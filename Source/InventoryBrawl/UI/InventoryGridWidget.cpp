#include "UI/InventoryGridWidget.h"

#include "Inventory/InventoryComponent.h"

void UInventoryGridWidget::SetInventory(UInventoryComponent* NewInventory)
{
	if (Inventory == NewInventory) return;

	if (Inventory)
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryGridWidget::OnBoundInventoryChanged);
	}

	Inventory = NewInventory;

	if (Inventory)
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &UInventoryGridWidget::OnBoundInventoryChanged);
	}

	HandleInventoryAssigned();
	HandleInventoryItemsChanged();
}

FInventoryPlacementPreview UInventoryGridWidget::PreviewExistingItem(
	FGuid ItemId,
	FIntPoint Anchor,
	EInventoryRotation Rotation) const
{
	if (!Inventory)
	{
		return {};
	}

	const FInventoryItemRuntimeData* Item = Inventory->GetItems().FindByPredicate([&ItemId](const FInventoryItemRuntimeData& Candidate)
	{
		return Candidate.ItemId == ItemId;
	});

	if (!Item || !Item->Definition)
	{
		return {};
	}

	return Inventory->PreviewPlacement(Item->Definition, Anchor, Rotation, ItemId);
}

FInventoryOperationResultData UInventoryGridWidget::CommitMove(FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation)
{
	return Inventory ? Inventory->TryMoveItem(ItemId, Anchor, Rotation) : FInventoryOperationResultData();
}

FInventoryOperationResultData UInventoryGridWidget::CommitTransfer(
	UInventoryComponent* SourceInventory,
	FGuid ItemId,
	FIntPoint Anchor,
	EInventoryRotation Rotation)
{
	return (Inventory && SourceInventory)
		? SourceInventory->TryTransferItem(ItemId, Inventory, Anchor, Rotation)
		: FInventoryOperationResultData();
}

void UInventoryGridWidget::NativeDestruct()
{
	if (Inventory)
	{
		Inventory->OnInventoryChanged.RemoveDynamic(this, &UInventoryGridWidget::OnBoundInventoryChanged);
	}

	Super::NativeDestruct();
}

void UInventoryGridWidget::OnBoundInventoryChanged()
{
	HandleInventoryItemsChanged();
}
