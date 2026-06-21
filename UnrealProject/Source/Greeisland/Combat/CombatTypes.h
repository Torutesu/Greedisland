#pragma once

#include "CoreMinimal.h"
#include "Cards/CardTypes.h"
#include "CombatTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatOutcome : uint8
{
    InProgress,
    PlayerVictory,
    PlayerDefeat
};

USTRUCT(BlueprintType)
struct FCombatantState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName CombatantId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 MaxHp = 20;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 CurrentHp = 20;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Block = 0;
};

USTRUCT(BlueprintType)
struct FCombatState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FCombatantState Player;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FCombatantState Enemy;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 TurnNumber = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Energy = 3;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> DrawPile;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> Hand;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> DiscardPile;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> ClaimedRewardCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ECombatOutcome Outcome = ECombatOutcome::InProgress;
};

USTRUCT(BlueprintType)
struct FCombatActionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Reasons;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> LogLines;

    UPROPERTY(BlueprintReadOnly)
    ECombatOutcome Outcome = ECombatOutcome::InProgress;
};
