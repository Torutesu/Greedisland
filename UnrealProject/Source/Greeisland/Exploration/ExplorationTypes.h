#pragma once

#include "CoreMinimal.h"
#include "ExplorationTypes.generated.h"

UENUM(BlueprintType)
enum class EExplorationEventType : uint8
{
    Battle,
    Treasure,
    Npc,
    Quest,
    KeyGate
};

USTRUCT(BlueprintType)
struct FExplorationEventDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName EventId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EExplorationEventType Type = EExplorationEventType::Treasure;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Description;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> RewardCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> RequiredCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> AllowedAiRewardCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> NextEventIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName NpcId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName EnemyId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 EnemyHp = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 EnemyAttack = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bCompleted = false;
};

USTRUCT(BlueprintType)
struct FZoneProgressState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName ZoneId = TEXT("zone_mvp_island_001");

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> CompletedEventIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> AcquiredKeyCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> AvailableEventIds;
};

USTRUCT(BlueprintType)
struct FExplorationResolveResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Reasons;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> LogLines;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> GrantedCardIds;
};
