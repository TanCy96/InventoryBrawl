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
	FInventoryItemRuntimeData* Item = FindItemMutable(ItemId);
	if (!Item || !Item->Definition)
	{
		return MakeFailure(EInventoryOperationResult::ItemNotFound, ItemId, TEXT("Inventory item was not found."));
	}

	EInventoryOperationResult PlacementResult = EInventoryOperationResult::Unsupported;
	TArray<FIntPoint> OccupiedCells;
	if (!FInventoryGridHelper::IsPlacementValid(Item->Definition->OccupiedCells, NewAnchor, NewRotation, GridSize, Items, &ItemId, PlacementResult, OccupiedCells))
	{
		return MakeFailure(PlacementResult, ItemId, TEXT("Move target is invalid."));
	}

	Item->Anchor = NewAnchor;
	Item->Rotation = NewRotation;
	OnInventoryChanged.Broadcast();
	return FInventoryOperationResultData::Make(EInventoryOperationResult::Success, ItemId, FText::GetEmpty());
}

FInventoryOperationResultData UInventoryComponent::TryRotateItem(FGuid ItemId, EInventoryRotation NewRotation)
{
	const FInventoryItemRuntimeData* Item = FindItem(ItemId);
	if (!Item)
	{
		return MakeFailure(EInventoryOperationResult::ItemNotFound, ItemId, TEXT("Inventory item was not found."));
	}

	return TryMoveItem(ItemId, Item->Anchor, NewRotation);
}

FInventoryOperationResultData UInventoryComponent::TryTransferItem(
	FGuid ItemId,
	UInventoryComponent* TargetInventory,
	FIntPoint TargetAnchor,
	EInventoryRotation TargetRotation)
{
	if (!TargetInventory)
	{
		return MakeFailure(EInventoryOperationResult::Unsupported, ItemId, TEXT("Target inventory is required."));
	}

	FInventoryItemRuntimeData* Item = FindItemMutable(ItemId);
	if (!Item || !Item->Definition)
	{
		return MakeFailure(EInventoryOperationResult::ItemNotFound, ItemId, TEXT("Inventory item was not found."));
	}

	EInventoryOperationResult PlacementResult = EInventoryOperationResult::Unsupported;
	TArray<FIntPoint> OccupiedCells;
	if (!FInventoryGridHelper::IsPlacementValid(
		Item->Definition->OccupiedCells,
		TargetAnchor,
		TargetRotation,
		TargetInventory->GridSize,
		TargetInventory->Items,
		nullptr,
		PlacementResult,
		OccupiedCells))
	{
		return MakeFailure(PlacementResult, ItemId, TEXT("Transfer target is invalid."));
	}

	FInventoryItemRuntimeData TransferredItem = *Item;
	TransferredItem.Anchor = TargetAnchor;
	TransferredItem.Rotation = TargetRotation;

	Items.RemoveAll([&ItemId](const FInventoryItemRuntimeData& ExistingItem) { return ExistingItem.ItemId == ItemId; });
	TargetInventory->Items.Add(TransferredItem);

	OnInventoryChanged.Broadcast();
	TargetInventory->OnInventoryChanged.Broadcast();

	return FInventoryOperationResultData::Make(EInventoryOperationResult::Success, ItemId, FText::GetEmpty());
}

FInventoryOperationResultData UInventoryComponent::DiscardItem(FGuid ItemId)
{
	const FInventoryItemRuntimeData* Item = FindItem(ItemId);
	if (!Item)
	{
		return MakeFailure(EInventoryOperationResult::ItemNotFound, ItemId, TEXT("Inventory item was not found."));
	}

	const FInventoryItemRuntimeData DiscardedCopy = *Item;
	Items.RemoveAll([&ItemId](const FInventoryItemRuntimeData& ExistingItem) { return ExistingItem.ItemId == ItemId; });

	OnItemDiscarded.Broadcast(DiscardedCopy, GridSize);
	OnInventoryChanged.Broadcast();

	return FInventoryOperationResultData::Make(EInventoryOperationResult::Success, ItemId, FText::GetEmpty());
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
