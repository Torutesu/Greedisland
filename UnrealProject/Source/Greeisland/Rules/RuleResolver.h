#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Cards/CardTypes.h"
#include "RuleResolver.generated.h"

USTRUCT(BlueprintType)
struct FRulePatch
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName SourceCardId;

    UPROPERTY(BlueprintReadOnly)
    ERuleChannel Channel = ERuleChannel::DrawCount;

    UPROPERTY(BlueprintReadOnly)
    ERuleOperation Operation = ERuleOperation::AddValue;

    UPROPERTY(BlueprintReadOnly)
    int32 Value = 0;

    UPROPERTY(BlueprintReadOnly)
    FName TargetTag;

    UPROPERTY(BlueprintReadOnly)
    EGamePhase Phase = EGamePhase::Exploration;

    UPROPERTY(BlueprintReadOnly)
    FCardDurationSpec Duration;

    UPROPERTY(BlueprintReadOnly)
    int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct FCardPlayContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    EGamePhase CurrentPhase = EGamePhase::Exploration;

    UPROPERTY(BlueprintReadWrite)
    int32 EnergyAvailable = 0;

    UPROPERTY(BlueprintReadWrite)
    int32 BasePartySize = 1;

    UPROPERTY(BlueprintReadWrite)
    int32 HandCount = 0;

    UPROPERTY(BlueprintReadWrite)
    TArray<FName> CollectionTags;

    UPROPERTY(BlueprintReadWrite)
    TArray<FName> ActiveStatusIds;

    UPROPERTY(BlueprintReadWrite)
    TArray<FCardDefinition> ActiveRuleCards;
};

USTRUCT(BlueprintType)
struct FCardPlayResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bCanPlay = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Reasons;

    UPROPERTY(BlueprintReadOnly)
    TArray<FRulePatch> AppliedPatches;

    UPROPERTY(BlueprintReadOnly)
    int32 EffectivePartySize = 1;
};

UCLASS()
class GREEISLAND_API URuleResolver : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Rules")
    static void GatherRulePatches(
        const FCardPlayContext& Context,
        TArray<FRulePatch>& OutPatches);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Rules")
    static int32 ResolveIntRule(
        ERuleChannel Channel,
        int32 BaseValue,
        const FCardPlayContext& Context,
        TArray<FRulePatch>& OutAppliedPatches);

    UFUNCTION(BlueprintPure, Category = "Greeisland|Rules")
    static FCardPlayResult CanPlayCard(
        const FCardDefinition& Card,
        const FCardPlayContext& Context);
};
