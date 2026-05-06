#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Inventory/InventoryTypes.h"
#include "InventoryGridWidget.generated.h"

class UInventoryComponent;
class UInventoryDragDropOperation;
class UInventoryItemDefinition;

UCLASS(Abstract, Blueprintable)
class INVENTORYBRAWL_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryComponent> Inventory = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetInventory(UInventoryComponent* NewInventory);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryPlacementPreview PreviewExistingItem(FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData CommitMove(FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData CommitTransfer(UInventoryComponent* SourceInventory, FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HandlePreviewUpdated(const FInventoryPlacementPreview& Preview);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HandleInventoryAssigned();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HandleInventoryItemsChanged();

protected:
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnBoundInventoryChanged();
};
