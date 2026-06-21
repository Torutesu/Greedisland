#include "Exploration/ExplorationEngine.h"

namespace
{
bool ContainsAllRequiredCards(
    const TArray<FName>& OwnedCardIds,
    const TArray<FName>& RequiredCardIds,
    TArray<FString>& OutReasons)
{
    bool bHasAll = true;
    for (const FName& RequiredCardId : RequiredCardIds)
    {
        if (!OwnedCardIds.Contains(RequiredCardId))
        {
            OutReasons.Add(FString::Printf(
                TEXT("Missing required card %s."),
                *RequiredCardId.ToString()));
            bHasAll = false;
        }
    }

    return bHasAll;
}

void AddUniqueCard(
    TArray<FName>& OwnedCardIds,
    FName CardId,
    FExplorationResolveResult& Result)
{
    if (CardId.IsNone())
    {
        return;
    }

    if (!OwnedCardIds.Contains(CardId))
    {
        OwnedCardIds.Add(CardId);
        Result.GrantedCardIds.Add(CardId);
        Result.LogLines.Add(FString::Printf(TEXT("Granted card %s."), *CardId.ToString()));
    }
    else
    {
        Result.LogLines.Add(FString::Printf(TEXT("Card %s already owned."), *CardId.ToString()));
    }
}
}

FExplorationResolveResult UExplorationEngine::CanResolveEvent(
    const FExplorationEventDefinition& Event,
    const TArray<FName>& OwnedCardIds,
    const FZoneProgressState& ZoneProgress)
{
    FExplorationResolveResult Result;

    if (Event.EventId.IsNone())
    {
        Result.Reasons.Add(TEXT("Event id is missing."));
        return Result;
    }

    if (Event.bCompleted || ZoneProgress.CompletedEventIds.Contains(Event.EventId))
    {
        Result.Reasons.Add(FString::Printf(
            TEXT("Event %s is already completed."),
            *Event.EventId.ToString()));
        return Result;
    }

    ContainsAllRequiredCards(OwnedCardIds, Event.RequiredCardIds, Result.Reasons);
    Result.bSuccess = Result.Reasons.Num() == 0;
    return Result;
}

FExplorationResolveResult UExplorationEngine::ResolveEvent(
    const FExplorationEventDefinition& Event,
    TArray<FName>& OwnedCardIds,
    FZoneProgressState& ZoneProgress)
{
    FExplorationResolveResult Result = CanResolveEvent(Event, OwnedCardIds, ZoneProgress);
    if (!Result.bSuccess)
    {
        return Result;
    }

    for (const FName& RewardCardId : Event.RewardCardIds)
    {
        AddUniqueCard(OwnedCardIds, RewardCardId, Result);
        if (RewardCardId.ToString().StartsWith(TEXT("key_")) &&
            !ZoneProgress.AcquiredKeyCardIds.Contains(RewardCardId))
        {
            ZoneProgress.AcquiredKeyCardIds.Add(RewardCardId);
            Result.LogLines.Add(FString::Printf(TEXT("Recorded key card %s."), *RewardCardId.ToString()));
        }
    }

    ZoneProgress.CompletedEventIds.AddUnique(Event.EventId);
    Result.LogLines.Add(FString::Printf(TEXT("Completed event %s."), *Event.EventId.ToString()));
    Result.bSuccess = true;
    return Result;
}

