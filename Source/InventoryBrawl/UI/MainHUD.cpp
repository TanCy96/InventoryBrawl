#include "MainHUD.h"
#include "Blueprint/UserWidget.h"

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!RootWidgetClass) return;

	APlayerController* OwningPC = GetOwningPlayerController();
	if (!OwningPC) return;

	RootWidget = CreateWidget<UUserWidget>(OwningPC, RootWidgetClass);
	if (!RootWidget) return;
	RootWidget->AddToViewport();
}
