#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Cards/CardTypes.h"
#include "Combat/CombatTypes.h"
#include "Rules/RuleResolver.h"
#include "CombatEngine.generated.h"

UCLASS()
class GREEISLAND_API UCombatEngine : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Combat")
    static FCombatState CreateCombatState(
        const TArray<FName>& DeckCardIds,
        FName EnemyId,
        int32 EnemyHp,
        int32 StartingEnergy);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Combat")
    static void DrawCards(
        UPARAM(ref) FCombatState& State,
        int32 Count,
        TArray<FString>& OutLogLines);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Combat")
    static FCombatActionResult PlayCard(
        UPARAM(ref) FCombatState& State,
        const FCardDefinition& Card,
        const FCardPlayContext& BaseContext);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Combat")
    static FCombatActionResult RunEnemyTurn(
        UPARAM(ref) FCombatState& State,
        int32 EnemyAttackDamage,
        int32 DrawCount);
};

