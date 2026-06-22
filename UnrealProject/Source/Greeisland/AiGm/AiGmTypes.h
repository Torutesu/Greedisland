#pragma once

#include "CoreMinimal.h"
#include "AiGmTypes.generated.h"

USTRUCT(BlueprintType)
struct FAiGmRequest
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName ZoneId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName EventId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName NpcId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> PlayerCollectionTags;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> AllowedRewardCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> AllowedQuestEventIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString PlayerChoice;
};

UENUM(BlueprintType)
enum class EAiGmIntent : uint8
{
    Flavor,
    QuestOffer,
    Negotiation,
    Reward,
    Refusal
};

USTRUCT(BlueprintType)
struct FAiGmResponse
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString SpeakerName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Dialogue;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EAiGmIntent Intent = EAiGmIntent::Flavor;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName ProposedQuestId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FName> AllowedRewardCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName DifficultyHint = TEXT("medium");
};
