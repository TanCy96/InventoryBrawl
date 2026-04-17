#include "Inventory/InventoryGridHelper.h"

#include "Algo/MinElement.h"
#include "Algo/MaxElement.h"
#include "Inventory/InventoryItemDefinition.h"

namespace
{
	TArray<FIntPoint> CollectNeighbors(const FIntPoint& Cell)
	{
		return {
			Cell + FIntPoint(1, 0),
			Cell + FIntPoint(-1, 0),
			Cell + FIntPoint(0, 1),
			Cell + FIntPoint(0, -1)
		};
	}
}

bool FInventoryGridHelper::ValidateShape(const TArray<FIntPoint>& Cells, FText& OutError)
{
	if (Cells.IsEmpty())
	{
		OutError = FText::FromString(TEXT("Inventory shape must contain at least one occupied cell."));
		return false;
	}

	TSet<FIntPoint> UniqueCells;
	for (const FIntPoint& Cell : Cells)
	{
		if (UniqueCells.Contains(Cell))
		{
			OutError = FText::FromString(TEXT("Inventory shape contains duplicate occupied cells."));
			return false;
		}

		UniqueCells.Add(Cell);
	}

	TArray<FIntPoint> OpenSet;
	TSet<FIntPoint> Visited;
	OpenSet.Add(Cells[0]);

	while (!OpenSet.IsEmpty())
	{
		const FIntPoint Current = OpenSet.Pop();
		if (Visited.Contains(Current))
		{
			continue;
		}

		Visited.Add(Current);

		for (const FIntPoint& Neighbor : CollectNeighbors(Current))
		{
			if (UniqueCells.Contains(Neighbor) && !Visited.Contains(Neighbor))
			{
				OpenSet.Add(Neighbor);
			}
		}
	}

	if (Visited.Num() != UniqueCells.Num())
	{
		OutError = FText::FromString(TEXT("Inventory shape must be 4-neighbor connected."));
		return false;
	}

	const int32 MinX = Algo::MinElementBy(Cells, [](const FIntPoint& Cell) { return Cell.X; })->X;
	const int32 MaxX = Algo::MaxElementBy(Cells, [](const FIntPoint& Cell) { return Cell.X; })->X;
	const int32 MinY = Algo::MinElementBy(Cells, [](const FIntPoint& Cell) { return Cell.Y; })->Y;
	const int32 MaxY = Algo::MaxElementBy(Cells, [](const FIntPoint& Cell) { return Cell.Y; })->Y;

	for (int32 Y = MinY + 1; Y < MaxY; ++Y)
	{
		for (int32 X = MinX + 1; X < MaxX; ++X)
		{
			const FIntPoint Probe(X, Y);
			if (UniqueCells.Contains(Probe))
			{
				continue;
			}

			bool bTouchesExterior = false;
			TArray<FIntPoint> Flood;
			TSet<FIntPoint> Seen;
			Flood.Add(Probe);

			while (!Flood.IsEmpty())
			{
				const FIntPoint Current = Flood.Pop();
				if (Seen.Contains(Current) || UniqueCells.Contains(Current))
				{
					continue;
				}

				Seen.Add(Current);

				if (Current.X < MinX || Current.X > MaxX || Current.Y < MinY || Current.Y > MaxY)
				{
					bTouchesExterior = true;
					break;
				}

				for (const FIntPoint& Neighbor : CollectNeighbors(Current))
				{
					Flood.Add(Neighbor);
				}
			}

			if (!bTouchesExterior)
			{
				OutError = FText::FromString(TEXT("Inventory shape cannot contain hollow cells."));
				return false;
			}
		}
	}

	return true;
}

TArray<FIntPoint> FInventoryGridHelper::RotateShape(const TArray<FIntPoint>& Cells, EInventoryRotation Rotation)
{
	TArray<FIntPoint> Rotated;
	Rotated.Reserve(Cells.Num());

	for (const FIntPoint& Cell : Cells)
	{
		switch (Rotation)
		{
		case EInventoryRotation::Degrees90:
			Rotated.Add(FIntPoint(-Cell.Y, Cell.X));
			break;
		case EInventoryRotation::Degrees180:
			Rotated.Add(FIntPoint(-Cell.X, -Cell.Y));
			break;
		case EInventoryRotation::Degrees270:
			Rotated.Add(FIntPoint(Cell.Y, -Cell.X));
			break;
		default:
			Rotated.Add(Cell);
			break;
		}
	}

	return NormalizeShape(Rotated);
}

TArray<FIntPoint> FInventoryGridHelper::NormalizeShape(const TArray<FIntPoint>& Cells)
{
	TArray<FIntPoint> Normalized = Cells;
	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;

	// Compute both minima in one pass instead of calling Algo::MinElementBy twice.
	for (const FIntPoint& Cell : Normalized)
	{
		MinX = FMath::Min(MinX, Cell.X);
		MinY = FMath::Min(MinY, Cell.Y);
	}

	for (FIntPoint& Cell : Normalized)
	{
		Cell.X -= MinX;
		Cell.Y -= MinY;
	}

	Normalized.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.Y == B.Y ? A.X < B.X : A.Y < B.Y;
	});

	return Normalized;
}

TArray<FIntPoint> FInventoryGridHelper::GetOccupiedCells(const TArray<FIntPoint>& Shape, const FIntPoint& Anchor, EInventoryRotation Rotation)
{
	TArray<FIntPoint> Occupied = RotateShape(Shape, Rotation);
	for (FIntPoint& Cell : Occupied)
	{
		Cell += Anchor;
	}

	return Occupied;
}

bool FInventoryGridHelper::AreCellsWithinBounds(const TArray<FIntPoint>& Cells, const FIntPoint& GridSize)
{
	for (const FIntPoint& Cell : Cells)
	{
		if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= GridSize.X || Cell.Y >= GridSize.Y)
		{
			return false;
		}
	}

	return true;
}

bool FInventoryGridHelper::DoCellsOverlap(const TArray<FIntPoint>& A, const TArray<FIntPoint>& B)
{
	TSet<FIntPoint> SetA(A);
	for (const FIntPoint& Cell : B)
	{
		if (SetA.Contains(Cell))
		{
			return true;
		}
	}

	return false;
}

bool FInventoryGridHelper::IsPlacementValid(
	const TArray<FIntPoint>& Shape,
	const FIntPoint& Anchor,
	EInventoryRotation Rotation,
	const FIntPoint& GridSize,
	const TArray<FInventoryItemRuntimeData>& ExistingItems,
	const FGuid* IgnoredItemId,
	EInventoryOperationResult& OutResult,
	TArray<FIntPoint>& OutOccupiedCells)
{
	OutOccupiedCells = GetOccupiedCells(Shape, Anchor, Rotation);
	if (!AreCellsWithinBounds(OutOccupiedCells, GridSize))
	{
		OutResult = EInventoryOperationResult::OutOfBounds;
		return false;
	}

	for (const FInventoryItemRuntimeData& Item : ExistingItems)
	{
		if (IgnoredItemId != nullptr && Item.ItemId == *IgnoredItemId)
		{
			continue;
		}

		if (!Item.Definition)
		{
			OutResult = EInventoryOperationResult::InvalidDefinition;
			return false;
		}

		const TArray<FIntPoint> ExistingCells = GetOccupiedCells(Item.Definition->OccupiedCells, Item.Anchor, Item.Rotation);
		if (DoCellsOverlap(OutOccupiedCells, ExistingCells))
		{
			OutResult = EInventoryOperationResult::Overlap;
			return false;
		}
	}

	OutResult = EInventoryOperationResult::Success;
	return true;
}
