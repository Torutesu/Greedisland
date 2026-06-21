#include "UI/GreeislandDebugHudWidget.h"

#include "Engine/GameInstance.h"

void UGreeislandDebugHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshPresentation();
}

UGreeislandGameSubsystem* UGreeislandDebugHudWidget::GetGreeislandSubsystem() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UGreeislandGameSubsystem>();
}

void UGreeislandDebugHudWidget::RefreshPresentation()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        CurrentSnapshot = FGreeislandUiSnapshot();
        PlayableCombatCardIds.Reset();
        OwnedCardViewData.Reset();
        HandCardViewData.Reset();
        EventViewData.Reset();
        OnPresentationUpdated();
        return;
    }

    CurrentSnapshot = Subsystem->BuildUiSnapshot(SnapshotLogLineCount);
    Subsystem->GetPlayableCombatCardIds(PlayableCombatCardIds);
    Subsystem->BuildOwnedCardViewData(OwnedCardViewData);
    Subsystem->BuildHandCardViewData(HandCardViewData);
    Subsystem->BuildEventViewData(EventViewData);
    OnPresentationUpdated();
}

FSessionActionResult UGreeislandDebugHudWidget::InitializeNewSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->InitializeNewSessionFromFiles(CardJsonPath, EventJsonPath));
}

FSessionActionResult UGreeislandDebugHudWidget::RestoreSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(
        Subsystem->RestoreSessionFromSaveSlot(
            SaveSlotName,
            SaveUserIndex,
            CardJsonPath,
            EventJsonPath));
}

FSessionActionResult UGreeislandDebugHudWidget::SaveSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->SaveSessionToSlot(SaveSlotName, SaveUserIndex, DefaultPlayerId));
}

FSessionActionResult UGreeislandDebugHudWidget::ResolveEventById(FName EventId)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->ResolveEvent(EventId));
}

FSessionActionResult UGreeislandDebugHudWidget::ResolveActiveEvent()
{
    return ResolveEventById(CurrentSnapshot.ActiveEventId);
}

FSessionActionResult UGreeislandDebugHudWidget::StartCombatForActiveEvent()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(
        Subsystem->StartCombat(CurrentSnapshot.ActiveEventId, DefaultOpeningDrawCount));
}

FSessionActionResult UGreeislandDebugHudWidget::PlayCombatCardById(FName CardId)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->PlayCombatCard(CardId));
}

FSessionActionResult UGreeislandDebugHudWidget::RunEnemyTurn()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->RunEnemyTurn(1));
}

FSessionActionResult UGreeislandDebugHudWidget::ApplyAiRewardResponse(
    const FString& SpeakerName,
    const FString& Dialogue,
    const TArray<FName>& AllowedRewardCardIds,
    const FString& PlayerChoice,
    EAiGmIntent Intent)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    FAiGmResponse Response;
    Response.SpeakerName = SpeakerName;
    Response.Dialogue = Dialogue;
    Response.Intent = Intent;
    Response.AllowedRewardCardIds = AllowedRewardCardIds;
    Response.DifficultyHint = TEXT("medium");
    return HandleActionResult(Subsystem->ApplyAiResponse(Response, PlayerChoice));
}

FSessionActionResult UGreeislandDebugHudWidget::FailResult(const FString& Message)
{
    FSessionActionResult Result;
    Result.Reasons.Add(Message);
    return Result;
}

FSessionActionResult UGreeislandDebugHudWidget::HandleActionResult(const FSessionActionResult& ActionResult)
{
    LastActionResult = ActionResult;
    RefreshPresentation();
    OnActionResultUpdated(LastActionResult);
    return LastActionResult;
}
