# Grid Inventory Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a reusable Unreal Engine 5.3 C++ grid inventory system with irregular connected item footprints, rotation, drag/drop movement, discard, atomic transfer, and UMG-facing APIs.

**Architecture:** Keep the gameplay core inside the existing `InventoryBrawl` runtime module. Split the system into data definitions, pure grid math helpers, runtime inventory records/result types, an authoritative `UInventoryComponent`, and thin UMG-facing widget/drag-drop classes that call component APIs instead of owning rules.

**Tech Stack:** Unreal Engine 5.3 C++, `UPrimaryDataAsset`, `UActorComponent`, `UUserWidget`, Automation Tests, UMG/Slate dependencies

---

## File Structure

### Create
- `Source/InventoryBrawl/Inventory/InventoryTypes.h`
- `Source/InventoryBrawl/Inventory/InventoryItemDefinition.h`
- `Source/InventoryBrawl/Inventory/InventoryItemDefinition.cpp`
- `Source/InventoryBrawl/Inventory/InventoryGridHelper.h`
- `Source/InventoryBrawl/Inventory/InventoryGridHelper.cpp`
- `Source/InventoryBrawl/Inventory/InventoryComponent.h`
- `Source/InventoryBrawl/Inventory/InventoryComponent.cpp`
- `Source/InventoryBrawl/UI/InventoryDragDropOperation.h`
- `Source/InventoryBrawl/UI/InventoryDragDropOperation.cpp`
- `Source/InventoryBrawl/UI/InventoryGridWidget.h`
- `Source/InventoryBrawl/UI/InventoryGridWidget.cpp`
- `Source/InventoryBrawl/Tests/InventoryGridHelperTests.cpp`
- `Source/InventoryBrawl/Tests/InventoryComponentTests.cpp`

### Modify
- `Source/InventoryBrawl/InventoryBrawl.Build.cs`

### Responsibilities
- `InventoryTypes.h`: shared enums, structs, result payloads, runtime item record, preview payloads
- `InventoryItemDefinition.*`: item data asset, canonical mask storage, runtime/editor validation entry points
- `InventoryGridHelper.*`: pure placement math, shape validation, rotation, bounds/overlap checks
- `InventoryComponent.*`: authoritative storage and mutation/query APIs, delegates, transfer/discard hooks
- `InventoryDragDropOperation.*`: UMG drag payload that carries inventory pointer, item id, anchor/rotation preview state
- `InventoryGridWidget.*`: Blueprintable base widget that exposes preview/query/commit helpers while keeping rules in C++
- `InventoryGridHelperTests.cpp`: narrow math validation coverage
- `InventoryComponentTests.cpp`: component mutation and transaction coverage

## Repo Constraints

- Per `AGENTS.md`, leave all changes unstaged.
- Do not add commit steps while executing this plan.
- Run build and test commands from `E:\Projects\InventoryBrawl`.

## Commands Used Repeatedly

- Build editor target:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
```

- Run inventory automation tests:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -ExecCmds="Automation RunTests InventoryBrawl.Inventory; Quit" -unattended -nop4 -nosplash -NullRHI -log
```

---

### Task 1: Enable Inventory Module Dependencies And Shared Types

**Files:**
- Modify: `Source/InventoryBrawl/InventoryBrawl.Build.cs`
- Create: `Source/InventoryBrawl/Inventory/InventoryTypes.h`

- [ ] **Step 1: Add the runtime dependencies the inventory UI and data assets need**

Replace the dependency list in `Source/InventoryBrawl/InventoryBrawl.Build.cs` with:

```csharp
using UnrealBuildTool;

public class InventoryBrawl : ModuleRules
{
	public InventoryBrawl(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"NavigationSystem",
			"AIModule",
			"Niagara",
			"EnhancedInput",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
```

- [ ] **Step 2: Create the shared runtime types header**

Create `Source/InventoryBrawl/Inventory/InventoryTypes.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "InventoryTypes.generated.h"

class UInventoryComponent;
class UInventoryItemDefinition;

UENUM(BlueprintType)
enum class EInventoryRotation : uint8
{
	Degrees0 UMETA(DisplayName = "0"),
	Degrees90 UMETA(DisplayName = "90"),
	Degrees180 UMETA(DisplayName = "180"),
	Degrees270 UMETA(DisplayName = "270")
};

UENUM(BlueprintType)
enum class EInventoryOperationResult : uint8
{
	Success,
	OutOfBounds,
	Overlap,
	InvalidDefinition,
	ItemNotFound,
	Unsupported
};

USTRUCT(BlueprintType)
struct FInventoryCell
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Coordinate = FIntPoint::ZeroValue;

	FInventoryCell() = default;
	explicit FInventoryCell(const FIntPoint& InCoordinate) : Coordinate(InCoordinate) {}
};

USTRUCT(BlueprintType)
struct FInventoryItemRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<const UInventoryItemDefinition> Definition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FIntPoint Anchor = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EInventoryRotation Rotation = EInventoryRotation::Degrees0;
};

USTRUCT(BlueprintType)
struct FInventoryOperationResultData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EInventoryOperationResult Result = EInventoryOperationResult::Unsupported;

	UPROPERTY(BlueprintReadOnly)
	FGuid ItemId;

	UPROPERTY(BlueprintReadOnly)
	FText Message;

	static FInventoryOperationResultData Make(EInventoryOperationResult InResult, const FGuid& InItemId, const FText& InMessage)
	{
		FInventoryOperationResultData Data;
		Data.Result = InResult;
		Data.ItemId = InItemId;
		Data.Message = InMessage;
		return Data;
	}
};

USTRUCT(BlueprintType)
struct FInventoryPlacementPreview
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EInventoryOperationResult Result = EInventoryOperationResult::Unsupported;

	UPROPERTY(BlueprintReadOnly)
	TArray<FIntPoint> OccupiedCells;
};
```

- [ ] **Step 3: Build to catch missing module or reflection errors early**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
```

Expected: build succeeds and UnrealHeaderTool generates `InventoryTypes.generated.h` without errors.

---

### Task 2: Add Item Definition Asset And Shape Validation Entry Points

**Files:**
- Create: `Source/InventoryBrawl/Inventory/InventoryItemDefinition.h`
- Create: `Source/InventoryBrawl/Inventory/InventoryItemDefinition.cpp`
- Test: `Source/InventoryBrawl/Tests/InventoryGridHelperTests.cpp`

- [ ] **Step 1: Write the first failing validation tests**

Create `Source/InventoryBrawl/Tests/InventoryGridHelperTests.cpp` with the initial test scaffold:

```cpp
#include "Misc/AutomationTest.h"
#include "Inventory/InventoryGridHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryShapeValidationTest, "InventoryBrawl.Inventory.ShapeValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryShapeValidationTest::RunTest(const FString& Parameters)
{
	{
		const TArray<FIntPoint> ValidLShape = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(0, 1)};
		FText Error;
		TestTrue(TEXT("Connected L-shape should validate"), FInventoryGridHelper::ValidateShape(ValidLShape, Error));
	}

	{
		const TArray<FIntPoint> DisconnectedShape = {FIntPoint(0, 0), FIntPoint(2, 0)};
		FText Error;
		TestFalse(TEXT("Disconnected cells should fail"), FInventoryGridHelper::ValidateShape(DisconnectedShape, Error));
	}

	return true;
}
```

- [ ] **Step 2: Create the item definition asset API**

Create `Source/InventoryBrawl/Inventory/InventoryItemDefinition.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryItemDefinition.generated.h"

UCLASS(BlueprintType)
class INVENTORYBRAWL_API UInventoryItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TArray<FIntPoint> OccupiedCells;

	bool IsShapeValid(FText& OutError) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
```

Create `Source/InventoryBrawl/Inventory/InventoryItemDefinition.cpp`:

```cpp
#include "Inventory/InventoryItemDefinition.h"
#include "Inventory/InventoryGridHelper.h"

bool UInventoryItemDefinition::IsShapeValid(FText& OutError) const
{
	return FInventoryGridHelper::ValidateShape(OccupiedCells, OutError);
}

#if WITH_EDITOR
EDataValidationResult UInventoryItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	FText Error;
	if (!IsShapeValid(Error))
	{
		Context.AddError(Error);
		return EDataValidationResult::Invalid;
	}

	return Super::IsDataValid(Context);
}
#endif
```

- [ ] **Step 3: Run the tests to verify `FInventoryGridHelper` is still missing**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
```

Expected: build fails because `Inventory/InventoryGridHelper.h` does not exist yet.

---

### Task 3: Implement The Pure Grid Helper And Expand Math Coverage

**Files:**
- Create: `Source/InventoryBrawl/Inventory/InventoryGridHelper.h`
- Create: `Source/InventoryBrawl/Inventory/InventoryGridHelper.cpp`
- Modify: `Source/InventoryBrawl/Tests/InventoryGridHelperTests.cpp`

- [ ] **Step 1: Define the pure helper API**

Create `Source/InventoryBrawl/Inventory/InventoryGridHelper.h`:

```cpp
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
		FInventoryOperationResult& OutResult,
		TArray<FIntPoint>& OutOccupiedCells);
};
```

- [ ] **Step 2: Implement validation, rotation normalization, and placement math**

Create `Source/InventoryBrawl/Inventory/InventoryGridHelper.cpp` with:

```cpp
#include "Inventory/InventoryGridHelper.h"

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
		if (!UniqueCells.Add(Cell))
		{
			OutError = FText::FromString(TEXT("Inventory shape contains duplicate occupied cells."));
			return false;
		}
	}

	TArray<FIntPoint> OpenSet;
	TSet<FIntPoint> Visited;
	OpenSet.Add(Cells[0]);

	while (!OpenSet.IsEmpty())
	{
		const FIntPoint Current = OpenSet.Pop();
		if (!Visited.Add(Current))
		{
			continue;
		}

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
				if (!Seen.Add(Current) || UniqueCells.Contains(Current))
				{
					continue;
				}

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
```

Then add the remaining methods in the same file:

```cpp
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

bool FInventoryGridHelper::IsPlacementValid(const TArray<FIntPoint>& Shape, const FIntPoint& Anchor, EInventoryRotation Rotation, const FIntPoint& GridSize, const TArray<FInventoryItemRuntimeData>& ExistingItems, const FGuid* IgnoredItemId, FInventoryOperationResult& OutResult, TArray<FIntPoint>& OutOccupiedCells)
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
```

- [ ] **Step 3: Expand the helper tests to cover rotation, hollow rejection, bounds, and overlap**

Append these checks to `Source/InventoryBrawl/Tests/InventoryGridHelperTests.cpp`:

```cpp
	{
		const TArray<FIntPoint> HollowSquare = {
			FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0),
			FIntPoint(0, 1),                     FIntPoint(2, 1),
			FIntPoint(0, 2), FIntPoint(1, 2), FIntPoint(2, 2)
		};
		FText Error;
		TestFalse(TEXT("Hollow shapes should fail"), FInventoryGridHelper::ValidateShape(HollowSquare, Error));
	}

	{
		const TArray<FIntPoint> Tee = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0), FIntPoint(1, 1)};
		const TArray<FIntPoint> Rotated = FInventoryGridHelper::RotateShape(Tee, EInventoryRotation::Degrees90);
		const TArray<FIntPoint> Expected = {FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(1, 1), FIntPoint(1, 2)};
		TestEqual(TEXT("Rotation should normalize shape"), Rotated, Expected);
	}

	{
		const TArray<FIntPoint> Cells = {FIntPoint(3, 0)};
		TestFalse(TEXT("Out-of-bounds cell should fail"), FInventoryGridHelper::AreCellsWithinBounds(Cells, FIntPoint(3, 3)));
	}

	{
		const TArray<FIntPoint> A = {FIntPoint(0, 0), FIntPoint(1, 0)};
		const TArray<FIntPoint> B = {FIntPoint(1, 0), FIntPoint(1, 1)};
		TestTrue(TEXT("Overlap should be detected"), FInventoryGridHelper::DoCellsOverlap(A, B));
	}
```

- [ ] **Step 4: Build and run the narrow helper test suite**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -ExecCmds="Automation RunTests InventoryBrawl.Inventory.ShapeValidation; Quit" -unattended -nop4 -nosplash -NullRHI -log
```

Expected: build succeeds and the shape validation test passes.

---

### Task 4: Add The Runtime Inventory Component Query Layer

**Files:**
- Create: `Source/InventoryBrawl/Inventory/InventoryComponent.h`
- Create: `Source/InventoryBrawl/Inventory/InventoryComponent.cpp`
- Modify: `Source/InventoryBrawl/Tests/InventoryComponentTests.cpp`

- [ ] **Step 1: Write the first component query test**

Create `Source/InventoryBrawl/Tests/InventoryComponentTests.cpp`:

```cpp
#include "Misc/AutomationTest.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItemDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryComponentQueryTest, "InventoryBrawl.Inventory.ComponentQuery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryComponentQueryTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>();
	Inventory->SetGridSize(FIntPoint(5, 5));

	UInventoryItemDefinition* Definition = NewObject<UInventoryItemDefinition>();
	Definition->OccupiedCells = {FIntPoint(0, 0), FIntPoint(1, 0)};

	const FInventoryOperationResultData AddResult = Inventory->TryAddItem(Definition, FIntPoint(1, 1), EInventoryRotation::Degrees0);
	TestEqual(TEXT("Add should succeed"), AddResult.Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Inventory should contain one runtime item"), Inventory->GetItems().Num(), 1);

	const TArray<FIntPoint> OccupiedCells = Inventory->GetOccupiedCellsForItem(AddResult.ItemId);
	TestEqual(TEXT("Stored item should occupy two cells"), OccupiedCells.Num(), 2);

	return true;
}
```

- [ ] **Step 2: Define the component API and delegates**

Create `Source/InventoryBrawl/Inventory/InventoryComponent.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryTypes.h"
#include "InventoryComponent.generated.h"

class UInventoryItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventoryDiscardedSignature, const FInventoryItemRuntimeData&, ItemData, const FIntPoint&, GridSize);

UCLASS(ClassGroup=(Inventory), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class INVENTORYBRAWL_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	FIntPoint GridSize = FIntPoint(5, 5);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FInventoryChangedSignature OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FInventoryDiscardedSignature OnItemDiscarded;

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
```

- [ ] **Step 3: Implement query-only behavior first**

Create `Source/InventoryBrawl/Inventory/InventoryComponent.cpp` with query scaffolding and `TryAddItem` only:

```cpp
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

FInventoryPlacementPreview UInventoryComponent::PreviewPlacement(const UInventoryItemDefinition* Definition, FIntPoint Anchor, EInventoryRotation Rotation, FGuid IgnoredItemId) const
{
	FInventoryPlacementPreview Preview;
	FText ValidationError;
	if (!ValidateDefinition(Definition, ValidationError))
	{
		Preview.Result = EInventoryOperationResult::InvalidDefinition;
		return Preview;
	}

	FInventoryOperationResult Result = EInventoryOperationResult::Unsupported;
	const FGuid* IgnoredIdPtr = IgnoredItemId.IsValid() ? &IgnoredItemId : nullptr;
	FInventoryGridHelper::IsPlacementValid(Definition->OccupiedCells, Anchor, Rotation, GridSize, Items, IgnoredIdPtr, Result, Preview.OccupiedCells);
	Preview.Result = Result;
	return Preview;
}

FInventoryOperationResultData UInventoryComponent::TryAddItem(const UInventoryItemDefinition* Definition, FIntPoint Anchor, EInventoryRotation Rotation)
{
	FText ValidationError;
	if (!ValidateDefinition(Definition, ValidationError))
	{
		return MakeFailure(EInventoryOperationResult::InvalidDefinition, FGuid(), ValidationError.ToString());
	}

	FInventoryOperationResult PlacementResult = EInventoryOperationResult::Unsupported;
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
```

Then finish the private helpers in the same file:

```cpp
const FInventoryItemRuntimeData* UInventoryComponent::FindItem(FGuid ItemId) const
{
	return Items.FindByPredicate([&ItemId](const FInventoryItemRuntimeData& Item) { return Item.ItemId == ItemId; });
}

FInventoryItemRuntimeData* UInventoryComponent::FindItemMutable(FGuid ItemId)
{
	return Items.FindByPredicate([&ItemId](const FInventoryItemRuntimeData& Item) { return Item.ItemId == ItemId; });
}

FInventoryOperationResultData UInventoryComponent::MakeFailure(EInventoryOperationResult Result, const FGuid& ItemId, const FString& Message) const
{
	return FInventoryOperationResultData::Make(Result, ItemId, FText::FromString(Message));
}

bool UInventoryComponent::ValidateDefinition(const UInventoryItemDefinition* Definition, FText& OutError) const
{
	return Definition != nullptr && Definition->IsShapeValid(OutError);
}
```

- [ ] **Step 4: Build and run the query test**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -ExecCmds="Automation RunTests InventoryBrawl.Inventory.ComponentQuery; Quit" -unattended -nop4 -nosplash -NullRHI -log
```

Expected: add/query flow passes without any move/transfer behavior implemented yet.

---

### Task 5: Implement Mutations, Atomic Transfer, And Discard

**Files:**
- Modify: `Source/InventoryBrawl/Inventory/InventoryComponent.cpp`
- Modify: `Source/InventoryBrawl/Tests/InventoryComponentTests.cpp`

- [ ] **Step 1: Extend the component tests with move, rotate, transfer, and discard coverage**

Append these tests to `Source/InventoryBrawl/Tests/InventoryComponentTests.cpp`:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryTransferTest, "InventoryBrawl.Inventory.Transfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryTransferTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Source = NewObject<UInventoryComponent>();
	UInventoryComponent* Target = NewObject<UInventoryComponent>();
	Source->SetGridSize(FIntPoint(5, 5));
	Target->SetGridSize(FIntPoint(3, 3));

	UInventoryItemDefinition* Tee = NewObject<UInventoryItemDefinition>();
	Tee->OccupiedCells = {FIntPoint(0, 0), FIntPoint(1, 0), FIntPoint(2, 0), FIntPoint(1, 1)};

	const FInventoryOperationResultData AddResult = Source->TryAddItem(Tee, FIntPoint(0, 0), EInventoryRotation::Degrees0);
	TestEqual(TEXT("Seed add should succeed"), AddResult.Result, EInventoryOperationResult::Success);

	const FInventoryOperationResultData TransferResult = Source->TryTransferItem(AddResult.ItemId, Target, FIntPoint(0, 0), EInventoryRotation::Degrees90);
	TestEqual(TEXT("Transfer should succeed"), TransferResult.Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Source should be empty after transfer"), Source->GetItems().Num(), 0);
	TestEqual(TEXT("Target should contain transferred item"), Target->GetItems().Num(), 1);

	return true;
}
```

Add this second test:

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryMoveAndDiscardTest, "InventoryBrawl.Inventory.MoveAndDiscard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInventoryMoveAndDiscardTest::RunTest(const FString& Parameters)
{
	UInventoryComponent* Inventory = NewObject<UInventoryComponent>();
	Inventory->SetGridSize(FIntPoint(4, 4));

	UInventoryItemDefinition* Bar = NewObject<UInventoryItemDefinition>();
	Bar->OccupiedCells = {FIntPoint(0, 0), FIntPoint(1, 0)};

	const FInventoryOperationResultData AddResult = Inventory->TryAddItem(Bar, FIntPoint(0, 0), EInventoryRotation::Degrees0);
	TestEqual(TEXT("Seed add should succeed"), AddResult.Result, EInventoryOperationResult::Success);

	TestEqual(TEXT("Move should succeed"), Inventory->TryMoveItem(AddResult.ItemId, FIntPoint(1, 1), EInventoryRotation::Degrees0).Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Rotate should succeed"), Inventory->TryRotateItem(AddResult.ItemId, EInventoryRotation::Degrees90).Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Discard should succeed"), Inventory->DiscardItem(AddResult.ItemId).Result, EInventoryOperationResult::Success);
	TestEqual(TEXT("Inventory should be empty after discard"), Inventory->GetItems().Num(), 0);

	return true;
}
```

- [ ] **Step 2: Implement move and rotate as same-record validated mutations**

Append these methods to `Source/InventoryBrawl/Inventory/InventoryComponent.cpp`:

```cpp
FInventoryOperationResultData UInventoryComponent::TryMoveItem(FGuid ItemId, FIntPoint NewAnchor, EInventoryRotation NewRotation)
{
	FInventoryItemRuntimeData* Item = FindItemMutable(ItemId);
	if (!Item || !Item->Definition)
	{
		return MakeFailure(EInventoryOperationResult::ItemNotFound, ItemId, TEXT("Inventory item was not found."));
	}

	FInventoryOperationResult PlacementResult = EInventoryOperationResult::Unsupported;
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
```

- [ ] **Step 3: Implement atomic transfer and discard**

Append these methods to `Source/InventoryBrawl/Inventory/InventoryComponent.cpp`:

```cpp
FInventoryOperationResultData UInventoryComponent::TryTransferItem(FGuid ItemId, UInventoryComponent* TargetInventory, FIntPoint TargetAnchor, EInventoryRotation TargetRotation)
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

	FInventoryOperationResult PlacementResult = EInventoryOperationResult::Unsupported;
	TArray<FIntPoint> OccupiedCells;
	if (!FInventoryGridHelper::IsPlacementValid(Item->Definition->OccupiedCells, TargetAnchor, TargetRotation, TargetInventory->GridSize, TargetInventory->Items, nullptr, PlacementResult, OccupiedCells))
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
```

- [ ] **Step 4: Run the full inventory automation suite**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -ExecCmds="Automation RunTests InventoryBrawl.Inventory; Quit" -unattended -nop4 -nosplash -NullRHI -log
```

Expected: add, move, rotate, transfer, and discard tests pass.

---

### Task 6: Add UMG Integration Primitives For Drag, Preview, And Commit

**Files:**
- Create: `Source/InventoryBrawl/UI/InventoryDragDropOperation.h`
- Create: `Source/InventoryBrawl/UI/InventoryDragDropOperation.cpp`
- Create: `Source/InventoryBrawl/UI/InventoryGridWidget.h`
- Create: `Source/InventoryBrawl/UI/InventoryGridWidget.cpp`

- [ ] **Step 1: Create the drag-drop payload class**

Create `Source/InventoryBrawl/UI/InventoryDragDropOperation.h`:

```cpp
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
```

Create `Source/InventoryBrawl/UI/InventoryDragDropOperation.cpp`:

```cpp
#include "UI/InventoryDragDropOperation.h"
```

- [ ] **Step 2: Create the blueprintable grid widget base**

Create `Source/InventoryBrawl/UI/InventoryGridWidget.h`:

```cpp
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<UInventoryComponent> Inventory = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryPlacementPreview PreviewExistingItem(FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData CommitMove(FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FInventoryOperationResultData CommitTransfer(UInventoryComponent* SourceInventory, FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void HandlePreviewUpdated(const FInventoryPlacementPreview& Preview);
};
```

Create `Source/InventoryBrawl/UI/InventoryGridWidget.cpp`:

```cpp
#include "UI/InventoryGridWidget.h"
#include "Inventory/InventoryComponent.h"

FInventoryPlacementPreview UInventoryGridWidget::PreviewExistingItem(FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation) const
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

FInventoryOperationResultData UInventoryGridWidget::CommitTransfer(UInventoryComponent* SourceInventory, FGuid ItemId, FIntPoint Anchor, EInventoryRotation Rotation)
{
	return (Inventory && SourceInventory) ? SourceInventory->TryTransferItem(ItemId, Inventory, Anchor, Rotation) : FInventoryOperationResultData();
}
```

- [ ] **Step 3: Build to verify the UI-facing layer compiles cleanly**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
```

Expected: the new UMG types compile without introducing gameplay rules into widgets.

---

### Task 7: Editor Setup And Manual Verification Pass

**Files:**
- No new C++ files required

- [ ] **Step 1: Create the inventory definition folder**

In the Content Browser:

```text
Content/Inventory/
Content/Inventory/Definitions/
```

Expected:
- `Definitions` exists and is empty before the new data assets are added.

- [ ] **Step 2: Create the first item definition asset**

In `Content/Inventory/Definitions/`, create a `UInventoryItemDefinition` data asset named:

```text
DA_Item_Bar2
```

Set:

```text
DisplayName = "Bar 2"
OccupiedCells = [(0,0), (1,0)]
```

Expected:
- Saving the asset produces no validation error.

- [ ] **Step 3: Create the second item definition asset**

Create:

```text
DA_Item_LShape
```

Set:

```text
DisplayName = "L Shape"
OccupiedCells = [(0,0), (1,0), (0,1)]
```

Expected:
- Saving the asset produces no validation error.

- [ ] **Step 4: Create the third item definition asset**

Create:

```text
DA_Item_TShape
```

Set:

```text
DisplayName = "T Shape"
OccupiedCells = [(0,0), (1,0), (2,0), (1,1)]
```

Expected:
- Saving the asset produces no validation error.

- [ ] **Step 5: Create the player inventory widget Blueprint**

Create a Widget Blueprint derived from `UInventoryGridWidget`:

```text
WBP_PlayerInventoryGrid
```

Expected:
- The Blueprint parent class is `UInventoryGridWidget`.
- The Blueprint compiles without warnings.

- [ ] **Step 6: Create the container inventory widget Blueprint**

Create a Widget Blueprint derived from `UInventoryGridWidget`:

```text
WBP_ContainerInventoryGrid
```

Expected:
- The Blueprint parent class is `UInventoryGridWidget`.
- The Blueprint compiles without warnings.

- [ ] **Step 7: Add a visible grid presentation to each widget**

In both widget Blueprints, add enough UI to make grid cells and item visuals inspectable during PIE. At minimum:
- a root panel
- a visual area representing the inventory grid
- a way to show item widgets or placeholders over that grid

Expected:
- Both widgets can be placed in another UI and visibly distinguish player vs container inventory.

- [ ] **Step 8: Bind each widget instance to a real `UInventoryComponent`**

Decide which actor owns the player inventory and which actor owns the container inventory, then assign the `Inventory` property on each widget instance to the corresponding component.

Expected:
- `WBP_PlayerInventoryGrid.Inventory` points to the player-side component.
- `WBP_ContainerInventoryGrid.Inventory` points to the container-side component.

- [ ] **Step 9: Seed both inventories with known test data**

Before running interaction tests, ensure you can create runtime items in the two inventory components so the widgets start with visible items. Use at least:
- one `DA_Item_Bar2`
- one `DA_Item_LShape`
- one `DA_Item_TShape`

Expected:
- At least one item is visible in the player inventory.
- At least one transfer target area is visible in the container inventory.

- [ ] **Step 10: Wire drag start to create `UInventoryDragDropOperation`**

When the user begins dragging an item widget:
- create `UInventoryDragDropOperation`
- set `SourceInventory`
- set `ItemId`
- initialize `PendingRotation`

Expected:
- Dragging an item starts a UMG drag operation carrying the correct runtime item id.

- [ ] **Step 11: Wire hover/drag-over preview**

During drag-over or hover on a grid cell:
- convert cursor position to target grid coordinates
- call `PreviewExistingItem(ItemId, Anchor, Rotation)`
- pass the returned preview into `HandlePreviewUpdated`

Expected:
- Valid targets and invalid targets can be visually distinguished.

- [ ] **Step 12: Wire rotate input during drag**

While dragging:
- update `PendingRotation` on the `UInventoryDragDropOperation`
- use that updated rotation when calling `PreviewExistingItem`

Expected:
- The preview footprint changes immediately when rotation input is triggered.

- [ ] **Step 13: Wire drop to same-inventory move**

If `SourceInventory` equals the widget's `Inventory`:
- call `CommitMove(ItemId, Anchor, PendingRotation)`

Expected:
- Successful drops move the existing runtime item.
- Invalid drops leave the item in its original location.

- [ ] **Step 14: Wire drop to cross-inventory transfer**

If `SourceInventory` differs from the widget's `Inventory`:
- call `CommitTransfer(SourceInventory, ItemId, Anchor, PendingRotation)`

Expected:
- Successful drops remove the item from the source and add it to the target.
- Invalid drops leave the source inventory unchanged.

- [ ] **Step 15: Wire a discard action**

Provide a temporary discard button, context action, or key path that calls:

```text
Inventory->DiscardItem(ItemId)
```

Expected:
- The item disappears from the source inventory UI.
- Any debug output or Blueprint listener bound to `OnItemDiscarded` fires once.

- [ ] **Step 16: Run manual verification in PIE and record outcomes**

Use this checklist during one or more PIE runs:

```text
[ ] Move within one inventory to a valid location succeeds.
[ ] Move within one inventory to an invalid location is rejected.
[ ] Overlap preview is visually distinct from valid preview.
[ ] Out-of-bounds preview is visually distinct from valid preview.
[ ] Rotating during drag updates the preview footprint.
[ ] Rotating and dropping into a valid position succeeds.
[ ] Transfer from player inventory to container succeeds.
[ ] Transfer to an invalid container position is rejected without item loss.
[ ] Discard removes the runtime record from the inventory.
[ ] Discard fires the expected delegate or debug hook once.
```

Record for each failed check:
- what you dragged
- source widget
- target widget
- expected result
- observed result

- [ ] **Step 17: Final regression build and automated tests**

Run:

```powershell
C:\Program Files\Epic Games\UE_5.3\Engine\Build\BatchFiles\Build.bat InventoryBrawlEditor Win64 Development "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -WaitMutex -FromMsBuild
C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe "E:\Projects\InventoryBrawl\InventoryBrawl.uproject" -ExecCmds="Automation RunTests InventoryBrawl.Inventory; Quit" -unattended -nop4 -nosplash -NullRHI -log
```

Expected:
- C++ inventory automation tests still pass after the editor wiring work.
- Manual UMG interaction results from Step 16 are the only remaining source of behavior validation.

---

## Self-Review

### Spec Coverage
- `UPrimaryDataAsset` item definitions: covered in Task 2.
- Pure grid math helper and validation: covered in Task 3.
- Runtime records and authoritative inventory component: covered in Tasks 1, 4, and 5.
- Rotation, drag/drop move, discard, atomic transfer: covered in Tasks 5 and 6.
- UMG-driven preview/interaction shell: covered in Task 6.
- Automated C++ tests and manual verification: covered in Tasks 3, 5, and 7.

### Placeholder Scan
- No `TODO`/`TBD` placeholders remain.
- Manual editor work is listed explicitly rather than deferred vaguely.

### Type Consistency
- Shared runtime types are introduced first in `InventoryTypes.h` and reused consistently in later tasks.
- `TryAddItem`, `TryMoveItem`, `TryRotateItem`, `TryTransferItem`, `DiscardItem`, and `PreviewPlacement` names match across tasks.
- Rotation uses `EInventoryRotation` consistently in helpers, component methods, and UMG payloads.
```
