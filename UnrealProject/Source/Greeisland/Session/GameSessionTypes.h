#pragma once

#include "CoreMinimal.h"
#include "AiGm/AiGmTypes.h"
#include "Cards/CardTypes.h"
#include "Combat/CombatTypes.h"
#include "Exploration/ExplorationEventLibrary.h"
#include "Exploration/ExplorationTypes.h"
#include "GameSessionTypes.generated.h"

USTRUCT(BlueprintType)
struct FGreeislandGameSession
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FZoneEventSet ZoneEventSet;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FCardDefinition> KnownCards;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> OwnedCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> DeckCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FZoneProgressState ZoneProgress;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> ActiveQuestIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> CompletedQuestIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bZoneCleared = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bCombatActive = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName ActiveEventId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FCombatState CombatState;
};

USTRUCT(BlueprintType)
struct FSessionActionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Reasons;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> LogLines;
};
