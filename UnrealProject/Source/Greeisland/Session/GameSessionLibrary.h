#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Session/GameSessionTypes.h"
#include "GameSessionLibrary.generated.h"

UCLASS()
class GREEISLAND_API UGameSessionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult InitializeSession(
        const TArray<FCardDefinition>& KnownCards,
        const FZoneEventSet& ZoneEventSet,
        FGreeislandGameSession& OutSession);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static bool FindKnownCardById(
        const FGreeislandGameSession& Session,
        FName CardId,
        FCardDefinition& OutCard);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult ResolveEventInSession(
        UPARAM(ref) FGreeislandGameSession& Session,
        FName EventId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult StartCombatForEvent(
        UPARAM(ref) FGreeislandGameSession& Session,
        FName EventId,
        int32 OpeningDrawCount);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult PlayCardInSessionCombat(
        UPARAM(ref) FGreeislandGameSession& Session,
        FName CardId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult RunEnemyTurnInSessionCombat(
        UPARAM(ref) FGreeislandGameSession& Session,
        int32 DrawCount);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult ResolveCombatDefeatInSession(
        UPARAM(ref) FGreeislandGameSession& Session);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult ApplyAiResponseToSession(
        UPARAM(ref) FGreeislandGameSession& Session,
        const FAiGmResponse& Response,
        const FString& PlayerChoice);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult GrantCardToSession(
        UPARAM(ref) FGreeislandGameSession& Session,
        FName CardId,
        bool bAddToDeck = true);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static FSessionActionResult RefreshQuestAndClearState(
        UPARAM(ref) FGreeislandGameSession& Session);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Session")
    static bool BuildAiRequestForEvent(
        const FGreeislandGameSession& Session,
        FName EventId,
        const FString& PlayerChoice,
        FAiGmRequest& OutRequest);
};
