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
	explicit FInventoryCell(const FIntPoint& InCoordinate)
		: Coordinate(InCoordinate)
	{
	}
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

	static FInventoryOperationResultData Make(
		EInventoryOperationResult InResult,
		const FGuid& InItemId,
		const FText& InMessage)
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
