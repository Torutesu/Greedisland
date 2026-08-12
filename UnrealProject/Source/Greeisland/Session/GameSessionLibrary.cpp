#include "Session/GameSessionLibrary.h"

#include "AiGm/AiGmValidator.h"
#include "Combat/CombatEngine.h"
#include "Exploration/ExplorationEngine.h"

namespace
{
constexpr int32 MvpBasePartySize = 3;

void AddUniqueName(TArray<FName>& Names, FName Value)
{
    if (!Value.IsNone())
    {
        Names.AddUnique(Value);
    }
}

TArray<FName> BuildCollectionTags(
    const TArray<FName>& OwnedCardIds,
    const TArray<FCardDefinition>& KnownCards)
{
    TArray<FName> Tags;
    for (const FName& OwnedCardId : OwnedCardIds)
    {
        for (const FCardDefinition& Card : KnownCards)
        {
            if (Card.CardId != OwnedCardId)
            {
                continue;
            }

            for (const FName& Tag : Card.Tags)
            {
                Tags.Add(Tag);
            }
            break;
        }
    }

    return Tags;
}

void AddCardCopiesToDeck(
    FGreeislandGameSession& Session,
    FName CardId)
{
    FCardDefinition Card;
    const int32 Copies = UGameSessionLibrary::FindKnownCardById(Session, CardId, Card)
        ? FMath::Max(1, Card.DeckCopies)
        : 1;

    int32 ExistingCopies = 0;
    for (const FName& DeckCardId : Session.DeckCardIds)
    {
        if (DeckCardId == CardId)
        {
            ++ExistingCopies;
        }
    }

    for (int32 CopyIndex = ExistingCopies; CopyIndex < Copies; ++CopyIndex)
    {
        Session.DeckCardIds.Add(CardId);
    }
}

FSessionActionResult CompleteBattleEvent(
    FGreeislandGameSession& Session,
    const FExplorationEventDefinition& Event)
{
    FSessionActionResult Result;
    FExplorationResolveResult ResolveResult =
        UExplorationEngine::ResolveEvent(Event, Session.OwnedCardIds, Session.ZoneProgress);

    Result.bSuccess = ResolveResult.bSuccess;
    Result.Reasons = ResolveResult.Reasons;
    Result.LogLines = ResolveResult.LogLines;
    if (!Result.bSuccess)
    {
        return Result;
    }

    for (const FName& GrantedCardId : ResolveResult.GrantedCardIds)
    {
        AddCardCopiesToDeck(Session, GrantedCardId);
    }

    for (const FName& NextEventId : Event.NextEventIds)
    {
        AddUniqueName(Session.ZoneProgress.AvailableEventIds, NextEventId);
    }

    Session.bCombatActive = false;
    Session.ActiveEventId = Event.EventId;
    Result.LogLines.Add(FString::Printf(
        TEXT("Battle event %s resolved with %d granted card(s)."),
        *Event.EventId.ToString(),
        ResolveResult.GrantedCardIds.Num()));
    return Result;
}
}

FSessionActionResult UGameSessionLibrary::InitializeSession(
    const TArray<FCardDefinition>& KnownCards,
    const FZoneEventSet& ZoneEventSet,
    FGreeislandGameSession& OutSession)
{
    OutSession = FGreeislandGameSession();

    FSessionActionResult Result;
    if (KnownCards.Num() == 0)
    {
        Result.Reasons.Add(TEXT("KnownCards is empty."));
        return Result;
    }

    if (ZoneEventSet.Events.Num() == 0)
    {
        Result.Reasons.Add(TEXT("ZoneEventSet has no events."));
        return Result;
    }

    OutSession.KnownCards = KnownCards;
    OutSession.ZoneEventSet = ZoneEventSet;
    OutSession.ZoneProgress.ZoneId = ZoneEventSet.ZoneId;

    if (ZoneEventSet.Events.Num() > 0)
    {
        AddUniqueName(OutSession.ZoneProgress.AvailableEventIds, ZoneEventSet.Events[0].EventId);
        OutSession.ActiveEventId = ZoneEventSet.Events[0].EventId;
    }

    Result.bSuccess = true;
    Result.LogLines.Add(FString::Printf(
        TEXT("Initialized session for zone %s with %d events."),
        *ZoneEventSet.ZoneId.ToString(),
        ZoneEventSet.Events.Num()));
    FSessionActionResult Refresh = RefreshQuestAndClearState(OutSession);
    Result.LogLines.Append(Refresh.LogLines);
    return Result;
}

bool UGameSessionLibrary::FindKnownCardById(
    const FGreeislandGameSession& Session,
    FName CardId,
    FCardDefinition& OutCard)
{
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

FSessionActionResult UGameSessionLibrary::ResolveEventInSession(
    FGreeislandGameSession& Session,
    FName EventId)
{
    FSessionActionResult Result;

    FExplorationEventDefinition Event;
    if (!UExplorationEventLibrary::FindEventById(Session.ZoneEventSet, EventId, Event))
    {
        Result.Reasons.Add(FString::Printf(TEXT("Unknown event %s."), *EventId.ToString()));
        return Result;
    }

    if (Event.Type == EExplorationEventType::Battle)
    {
        Result.Reasons.Add(FString::Printf(
            TEXT("Event %s is a battle event. Start combat instead of resolving directly."),
            *EventId.ToString()));
        return Result;
    }

    if (!Session.ZoneProgress.AvailableEventIds.Contains(EventId))
    {
        Result.Reasons.Add(FString::Printf(
            TEXT("Event %s is not available yet."),
            *EventId.ToString()));
        return Result;
    }

    FExplorationResolveResult ResolveResult =
        UExplorationEngine::ResolveEvent(Event, Session.OwnedCardIds, Session.ZoneProgress);
    Result.bSuccess = ResolveResult.bSuccess;
    Result.Reasons = ResolveResult.Reasons;
    Result.LogLines = ResolveResult.LogLines;

    if (!Result.bSuccess)
    {
        return Result;
    }

    for (const FName& GrantedCardId : ResolveResult.GrantedCardIds)
    {
        AddCardCopiesToDeck(Session, GrantedCardId);
    }

    for (const FName& NextEventId : Event.NextEventIds)
    {
        AddUniqueName(Session.ZoneProgress.AvailableEventIds, NextEventId);
    }

    Session.ActiveEventId = EventId;
    Result.LogLines.Add(FString::Printf(
        TEXT("Session now has %d owned card(s)."),
        Session.OwnedCardIds.Num()));
    FSessionActionResult Refresh = RefreshQuestAndClearState(Session);
    Result.LogLines.Append(Refresh.LogLines);
    return Result;
}

FSessionActionResult UGameSessionLibrary::StartCombatForEvent(
    FGreeislandGameSession& Session,
    FName EventId,
    int32 OpeningDrawCount)
{
    FSessionActionResult Result;

    FExplorationEventDefinition Event;
    if (!UExplorationEventLibrary::FindEventById(Session.ZoneEventSet, EventId, Event))
    {
        Result.Reasons.Add(FString::Printf(TEXT("Unknown event %s."), *EventId.ToString()));
        return Result;
    }

    if (Event.Type != EExplorationEventType::Battle)
    {
        Result.Reasons.Add(FString::Printf(TEXT("Event %s is not a battle event."), *EventId.ToString()));
        return Result;
    }

    if (!Session.ZoneProgress.AvailableEventIds.Contains(EventId))
    {
        Result.Reasons.Add(FString::Printf(TEXT("Event %s is not available yet."), *EventId.ToString()));
        return Result;
    }

    Session.CombatState = UCombatEngine::CreateCombatState(
        Session.DeckCardIds,
        Event.EnemyId,
        Event.EnemyHp,
        3);
    UCombatEngine::DrawCards(Session.CombatState, OpeningDrawCount, Result.LogLines);
    Session.bCombatActive = true;
    Session.ActiveEventId = EventId;

    Result.bSuccess = true;
    Result.LogLines.Add(FString::Printf(
        TEXT("Started combat for event %s against %s."),
        *Event.EventId.ToString(),
        *Event.EnemyId.ToString()));
    return Result;
}

FSessionActionResult UGameSessionLibrary::PlayCardInSessionCombat(
    FGreeislandGameSession& Session,
    FName CardId)
{
    FSessionActionResult Result;

    if (!Session.bCombatActive)
    {
        Result.Reasons.Add(TEXT("No combat is active."));
        return Result;
    }

    FCardDefinition Card;
    if (!FindKnownCardById(Session, CardId, Card))
    {
        Result.Reasons.Add(FString::Printf(TEXT("Unknown card %s."), *CardId.ToString()));
        return Result;
    }

    FCardPlayContext Context;
    Context.BasePartySize = MvpBasePartySize;
    Context.CollectionTags = BuildCollectionTags(Session.OwnedCardIds, Session.KnownCards);
    for (const FName& OwnedCardId : Session.OwnedCardIds)
    {
        FCardDefinition OwnedCard;
        if (!FindKnownCardById(Session, OwnedCardId, OwnedCard))
        {
            continue;
        }

        if (OwnedCard.Kind == ECardKind::Rule || OwnedCard.Kind == ECardKind::Constraint)
        {
            Context.ActiveRuleCards.Add(OwnedCard);
        }
    }

    FCombatActionResult CombatResult =
        UCombatEngine::PlayCard(Session.CombatState, Card, Context);
    Result.bSuccess = CombatResult.bSuccess;
    Result.Reasons = CombatResult.Reasons;
    Result.LogLines = CombatResult.LogLines;

    if (!Result.bSuccess)
    {
        return Result;
    }

    if (Session.CombatState.Outcome == ECombatOutcome::PlayerVictory)
    {
        FExplorationEventDefinition Event;
        if (!UExplorationEventLibrary::FindEventById(Session.ZoneEventSet, Session.ActiveEventId, Event))
        {
            Result.Reasons.Add(TEXT("Active combat event could not be found for reward resolution."));
            Result.bSuccess = false;
            return Result;
        }

        FSessionActionResult Completion = CompleteBattleEvent(Session, Event);
        Result.LogLines.Append(Completion.LogLines);
        if (!Completion.bSuccess)
        {
            Result.Reasons = Completion.Reasons;
            Result.bSuccess = false;
            return Result;
        }
        FSessionActionResult Refresh = RefreshQuestAndClearState(Session);
        Result.LogLines.Append(Refresh.LogLines);
    }
    else if (Session.CombatState.Outcome == ECombatOutcome::PlayerDefeat)
    {
        FSessionActionResult DefeatResult = ResolveCombatDefeatInSession(Session);
        Result.LogLines.Append(DefeatResult.LogLines);
        if (!DefeatResult.bSuccess)
        {
            Result.Reasons = DefeatResult.Reasons;
            Result.bSuccess = false;
            return Result;
        }
    }

    return Result;
}

FSessionActionResult UGameSessionLibrary::RunEnemyTurnInSessionCombat(
    FGreeislandGameSession& Session,
    int32 DrawCount)
{
    FSessionActionResult Result;

    if (!Session.bCombatActive)
    {
        Result.Reasons.Add(TEXT("No combat is active."));
        return Result;
    }

    FExplorationEventDefinition Event;
    if (!UExplorationEventLibrary::FindEventById(Session.ZoneEventSet, Session.ActiveEventId, Event))
    {
        Result.Reasons.Add(TEXT("Active combat event could not be found."));
        return Result;
    }

    FCombatActionResult CombatResult =
        UCombatEngine::RunEnemyTurn(Session.CombatState, Event.EnemyAttack, DrawCount);
    Result.bSuccess = CombatResult.bSuccess;
    Result.Reasons = CombatResult.Reasons;
    Result.LogLines = CombatResult.LogLines;

    if (Session.CombatState.Outcome == ECombatOutcome::PlayerDefeat)
    {
        FSessionActionResult DefeatResult = ResolveCombatDefeatInSession(Session);
        Result.LogLines.Append(DefeatResult.LogLines);
        if (!DefeatResult.bSuccess)
        {
            Result.Reasons = DefeatResult.Reasons;
            Result.bSuccess = false;
            return Result;
        }
    }

    return Result;
}

FSessionActionResult UGameSessionLibrary::ResolveCombatDefeatInSession(
    FGreeislandGameSession& Session)
{
    FSessionActionResult Result;

    if (Session.CombatState.Outcome != ECombatOutcome::PlayerDefeat)
    {
        Result.Reasons.Add(TEXT("Combat has not ended in defeat."));
        return Result;
    }

    AddUniqueName(Session.ZoneProgress.AvailableEventIds, Session.ActiveEventId);
    Session.bCombatActive = false;
    Session.CombatState = FCombatState();
    Result.bSuccess = true;
    Result.LogLines.Add(FString::Printf(
        TEXT("Combat defeat resolved. Event %s remains available for retry."),
        *Session.ActiveEventId.ToString()));
    return Result;
}

FSessionActionResult UGameSessionLibrary::ApplyAiResponseToSession(
    FGreeislandGameSession& Session,
    const FAiGmResponse& Response,
    const FString& PlayerChoice)
{
    FSessionActionResult Result;

    FAiGmRequest Request;
    if (!BuildAiRequestForEvent(Session, Session.ActiveEventId, PlayerChoice, Request))
    {
        Result.Reasons.Add(TEXT("Could not build AI request for the active event."));
        return Result;
    }

    const FAiGmValidationResult Validation =
        UAiGmValidator::ValidateResponse(Response, Request, Session.KnownCards);
    if (!Validation.bIsValid)
    {
        Result.Reasons = Validation.Reasons;
        return Result;
    }

    for (const FName& RewardCardId : Response.AllowedRewardCardIds)
    {
        Session.OwnedCardIds.AddUnique(RewardCardId);
        AddCardCopiesToDeck(Session, RewardCardId);
        Result.LogLines.Add(FString::Printf(
            TEXT("AI GM granted reward card %s."),
            *RewardCardId.ToString()));
    }

    if (!Response.ProposedQuestId.IsNone())
    {
        Session.ZoneProgress.AvailableEventIds.AddUnique(Response.ProposedQuestId);
        Result.LogLines.Add(FString::Printf(
            TEXT("AI GM proposed quest event %s."),
            *Response.ProposedQuestId.ToString()));
    }

    Result.bSuccess = true;
    Result.LogLines.Add(FString::Printf(
        TEXT("Applied AI response from %s."),
        *Response.SpeakerName));
    FSessionActionResult Refresh = RefreshQuestAndClearState(Session);
    Result.LogLines.Append(Refresh.LogLines);
    return Result;
}

FSessionActionResult UGameSessionLibrary::GrantCardToSession(
    FGreeislandGameSession& Session,
    FName CardId,
    bool bAddToDeck)
{
    FSessionActionResult Result;

    if (CardId.IsNone())
    {
        Result.Reasons.Add(TEXT("CardId is empty."));
        return Result;
    }

    FCardDefinition Card;
    if (!FindKnownCardById(Session, CardId, Card))
    {
        Result.Reasons.Add(FString::Printf(TEXT("Unknown card %s."), *CardId.ToString()));
        return Result;
    }

    Session.OwnedCardIds.AddUnique(CardId);
    if (bAddToDeck)
    {
        AddCardCopiesToDeck(Session, CardId);
    }

    Result.bSuccess = true;
    Result.LogLines.Add(FString::Printf(
        TEXT("Granted developer card %s%s."),
        *CardId.ToString(),
        bAddToDeck ? TEXT(" and added it to the deck") : TEXT("")));

    FSessionActionResult Refresh = RefreshQuestAndClearState(Session);
    Result.LogLines.Append(Refresh.LogLines);
    return Result;
}

bool UGameSessionLibrary::BuildAiRequestForEvent(
    const FGreeislandGameSession& Session,
    FName EventId,
    const FString& PlayerChoice,
    FAiGmRequest& OutRequest)
{
    FExplorationEventDefinition Event;
    if (!UExplorationEventLibrary::FindEventById(Session.ZoneEventSet, EventId, Event))
    {
        return false;
    }

    OutRequest = FAiGmRequest();
    OutRequest.ZoneId = Session.ZoneEventSet.ZoneId;
    OutRequest.EventId = Event.EventId;
    OutRequest.NpcId = Event.NpcId;
    OutRequest.PlayerCollectionTags = BuildCollectionTags(Session.OwnedCardIds, Session.KnownCards);
    OutRequest.AllowedRewardCardIds = Event.AllowedAiRewardCardIds;
    for (const FName& NextEventId : Event.NextEventIds)
    {
        FExplorationEventDefinition NextEvent;
        if (!UExplorationEventLibrary::FindEventById(Session.ZoneEventSet, NextEventId, NextEvent))
        {
            continue;
        }

        if (NextEvent.Type == EExplorationEventType::Quest)
        {
            OutRequest.AllowedQuestEventIds.AddUnique(NextEventId);
        }
    }
    OutRequest.PlayerChoice = PlayerChoice;
    return true;
}

FSessionActionResult UGameSessionLibrary::RefreshQuestAndClearState(
    FGreeislandGameSession& Session)
{
    FSessionActionResult Result;

    Session.ActiveQuestIds.Reset();
    for (const FExplorationEventDefinition& Event : Session.ZoneEventSet.Events)
    {
        if (Event.Type != EExplorationEventType::Quest)
        {
            continue;
        }

        if (Session.ZoneProgress.CompletedEventIds.Contains(Event.EventId))
        {
            Session.CompletedQuestIds.AddUnique(Event.EventId);
            Session.ActiveQuestIds.Remove(Event.EventId);
            continue;
        }

        if (Session.ZoneProgress.AvailableEventIds.Contains(Event.EventId))
        {
            Session.ActiveQuestIds.AddUnique(Event.EventId);
        }
    }

    bool bHasAllClearCards = true;
    for (const FName& RequiredCardId : Session.ZoneEventSet.ClearRequiredCardIds)
    {
        if (!Session.OwnedCardIds.Contains(RequiredCardId))
        {
            bHasAllClearCards = false;
            break;
        }
    }

    const bool bWasCleared = Session.bZoneCleared;
    Session.bZoneCleared = bHasAllClearCards && Session.ZoneEventSet.ClearRequiredCardIds.Num() > 0;

    Result.bSuccess = true;
    Result.LogLines.Add(FString::Printf(
        TEXT("Quest state refreshed. %d active, %d completed."),
        Session.ActiveQuestIds.Num(),
        Session.CompletedQuestIds.Num()));
    if (Session.bZoneCleared && !bWasCleared)
    {
        Result.LogLines.Add(FString::Printf(
            TEXT("Zone %s is now cleared."),
            *Session.ZoneEventSet.ZoneId.ToString()));
    }
    return Result;
}
