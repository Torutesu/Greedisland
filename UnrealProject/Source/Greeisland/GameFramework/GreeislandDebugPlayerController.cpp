#include "GameFramework/GreeislandDebugPlayerController.h"

#include "UI/GreeislandDebugHud.h"
#include "UI/GreeislandDebugHudWidget.h"

AGreeislandDebugPlayerController::AGreeislandDebugPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void AGreeislandDebugPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    check(InputComponent);
    InputComponent->BindAction(TEXT("InitializeSession"), IE_Pressed, this, &AGreeislandDebugPlayerController::InitializeSessionHotkey);
    InputComponent->BindAction(TEXT("RestoreSession"), IE_Pressed, this, &AGreeislandDebugPlayerController::RestoreSessionHotkey);
    InputComponent->BindAction(TEXT("SaveSession"), IE_Pressed, this, &AGreeislandDebugPlayerController::SaveSessionHotkey);
    InputComponent->BindAction(TEXT("ResolveActiveEvent"), IE_Pressed, this, &AGreeislandDebugPlayerController::ResolveActiveEventHotkey);
    InputComponent->BindAction(TEXT("StartCombat"), IE_Pressed, this, &AGreeislandDebugPlayerController::StartCombatHotkey);
    InputComponent->BindAction(TEXT("RunEnemyTurn"), IE_Pressed, this, &AGreeislandDebugPlayerController::RunEnemyTurnHotkey);
    InputComponent->BindAction(TEXT("BuildFallbackAi"), IE_Pressed, this, &AGreeislandDebugPlayerController::BuildFallbackAiHotkey);
    InputComponent->BindAction(TEXT("ApplyAi"), IE_Pressed, this, &AGreeislandDebugPlayerController::ApplyAiHotkey);
    InputComponent->BindAction(TEXT("PlayHandCard1"), IE_Pressed, this, &AGreeislandDebugPlayerController::PlayHandCardSlot1);
    InputComponent->BindAction(TEXT("PlayHandCard2"), IE_Pressed, this, &AGreeislandDebugPlayerController::PlayHandCardSlot2);
    InputComponent->BindAction(TEXT("PlayHandCard3"), IE_Pressed, this, &AGreeislandDebugPlayerController::PlayHandCardSlot3);
    InputComponent->BindAction(TEXT("PlayHandCard4"), IE_Pressed, this, &AGreeislandDebugPlayerController::PlayHandCardSlot4);
    InputComponent->BindAction(TEXT("PlayHandCard5"), IE_Pressed, this, &AGreeislandDebugPlayerController::PlayHandCardSlot5);
}

UGreeislandDebugHudWidget* AGreeislandDebugPlayerController::GetDebugHudWidget() const
{
    const AGreeislandDebugHud* DebugHud = Cast<AGreeislandDebugHud>(GetHUD());
    return DebugHud ? DebugHud->GetDebugHudWidget() : nullptr;
}

void AGreeislandDebugPlayerController::InitializeSessionHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->InitializeNewSession();
}

void AGreeislandDebugPlayerController::RestoreSessionHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->RestoreSession();
}

void AGreeislandDebugPlayerController::SaveSessionHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->SaveSession();
}

void AGreeislandDebugPlayerController::ResolveActiveEventHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->ResolveActiveEvent();
}

void AGreeislandDebugPlayerController::StartCombatHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->StartCombatForActiveEvent();
}

void AGreeislandDebugPlayerController::RunEnemyTurnHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->RunEnemyTurn();
}

void AGreeislandDebugPlayerController::BuildFallbackAiHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget())
    {
        FAiGmResponse Response;
        Widget->BuildFallbackAiResponseForActiveEvent(TEXT("交渉する"), Response);
    }
}

void AGreeislandDebugPlayerController::ApplyAiHotkey()
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget()) Widget->ApplyLastAiResponse();
}

void AGreeislandDebugPlayerController::PlayHandCardSlot1() { PlayHandCardAtSlot(0); }
void AGreeislandDebugPlayerController::PlayHandCardSlot2() { PlayHandCardAtSlot(1); }
void AGreeislandDebugPlayerController::PlayHandCardSlot3() { PlayHandCardAtSlot(2); }
void AGreeislandDebugPlayerController::PlayHandCardSlot4() { PlayHandCardAtSlot(3); }
void AGreeislandDebugPlayerController::PlayHandCardSlot5() { PlayHandCardAtSlot(4); }

void AGreeislandDebugPlayerController::PlayHandCardAtSlot(int32 SlotIndex)
{
    if (UGreeislandDebugHudWidget* Widget = GetDebugHudWidget())
    {
        const TArray<FGreeislandCardViewData>& Hand = Widget->GetHandCardViewData();
        if (Hand.IsValidIndex(SlotIndex))
        {
            Widget->PlayCombatCardById(Hand[SlotIndex].CardId);
        }
    }
}
