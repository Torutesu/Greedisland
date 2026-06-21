#include "Runtime/GreeislandGameSubsystem.h"

#include "Cards/CardDefinitionLibrary.h"
#include "Exploration/ExplorationEventLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Rules/RuleResolver.h"
#include "Save/SaveGameMapper.h"
#include "Session/GameSessionLibrary.h"

namespace
{
FString CardKindToLabel(ECardKind Kind)
{
    switch (Kind)
    {
        case ECardKind::Action:
            return TEXT("Action");
        case ECardKind::Item:
            return TEXT("Item");
        case ECardKind::Rule:
            return TEXT("Rule");
        case ECardKind::Constraint:
            return TEXT("Constraint");
        case ECardKind::Key:
            return TEXT("Key");
    }

    return TEXT("Unknown");
}

FString EventTypeToLabel(EExplorationEventType Type)
{
    switch (Type)
    {
        case EExplorationEventType::Battle:
            return TEXT("Battle");
        case EExplorationEventType::Treasure:
            return TEXT("Treasure");
        case EExplorationEventType::Npc:
            return TEXT("Npc");
        case EExplorationEventType::Quest:
            return TEXT("Quest");
        case EExplorationEventType::KeyGate:
            return TEXT("KeyGate");
    }

    return TEXT("Unknown");
}
}

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

FAiGmValidationResult UGreeislandGameSubsystem::ValidateAiResponseForActiveEvent(
    const FAiGmResponse& Response,
    const FString& PlayerChoice) const
{
    FAiGmValidationResult Result;

    if (!bHasInitializedSession)
    {
        Result.Reasons.Add(TEXT("Session is not initialized."));
        return Result;
    }

    FAiGmRequest Request;
    if (!BuildAiRequest(Session.ActiveEventId, PlayerChoice, Request))
    {
        Result.Reasons.Add(TEXT("Could not build AI request for the active event."));
        return Result;
    }

    return UAiGmValidator::ValidateResponse(Response, Request, Session.KnownCards);
}

bool UGreeislandGameSubsystem::BuildFallbackAiResponseForActiveEvent(
    const FString& PlayerChoice,
    FAiGmResponse& OutResponse) const
{
    OutResponse = FAiGmResponse();

    if (!bHasInitializedSession)
    {
        return false;
    }

    FExplorationEventDefinition Event;
    if (!GetEventDefinition(Session.ActiveEventId, Event))
    {
        return false;
    }

    FAiGmRequest Request;
    if (!BuildAiRequest(Session.ActiveEventId, PlayerChoice, Request))
    {
        return false;
    }

    OutResponse.SpeakerName = Event.NpcId.IsNone()
        ? Event.DisplayName
        : Event.NpcId.ToString().Replace(TEXT("npc_"), TEXT("NPC "));

    if (Request.AllowedRewardCardIds.Num() > 0)
    {
        OutResponse.Intent = EAiGmIntent::Reward;
        OutResponse.AllowedRewardCardIds = { Request.AllowedRewardCardIds[0] };
        OutResponse.Dialogue = FString::Printf(
            TEXT("%sを確認した。今回は定型手続きとして報酬を一つだけ渡す。"),
            PlayerChoice.TrimStartAndEnd().IsEmpty() ? TEXT("申し出") : *PlayerChoice);
        OutResponse.DifficultyHint = TEXT("low");
    }
    else
    {
        OutResponse.Intent = EAiGmIntent::Flavor;
        OutResponse.Dialogue = FString::Printf(
            TEXT("%sでの反応は記録した。いまは状況説明だけを返す。"),
            *Event.DisplayName);
        OutResponse.DifficultyHint = TEXT("medium");
    }

    return true;
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
    FCardPlayContext Context;
    if (!BuildCombatPlayContext(Context))
    {
        return;
    }

    for (const FName& CardId : Session.CombatState.Hand)
    {
        FCardDefinition Card;
        if (!FindKnownCard(CardId, Card))
        {
            continue;
        }

        const FCardPlayResult Result = URuleResolver::CanPlayCard(Card, Context);
        if (Result.bCanPlay)
        {
            OutCardIds.Add(CardId);
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
        ViewData.KindLabel = CardKindToLabel(Card.Kind);

        if (ViewData.bInHand)
        {
            FCardPlayResult PlayResult;
            if (GetCombatCardPlayResult(CardId, PlayResult))
            {
                ViewData.EffectivePartySize = PlayResult.EffectivePartySize;
                ViewData.UnplayableReasons = PlayResult.Reasons;
            }
        }

        TArray<FString> StateParts;
        StateParts.Add(ViewData.KindLabel);
        if (ViewData.bInHand)
        {
            StateParts.Add(ViewData.bPlayableNow ? TEXT("Playable") : TEXT("Blocked"));
        }
        else if (ViewData.bInDeck)
        {
            StateParts.Add(TEXT("In Deck"));
        }
        else
        {
            StateParts.Add(TEXT("Owned"));
        }
        ViewData.StateSummary = FString::Join(StateParts, TEXT(" | "));

        TArray<FString> DetailParts;
        if (ViewData.bInHand)
        {
            DetailParts.Add(FString::Printf(TEXT("party %d"), ViewData.EffectivePartySize));
        }
        if (ViewData.UnplayableReasons.Num() > 0)
        {
            DetailParts.Add(ViewData.UnplayableReasons[0]);
        }
        ViewData.DetailSummary = FString::Join(DetailParts, TEXT(" | "));

        if (ViewData.bInHand)
        {
            ViewData.PrimaryActionId = TEXT("play_combat_card");
            ViewData.PrimaryActionLabel = TEXT("Play Card");
            ViewData.PrimaryActionNameArgument = ViewData.CardId;
            ViewData.bHasPrimaryAction = true;
            ViewData.bPrimaryActionEnabled = ViewData.bPlayableNow;
        }

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
        ViewData.KindLabel = CardKindToLabel(Card.Kind);
        FCardPlayResult PlayResult;
        if (GetCombatCardPlayResult(CardId, PlayResult))
        {
            ViewData.EffectivePartySize = PlayResult.EffectivePartySize;
            ViewData.UnplayableReasons = PlayResult.Reasons;
        }
        ViewData.StateSummary = FString::Printf(
            TEXT("%s | %s"),
            *ViewData.KindLabel,
            ViewData.bPlayableNow ? TEXT("Playable") : TEXT("Blocked"));
        TArray<FString> DetailParts;
        DetailParts.Add(FString::Printf(TEXT("party %d"), ViewData.EffectivePartySize));
        if (ViewData.UnplayableReasons.Num() > 0)
        {
            DetailParts.Add(ViewData.UnplayableReasons[0]);
        }
        ViewData.DetailSummary = FString::Join(DetailParts, TEXT(" | "));
        ViewData.PrimaryActionId = TEXT("play_combat_card");
        ViewData.PrimaryActionLabel = TEXT("Play Card");
        ViewData.PrimaryActionNameArgument = ViewData.CardId;
        ViewData.bHasPrimaryAction = true;
        ViewData.bPrimaryActionEnabled = ViewData.bPlayableNow;
        OutCards.Add(ViewData);
    }
}

bool UGreeislandGameSubsystem::GetCombatCardPlayResult(FName CardId, FCardPlayResult& OutResult) const
{
    OutResult = FCardPlayResult();

    FCardPlayContext Context;
    if (!BuildCombatPlayContext(Context))
    {
        return false;
    }

    FCardDefinition Card;
    if (!FindKnownCard(CardId, Card))
    {
        return false;
    }

    OutResult = URuleResolver::CanPlayCard(Card, Context);
    return true;
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
        ViewData.TypeLabel = EventTypeToLabel(Event.Type);
        ViewData.StatusSummary = ViewData.bCompleted
            ? TEXT("Completed")
            : (ViewData.bAvailable ? (ViewData.bIsActive ? TEXT("Available | Active") : TEXT("Available")) : TEXT("Locked"));

        TArray<FString> DetailParts;
        if (Event.RequiredCardIds.Num() > 0)
        {
            DetailParts.Add(FString::Printf(TEXT("needs %d"), Event.RequiredCardIds.Num()));
        }
        if (Event.RewardCardIds.Num() > 0)
        {
            DetailParts.Add(FString::Printf(TEXT("rewards %d"), Event.RewardCardIds.Num()));
        }
        ViewData.DetailSummary = FString::Join(DetailParts, TEXT(" | "));

        if (ViewData.bAvailable && ViewData.bIsActive)
        {
            ViewData.bHasPrimaryAction = true;
            if (Event.Type == EExplorationEventType::Battle)
            {
                ViewData.PrimaryActionId = TEXT("start_active_combat");
                ViewData.PrimaryActionLabel = TEXT("Start Combat");
                ViewData.bPrimaryActionEnabled = !Session.bCombatActive;
            }
            else
            {
                ViewData.PrimaryActionId = TEXT("resolve_active_event");
                ViewData.PrimaryActionLabel = TEXT("Resolve Event");
                ViewData.bPrimaryActionEnabled = true;
            }
            ViewData.PrimaryActionNameArgument = Event.EventId;
        }

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

bool UGreeislandGameSubsystem::BuildCombatPlayContext(FCardPlayContext& OutContext) const
{
    OutContext = FCardPlayContext();

    if (!bHasInitializedSession || !Session.bCombatActive)
    {
        return false;
    }

    OutContext.CurrentPhase = EGamePhase::Combat;
    OutContext.EnergyAvailable = Session.CombatState.Energy;
    OutContext.BasePartySize = 1;
    OutContext.HandCount = Session.CombatState.Hand.Num();

    for (const FName& OwnedCardId : Session.OwnedCardIds)
    {
        FCardDefinition Card;
        if (!FindKnownCard(OwnedCardId, Card))
        {
            continue;
        }

        OutContext.CollectionTags.Append(Card.Tags);
        if (Card.Kind == ECardKind::Rule || Card.Kind == ECardKind::Constraint)
        {
            OutContext.ActiveRuleCards.Add(Card);
        }
    }

    return true;
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
