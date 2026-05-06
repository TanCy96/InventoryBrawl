#pragma once

#include "CoreMinimal.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryBlueprintLibrary.generated.h"

UCLASS()
class INVENTORYBRAWL_API UInventoryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure, Category = "Inventory|Shape")
	static TArray<FIntPoint> RotateOccupiedCells(const TArray<FIntPoint>& OccupiedCells, EInventoryRotation Rotation);
	
	UFUNCTION(BlueprintPure, Category = "Inventory|Shape")
	static TArray<FIntPoint> NormalizeOccupiedCells(const TArray<FIntPoint>& OccupiedCells);
	
	UFUNCTION(BlueprintPure, Category = "Inventory|Shape")
	static FIntPoint GetRotatedNormalizedShapeSize(const TArray<FIntPoint>& OccupiedCells, EInventoryRotation Rotation);
};



