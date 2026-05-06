#pragma once

#include "CoreMinimal.h"
#include "GameFramework//HUD.h"
#include "MainHUD.generated.h"

class UUserWidget;

UCLASS()
class INVENTORYBRAWL_API AMainHUD : public AHUD
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UUserWidget> RootWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UUserWidget> RootWidget = nullptr;

	virtual void BeginPlay() override;
};
