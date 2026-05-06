#include "InventoryBlueprintLibrary.h"

#include "Inventory/InventoryGridHelper.h"

TArray<FIntPoint> UInventoryBlueprintLibrary::RotateOccupiedCells(const TArray<FIntPoint>& OccupiedCells, EInventoryRotation Rotation)
{
	return FInventoryGridHelper::RotateShape(OccupiedCells, Rotation);
}

TArray<FIntPoint> UInventoryBlueprintLibrary::NormalizeOccupiedCells(const TArray<FIntPoint>& OccupiedCells)
{
	return FInventoryGridHelper::NormalizeShape(OccupiedCells);
}

FIntPoint UInventoryBlueprintLibrary::GetRotatedNormalizedShapeSize(const TArray<FIntPoint>& OccupiedCells, EInventoryRotation Rotation)
{
	const TArray<FIntPoint> Rotated = RotateOccupiedCells(OccupiedCells, Rotation);
	const TArray<FIntPoint> Normalized = NormalizeOccupiedCells(Rotated);
	int MaxX = 0;
	int MaxY = 0;

	for (int i = 0; i < Normalized.Num(); i++)
	{
		MaxX = FMath::Max(MaxX, Normalized[i].X);
		MaxY = FMath::Max(MaxY, Normalized[i].Y);
	}
	return FIntPoint(MaxX, MaxY);
}


