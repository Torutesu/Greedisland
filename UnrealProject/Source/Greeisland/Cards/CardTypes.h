#pragma once

#include "CoreMinimal.h"
#include "CardTypes.generated.h"

UENUM(BlueprintType)
enum class ECardKind : uint8
{
    Action,
    Item,
    Rule,
    Constraint,
    Key
};

UENUM(BlueprintType)
enum class ECardRarity : uint8
{
    Common,
    Uncommon,
    Rare,
    Epic,
    Legendary,
    Key
};

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Exploration,
    Combat,
    Dialogue,
    Reward
};

UENUM(BlueprintType)
enum class ECardEffectType : uint8
{
    Damage,
    GainBlock,
    Heal,
    DrawCards,
    ModifyRule,
    GainCard,
    AddStatus,
    RemoveStatus,
    ClaimReward,
    PreventFailure
};

UENUM(BlueprintType)
enum class ECardConstraintType : uint8
{
    HasTagInCollection,
    EffectivePartySizeAtLeast,
    HandCountAtMost,
    HasStatus,
    NotHasStatus
};

UENUM(BlueprintType)
enum class ERuleChannel : uint8
{
    DrawCount,
    HandLimit,
    MoveCount,
    EncounterDifficulty,
    TradePrice,
    RewardMultiplier,
    CardPlayablePhase,
    PartyRequirement,
    TargetingRule
};

UENUM(BlueprintType)
enum class ERuleOperation : uint8
{
    AddValue,
    SetValue,
    MultiplyValue,
    AddPhase,
    AddMatchingTagCount,
    SetFlag
};

UENUM(BlueprintType)
enum class ECardDurationType : uint8
{
    Instant,
    Turns,
    Encounter,
    Permanent
};

USTRUCT(BlueprintType)
struct FCardCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Energy = 0;
};

USTRUCT(BlueprintType)
struct FCardDurationSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardDurationType Type = ECardDurationType::Instant;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Value = 0;
};

USTRUCT(BlueprintType)
struct FCardConstraintSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardConstraintType Type = ECardConstraintType::HasTagInCollection;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName Tag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MinCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Value = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName StatusId;
};

USTRUCT(BlueprintType)
struct FCardEffectSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardEffectType Type = ECardEffectType::Damage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Amount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERuleChannel Channel = ERuleChannel::DrawCount;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ERuleOperation Operation = ERuleOperation::AddValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Value = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName TargetTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EGamePhase Phase = EGamePhase::Exploration;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CardId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName StatusId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FCardDurationSpec Duration;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct FCardDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CardId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardKind Kind = ECardKind::Action;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ECardRarity Rarity = ECardRarity::Common;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> Tags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<EGamePhase> PlayablePhases;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FCardCost Cost;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCardConstraintSpec> Constraints;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FCardEffectSpec> Effects;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText FlavorText;
};

