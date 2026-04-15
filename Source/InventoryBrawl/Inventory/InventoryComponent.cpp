#include "Inventory/InventoryComponent.h"

#include "Inventory/InventoryGridHelper.h"
#include "Inventory/InventoryItemDefinition.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::SetGridSize(FIntPoint InGridSize)
{
	GridSize = InGridSize;
}

const TArray<FInventoryItemRuntimeData>& UInventoryComponent::GetItems() const
{
	return Items;
}

TArray<FIntPoint> UInventoryComponent::GetOccupiedCellsForItem(FGuid ItemId) const
{
	const FInventoryItemRuntimeData* Item = FindItem(ItemId);
	if (!Item || !Item->Definition)
	{
		return {};
	}

	return FInventoryGridHelper::GetOccupiedCells(Item->Definition->OccupiedCells, Item->Anchor, Item->Rotation);
}

FInventoryPlacementPreview UInventoryComponent::PreviewPlacement(
	const UInventoryItemDefinition* Definition,
	FIntPoint Anchor,
	EInventoryRotation Rotation,
	FGuid IgnoredItemId) const
{
	FInventoryPlacementPreview Preview;
	FText ValidationError;
	if (!ValidateDefinition(Definition, ValidationError))
	{
		Preview.Result = EInventoryOperationResult::InvalidDefinition;
		return Preview;
	}

	EInventoryOperationResult Result = EInventoryOperationResult::Unsupported;
	const FGuid* IgnoredIdPtr = IgnoredItemId.IsValid() ? &IgnoredItemId : nullptr;
	FInventoryGridHelper::IsPlacementValid(Definition->OccupiedCells, Anchor, Rotation, GridSize, Items, IgnoredIdPtr, Result, Preview.OccupiedCells);
	Preview.Result = Result;
	return Preview;
}

FInventoryOperationResultData UInventoryComponent::TryAddItem(
	const UInventoryItemDefinition* Definition,
	FIntPoint Anchor,
	EInventoryRotation Rotation)
{
	FText ValidationError;
	if (!ValidateDefinition(Definition, ValidationError))
	{
		return MakeFailure(EInventoryOperationResult::InvalidDefinition, FGuid(), ValidationError.ToString());
	}

	EInventoryOperationResult PlacementResult = EInventoryOperationResult::Unsupported;
	TArray<FIntPoint> OccupiedCells;
	if (!FInventoryGridHelper::IsPlacementValid(Definition->OccupiedCells, Anchor, Rotation, GridSize, Items, nullptr, PlacementResult, OccupiedCells))
	{
		return MakeFailure(PlacementResult, FGuid(), TEXT("Item does not fit in target inventory."));
	}

	FInventoryItemRuntimeData& NewItem = Items.AddDefaulted_GetRef();
	NewItem.ItemId = FGuid::NewGuid();
	NewItem.Definition = Definition;
	NewItem.Anchor = Anchor;
	NewItem.Rotation = Rotation;

	OnInventoryChanged.Broadcast();
	return FInventoryOperationResultData::Make(EInventoryOperationResult::Success, NewItem.ItemId, FText::GetEmpty());
}

FInventoryOperationResultData UInventoryComponent::TryMoveItem(FGuid ItemId, FIntPoint NewAnchor, EInventoryRotation NewRotation)
{
	return MakeFailure(EInventoryOperationResult::Unsupported, ItemId, TEXT("Move is not implemented yet."));
}

FInventoryOperationResultData UInventoryComponent::TryRotateItem(FGuid ItemId, EInventoryRotation NewRotation)
{
	return MakeFailure(EInventoryOperationResult::Unsupported, ItemId, TEXT("Rotate is not implemented yet."));
}

FInventoryOperationResultData UInventoryComponent::TryTransferItem(
	FGuid ItemId,
	UInventoryComponent* TargetInventory,
	FIntPoint TargetAnchor,
	EInventoryRotation TargetRotation)
{
	return MakeFailure(EInventoryOperationResult::Unsupported, ItemId, TEXT("Transfer is not implemented yet."));
}

FInventoryOperationResultData UInventoryComponent::DiscardItem(FGuid ItemId)
{
	return MakeFailure(EInventoryOperationResult::Unsupported, ItemId, TEXT("Discard is not implemented yet."));
}

const FInventoryItemRuntimeData* UInventoryComponent::FindItem(FGuid ItemId) const
{
	return Items.FindByPredicate([&ItemId](const FInventoryItemRuntimeData& Item) { return Item.ItemId == ItemId; });
}

FInventoryItemRuntimeData* UInventoryComponent::FindItemMutable(FGuid ItemId)
{
	return Items.FindByPredicate([&ItemId](const FInventoryItemRuntimeData& Item) { return Item.ItemId == ItemId; });
}

FInventoryOperationResultData UInventoryComponent::MakeFailure(
	EInventoryOperationResult Result,
	const FGuid& ItemId,
	const FString& Message) const
{
	return FInventoryOperationResultData::Make(Result, ItemId, FText::FromString(Message));
}

bool UInventoryComponent::ValidateDefinition(const UInventoryItemDefinition* Definition, FText& OutError) const
{
	return Definition != nullptr && Definition->IsShapeValid(OutError);
}
