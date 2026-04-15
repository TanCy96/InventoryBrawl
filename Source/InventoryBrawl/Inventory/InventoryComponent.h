#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "Inventory/InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UInventoryItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventoryItemDiscardedSignature, FInventoryItemRuntimeData, ItemData, FIntPoint, InventoryGridSize);

UCLASS(ClassGroup = (Inventory), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class INVENTORYBRAWL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FIntPoint GridSize = FIntPoint(1, 1);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FInventoryChangedSignature OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FInventoryItemDiscardedSignature OnItemDiscarded;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetGridSize(FIntPoint InGridSize);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryItemRuntimeData>& GetItems() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FIntPoint> GetOccupiedCellsForItem(FGuid ItemId) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryPlacementPreview PreviewPlacement(const UInventoryItemDefinition* Definition, FIntPoint Anchor, EInventoryRotation Rotation, FGuid IgnoredItemId) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData TryAddItem(const UInventoryItemDefinition* Definition, FIntPoint Anchor, EInventoryRotation Rotation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData TryMoveItem(FGuid ItemId, FIntPoint NewAnchor, EInventoryRotation NewRotation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData TryRotateItem(FGuid ItemId, EInventoryRotation NewRotation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData TryTransferItem(FGuid ItemId, UInventoryComponent* TargetInventory, FIntPoint TargetAnchor, EInventoryRotation TargetRotation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData DiscardItem(FGuid ItemId);

private:
	UPROPERTY()
	TArray<FInventoryItemRuntimeData> Items;

	const FInventoryItemRuntimeData* FindItem(FGuid ItemId) const;
	FInventoryItemRuntimeData* FindItemMutable(FGuid ItemId);
	FInventoryOperationResultData MakeFailure(EInventoryOperationResult Result, const FGuid& ItemId, const FString& Message) const;
	bool ValidateDefinition(const UInventoryItemDefinition* Definition, FText& OutError) const;
};
