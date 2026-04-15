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
