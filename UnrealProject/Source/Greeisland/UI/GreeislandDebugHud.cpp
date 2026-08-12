#include "UI/GreeislandDebugHud.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void AGreeislandDebugHud::BeginPlay()
{
    Super::BeginPlay();

    if (bCreateWidgetOnBeginPlay)
    {
        EnsureDebugHudWidget();
    }
}

UGreeislandDebugHudWidget* AGreeislandDebugHud::EnsureDebugHudWidget()
{
    if (DebugHudWidget)
    {
        return DebugHudWidget;
    }

    APlayerController* PlayerController = GetOwningPlayerController();
    if (!PlayerController)
    {
        return nullptr;
    }

    TSubclassOf<UGreeislandDebugHudWidget> WidgetClass = DebugHudWidgetClass;
    if (!WidgetClass)
    {
        WidgetClass = UGreeislandDebugHudWidget::StaticClass();
    }

    DebugHudWidget = CreateWidget<UGreeislandDebugHudWidget>(PlayerController, WidgetClass);
    if (!DebugHudWidget)
    {
        return nullptr;
    }

    DebugHudWidget->AddToViewport(AddToViewportZOrder);
    return DebugHudWidget;
}

void AGreeislandDebugHud::RemoveDebugHudWidget()
{
    if (!DebugHudWidget)
    {
        return;
    }

    DebugHudWidget->RemoveFromParent();
    DebugHudWidget = nullptr;
}
