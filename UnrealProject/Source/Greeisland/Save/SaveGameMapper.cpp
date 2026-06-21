#include "Save/SaveGameMapper.h"

#include "Session/GameSessionLibrary.h"

void USaveGameMapper::CopySessionToSaveGame(
    const FGreeislandGameSession& Session,
    UGreeislandSaveGame*& SaveGame)
{
    if (!SaveGame)
    {
        return;
    }

    SaveGame->CurrentZoneId = Session.ZoneProgress.ZoneId;
    SaveGame->OwnedCardIds = Session.OwnedCardIds;
    SaveGame->DeckCardIds = Session.DeckCardIds;
    SaveGame->CompletedEventIds = Session.ZoneProgress.CompletedEventIds;
    SaveGame->AcquiredKeyCardIds = Session.ZoneProgress.AcquiredKeyCardIds;
    SaveGame->AvailableEventIds = Session.ZoneProgress.AvailableEventIds;
    SaveGame->ActiveQuestIds = Session.ActiveQuestIds;
    SaveGame->CompletedQuestIds = Session.CompletedQuestIds;
    SaveGame->ActiveEventId = Session.ActiveEventId;
    SaveGame->bZoneCleared = Session.bZoneCleared;
}

FSessionActionResult USaveGameMapper::ApplySaveGameToSession(
    const UGreeislandSaveGame* SaveGame,
    const TArray<FCardDefinition>& KnownCards,
    const FZoneEventSet& ZoneEventSet,
    FGreeislandGameSession& OutSession)
{
    FSessionActionResult Result =
        UGameSessionLibrary::InitializeSession(KnownCards, ZoneEventSet, OutSession);
    if (!Result.bSuccess)
    {
        return Result;
    }

    if (!SaveGame)
    {
        Result.Reasons.Add(TEXT("SaveGame is null."));
        Result.bSuccess = false;
        return Result;
    }

    if (SaveGame->CurrentZoneId != NAME_None &&
        SaveGame->CurrentZoneId != ZoneEventSet.ZoneId)
    {
        Result.Reasons.Add(FString::Printf(
            TEXT("Save zone %s does not match event set zone %s."),
            *SaveGame->CurrentZoneId.ToString(),
            *ZoneEventSet.ZoneId.ToString()));
        Result.bSuccess = false;
        return Result;
    }

    OutSession.OwnedCardIds = SaveGame->OwnedCardIds;
    OutSession.DeckCardIds = SaveGame->DeckCardIds;
    OutSession.ZoneProgress.CompletedEventIds = SaveGame->CompletedEventIds;
    OutSession.ZoneProgress.AcquiredKeyCardIds = SaveGame->AcquiredKeyCardIds;
    OutSession.ZoneProgress.AvailableEventIds = SaveGame->AvailableEventIds;
    OutSession.ActiveQuestIds = SaveGame->ActiveQuestIds;
    OutSession.CompletedQuestIds = SaveGame->CompletedQuestIds;
    OutSession.ActiveEventId = SaveGame->ActiveEventId;
    OutSession.bZoneCleared = SaveGame->bZoneCleared;

    Result.LogLines.Add(FString::Printf(
        TEXT("Restored session with %d owned card(s) and %d completed event(s)."),
        OutSession.OwnedCardIds.Num(),
        OutSession.ZoneProgress.CompletedEventIds.Num()));
    FSessionActionResult Refresh = UGameSessionLibrary::RefreshQuestAndClearState(OutSession);
    Result.LogLines.Append(Refresh.LogLines);
    return Result;
}
