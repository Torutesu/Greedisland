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
    if (!PlayerController || !DebugHudWidgetClass)
    {
        return nullptr;
    }

    DebugHudWidget = CreateWidget<UGreeislandDebugHudWidget>(PlayerController, DebugHudWidgetClass);
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

