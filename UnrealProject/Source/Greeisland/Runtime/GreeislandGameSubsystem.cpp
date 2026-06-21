#include "Runtime/GreeislandGameSubsystem.h"

#include "Cards/CardDefinitionLibrary.h"
#include "Exploration/ExplorationEventLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Rules/RuleResolver.h"
#include "Save/SaveGameMapper.h"
#include "Session/GameSessionLibrary.h"

FSessionActionResult UGreeislandGameSubsystem::InitializeNewSessionFromFiles(
    const FString& CardJsonPath,
    const FString& EventJsonPath)
{
    TArray<FCardDefinition> LoadedCards;
    TArray<FString> Errors;
    if (!UCardDefinitionLibrary::LoadCardsFromJsonFile(CardJsonPath, LoadedCards, Errors))
    {
        FSessionActionResult Result;
        Result.Reasons = Errors;
        return Result;
    }

    FZoneEventSet EventSet;
    Errors.Reset();
    if (!UExplorationEventLibrary::LoadZoneEventsFromJsonFile(EventJsonPath, EventSet, Errors))
    {
        FSessionActionResult Result;
        Result.Reasons = Errors;
        return Result;
    }

    FSessionActionResult Result =
        UGameSessionLibrary::InitializeSession(LoadedCards, EventSet, Session);
    bHasInitializedSession = Result.bSuccess;
    if (Result.bSuccess)
    {
        RuntimeLogLines.Reset();
    }
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::RestoreSessionFromSaveSlot(
    const FString& SlotName,
    int32 UserIndex,
    const FString& CardJsonPath,
    const FString& EventJsonPath)
{
    TArray<FCardDefinition> LoadedCards;
    TArray<FString> Errors;
    if (!UCardDefinitionLibrary::LoadCardsFromJsonFile(CardJsonPath, LoadedCards, Errors))
    {
        FSessionActionResult Result;
        Result.Reasons = Errors;
        return Result;
    }

    FZoneEventSet EventSet;
    Errors.Reset();
    if (!UExplorationEventLibrary::LoadZoneEventsFromJsonFile(EventJsonPath, EventSet, Errors))
    {
        FSessionActionResult Result;
        Result.Reasons = Errors;
        return Result;
    }

    USaveGame* RawSave = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
    UGreeislandSaveGame* SaveGame = Cast<UGreeislandSaveGame>(RawSave);
    if (!SaveGame)
    {
        return FailResult(FString::Printf(TEXT("No Greeisland save found in slot %s."), *SlotName));
    }

    FSessionActionResult Result =
        USaveGameMapper::ApplySaveGameToSession(SaveGame, LoadedCards, EventSet, Session);
    bHasInitializedSession = Result.bSuccess;
    if (Result.bSuccess)
    {
        RuntimeLogLines.Reset();
    }
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::SaveSessionToSlot(
    const FString& SlotName,
    int32 UserIndex,
    const FString& PlayerId)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    UGreeislandSaveGame* SaveGame = Cast<UGreeislandSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UGreeislandSaveGame::StaticClass()));
    if (!SaveGame)
    {
        return FailResult(TEXT("Failed to create save game object."));
    }

    SaveGame->PlayerId = PlayerId;
    USaveGameMapper::CopySessionToSaveGame(Session, SaveGame);

    FSessionActionResult Result;
    Result.bSuccess = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
    if (!Result.bSuccess)
    {
        Result.Reasons.Add(FString::Printf(TEXT("Failed to save slot %s."), *SlotName));
        AppendRuntimeLogs(Result);
        return Result;
    }

    Result.LogLines.Add(FString::Printf(TEXT("Saved session to slot %s."), *SlotName));
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::ResolveEvent(FName EventId)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FSessionActionResult Result = UGameSessionLibrary::ResolveEventInSession(Session, EventId);
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::ResolveOrStartEvent(FName EventId, int32 OpeningDrawCount)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FExplorationEventDefinition Event;
    if (!GetEventDefinition(EventId, Event))
    {
        return FailResult(FString::Printf(TEXT("Unknown event %s."), *EventId.ToString()));
    }

    FSessionActionResult Result = (Event.Type == EExplorationEventType::Battle)
        ? UGameSessionLibrary::StartCombatForEvent(Session, EventId, OpeningDrawCount)
        : UGameSessionLibrary::ResolveEventInSession(Session, EventId);
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::StartCombat(FName EventId, int32 OpeningDrawCount)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FSessionActionResult Result = UGameSessionLibrary::StartCombatForEvent(Session, EventId, OpeningDrawCount);
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::PlayCombatCard(FName CardId)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FSessionActionResult Result = UGameSessionLibrary::PlayCardInSessionCombat(Session, CardId);
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::RunEnemyTurn(int32 DrawCount)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FSessionActionResult Result = UGameSessionLibrary::RunEnemyTurnInSessionCombat(Session, DrawCount);
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::ApplyAiResponse(
    const FAiGmResponse& Response,
    const FString& PlayerChoice)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FSessionActionResult Result = UGameSessionLibrary::ApplyAiResponseToSession(Session, Response, PlayerChoice);
    AppendRuntimeLogs(Result);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::GrantDeveloperCard(FName CardId, bool bAddToDeck)
{
    FSessionActionResult Guard = EnsureInitialized();
    if (!Guard.bSuccess)
    {
        return Guard;
    }

    FSessionActionResult Result = UGameSessionLibrary::GrantCardToSession(Session, CardId, bAddToDeck);
    AppendRuntimeLogs(Result);
    return Result;
}

bool UGreeislandGameSubsystem::BuildAiRequest(
    FName EventId,
    const FString& PlayerChoice,
    FAiGmRequest& OutRequest) const
{
    if (!bHasInitializedSession)
    {
        return false;
    }

    return UGameSessionLibrary::BuildAiRequestForEvent(Session, EventId, PlayerChoice, OutRequest);
}

FGreeislandUiSnapshot UGreeislandGameSubsystem::BuildUiSnapshot(int32 MaxLogLines) const
{
    FGreeislandUiSnapshot Snapshot;
    Snapshot.bHasInitializedSession = bHasInitializedSession;
    if (!bHasInitializedSession)
    {
        return Snapshot;
    }

    Snapshot.bCombatActive = Session.bCombatActive;
    Snapshot.bZoneCleared = Session.bZoneCleared;
    Snapshot.ZoneId = Session.ZoneProgress.ZoneId;
    Snapshot.ActiveEventId = Session.ActiveEventId;
    Snapshot.AvailableEventIds = Session.ZoneProgress.AvailableEventIds;
    Snapshot.ActiveQuestIds = Session.ActiveQuestIds;
    Snapshot.CompletedQuestIds = Session.CompletedQuestIds;
    Snapshot.OwnedCardIds = Session.OwnedCardIds;
    Snapshot.DeckCardIds = Session.DeckCardIds;
    Snapshot.HandCardIds = Session.CombatState.Hand;
    Snapshot.PlayerHp = Session.CombatState.Player.CurrentHp;
    Snapshot.EnemyHp = Session.CombatState.Enemy.CurrentHp;
    Snapshot.Energy = Session.CombatState.Energy;
    FindEventDisplayName(Session.ActiveEventId, Snapshot.ActiveEventDisplayName);

    const int32 SafeMaxLines = FMath::Max(0, MaxLogLines);
    const int32 StartIndex = FMath::Max(0, RuntimeLogLines.Num() - SafeMaxLines);
    for (int32 Index = StartIndex; Index < RuntimeLogLines.Num(); ++Index)
    {
        Snapshot.RecentLogLines.Add(RuntimeLogLines[Index]);
    }

    return Snapshot;
}

void UGreeislandGameSubsystem::GetPlayableCombatCardIds(TArray<FName>& OutCardIds) const
{
    OutCardIds.Reset();
    if (!bHasInitializedSession || !Session.bCombatActive)
    {
        return;
    }

    FCardPlayContext Context;
    Context.CurrentPhase = EGamePhase::Combat;
    Context.EnergyAvailable = Session.CombatState.Energy;
    Context.BasePartySize = 1;
    Context.HandCount = Session.CombatState.Hand.Num();

    for (const FName& OwnedCardId : Session.OwnedCardIds)
    {
        for (const FCardDefinition& Card : Session.KnownCards)
        {
            if (Card.CardId != OwnedCardId)
            {
                continue;
            }

            Context.CollectionTags.Append(Card.Tags);
            break;
        }
    }

    for (const FName& CardId : Session.CombatState.Hand)
    {
        for (const FCardDefinition& Card : Session.KnownCards)
        {
            if (Card.CardId != CardId)
            {
                continue;
            }

            const FCardPlayResult Result = URuleResolver::CanPlayCard(Card, Context);
            if (Result.bCanPlay)
            {
                OutCardIds.Add(CardId);
            }
            break;
        }
    }
}

void UGreeislandGameSubsystem::BuildOwnedCardViewData(TArray<FGreeislandCardViewData>& OutCards) const
{
    OutCards.Reset();
    if (!bHasInitializedSession)
    {
        return;
    }

    TArray<FName> PlayableCardIds;
    GetPlayableCombatCardIds(PlayableCardIds);

    for (const FName& CardId : Session.OwnedCardIds)
    {
        FCardDefinition Card;
        if (!FindKnownCard(CardId, Card))
        {
            continue;
        }

        FGreeislandCardViewData ViewData;
        ViewData.CardId = Card.CardId;
        ViewData.DisplayName = Card.DisplayName;
        ViewData.Kind = Card.Kind;
        ViewData.Rarity = Card.Rarity;
        ViewData.bOwned = true;
        ViewData.bInDeck = Session.DeckCardIds.Contains(CardId);
        ViewData.bInHand = Session.CombatState.Hand.Contains(CardId);
        ViewData.bPlayableNow = PlayableCardIds.Contains(CardId);
        OutCards.Add(ViewData);
    }
}

void UGreeislandGameSubsystem::BuildHandCardViewData(TArray<FGreeislandCardViewData>& OutCards) const
{
    OutCards.Reset();
    if (!bHasInitializedSession)
    {
        return;
    }

    TArray<FName> PlayableCardIds;
    GetPlayableCombatCardIds(PlayableCardIds);

    for (const FName& CardId : Session.CombatState.Hand)
    {
        FCardDefinition Card;
        if (!FindKnownCard(CardId, Card))
        {
            continue;
        }

        FGreeislandCardViewData ViewData;
        ViewData.CardId = Card.CardId;
        ViewData.DisplayName = Card.DisplayName;
        ViewData.Kind = Card.Kind;
        ViewData.Rarity = Card.Rarity;
        ViewData.bOwned = Session.OwnedCardIds.Contains(CardId);
        ViewData.bInDeck = Session.DeckCardIds.Contains(CardId);
        ViewData.bInHand = true;
        ViewData.bPlayableNow = PlayableCardIds.Contains(CardId);
        OutCards.Add(ViewData);
    }
}

void UGreeislandGameSubsystem::BuildEventViewData(TArray<FGreeislandEventViewData>& OutEvents) const
{
    OutEvents.Reset();
    if (!bHasInitializedSession)
    {
        return;
    }

    for (const FExplorationEventDefinition& Event : Session.ZoneEventSet.Events)
    {
        FGreeislandEventViewData ViewData;
        ViewData.EventId = Event.EventId;
        ViewData.DisplayName = Event.DisplayName;
        ViewData.Type = Event.Type;
        ViewData.bAvailable = Session.ZoneProgress.AvailableEventIds.Contains(Event.EventId);
        ViewData.bCompleted = Session.ZoneProgress.CompletedEventIds.Contains(Event.EventId);
        ViewData.bIsActive = Session.ActiveEventId == Event.EventId;
        OutEvents.Add(ViewData);
    }
}

bool UGreeislandGameSubsystem::GetEventDefinition(FName EventId, FExplorationEventDefinition& OutEvent) const
{
    if (!bHasInitializedSession)
    {
        return false;
    }

    for (const FExplorationEventDefinition& Event : Session.ZoneEventSet.Events)
    {
        if (Event.EventId == EventId)
        {
            OutEvent = Event;
            return true;
        }
    }

    return false;
}

FSessionActionResult UGreeislandGameSubsystem::FailResult(const FString& Message) const
{
    FSessionActionResult Result;
    Result.Reasons.Add(Message);
    return Result;
}

FSessionActionResult UGreeislandGameSubsystem::EnsureInitialized() const
{
    if (bHasInitializedSession)
    {
        FSessionActionResult Result;
        Result.bSuccess = true;
        return Result;
    }

    return FailResult(TEXT("Session is not initialized."));
}

void UGreeislandGameSubsystem::AppendRuntimeLogs(const FSessionActionResult& ActionResult)
{
    for (const FString& LogLine : ActionResult.LogLines)
    {
        RuntimeLogLines.Add(LogLine);
    }
    for (const FString& Reason : ActionResult.Reasons)
    {
        RuntimeLogLines.Add(FString::Printf(TEXT("Error: %s"), *Reason));
    }

    constexpr int32 MaxRuntimeLogLines = 80;
    while (RuntimeLogLines.Num() > MaxRuntimeLogLines)
    {
        RuntimeLogLines.RemoveAt(0);
    }
}

bool UGreeislandGameSubsystem::FindEventDisplayName(FName EventId, FText& OutDisplayName) const
{
    if (!bHasInitializedSession)
    {
        return false;
    }

    for (const FExplorationEventDefinition& Event : Session.ZoneEventSet.Events)
    {
        if (Event.EventId == EventId)
        {
            OutDisplayName = Event.DisplayName;
            return true;
        }
    }

    return false;
}

bool UGreeislandGameSubsystem::FindKnownCard(FName CardId, FCardDefinition& OutCard) const
{
    if (!bHasInitializedSession)
    {
        return false;
    }

    for (const FCardDefinition& Card : Session.KnownCards)
    {
        if (Card.CardId == CardId)
        {
            OutCard = Card;
            return true;
        }
    }

    return false;
}
