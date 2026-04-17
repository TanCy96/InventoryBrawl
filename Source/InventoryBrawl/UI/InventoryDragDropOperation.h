#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"

#include "Inventory/InventoryTypes.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryComponent;

UCLASS()
class INVENTORYBRAWL_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UInventoryComponent> SourceInventory = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	FGuid ItemId;

	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	EInventoryRotation PendingRotation = EInventoryRotation::Degrees0;
};
