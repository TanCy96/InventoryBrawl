#pragma once

#include "CoreMinimal.h"

#include "Inventory/InventoryTypes.h"

class INVENTORYBRAWL_API FInventoryGridHelper
{
public:
	static bool ValidateShape(const TArray<FIntPoint>& Cells, FText& OutError);
	static TArray<FIntPoint> RotateShape(const TArray<FIntPoint>& Cells, EInventoryRotation Rotation);
	static TArray<FIntPoint> NormalizeShape(const TArray<FIntPoint>& Cells);
	static TArray<FIntPoint> GetOccupiedCells(const TArray<FIntPoint>& Shape, const FIntPoint& Anchor, EInventoryRotation Rotation);
	static bool AreCellsWithinBounds(const TArray<FIntPoint>& Cells, const FIntPoint& GridSize);
	static bool DoCellsOverlap(const TArray<FIntPoint>& A, const TArray<FIntPoint>& B);
	static bool IsPlacementValid(
		const TArray<FIntPoint>& Shape,
		const FIntPoint& Anchor,
		EInventoryRotation Rotation,
		const FIntPoint& GridSize,
		const TArray<FInventoryItemRuntimeData>& ExistingItems,
		const FGuid* IgnoredItemId,
		EInventoryOperationResult& OutResult,
		TArray<FIntPoint>& OutOccupiedCells);
};
