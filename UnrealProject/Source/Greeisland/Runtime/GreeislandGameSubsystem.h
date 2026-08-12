#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AiGm/AiGmTypes.h"
#include "AiGm/AiGmValidator.h"
#include "Save/GreeislandSaveGame.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandGameSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FGreeislandUiSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bHasInitializedSession = false;

    UPROPERTY(BlueprintReadOnly)
    bool bCombatActive = false;

    UPROPERTY(BlueprintReadOnly)
    bool bZoneCleared = false;

    UPROPERTY(BlueprintReadOnly)
    FName ZoneId;

    UPROPERTY(BlueprintReadOnly)
    FName ActiveEventId;

    UPROPERTY(BlueprintReadOnly)
    FText ActiveEventDisplayName;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> AvailableEventIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> ActiveQuestIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> CompletedQuestIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> OwnedCardIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> DeckCardIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> HandCardIds;

    UPROPERTY(BlueprintReadOnly)
    int32 PlayerHp = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 EnemyHp = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Energy = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> RecentLogLines;
};

USTRUCT(BlueprintType)
struct FGreeislandCardViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName CardId;

    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly)
    ECardKind Kind = ECardKind::Action;

    UPROPERTY(BlueprintReadOnly)
    ECardRarity Rarity = ECardRarity::Common;

    UPROPERTY(BlueprintReadOnly)
    bool bOwned = false;

    UPROPERTY(BlueprintReadOnly)
    bool bInDeck = false;

    UPROPERTY(BlueprintReadOnly)
    bool bInHand = false;

    UPROPERTY(BlueprintReadOnly)
    bool bPlayableNow = false;

    UPROPERTY(BlueprintReadOnly)
    int32 EffectivePartySize = 1;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> UnplayableReasons;

    UPROPERTY(BlueprintReadOnly)
    FString KindLabel;

    UPROPERTY(BlueprintReadOnly)
    FString StateSummary;

    UPROPERTY(BlueprintReadOnly)
    FString DetailSummary;

    UPROPERTY(BlueprintReadOnly)
    FString PrimaryActionId;

    UPROPERTY(BlueprintReadOnly)
    FString PrimaryActionLabel;

    UPROPERTY(BlueprintReadOnly)
    FName PrimaryActionNameArgument;

    UPROPERTY(BlueprintReadOnly)
    bool bHasPrimaryAction = false;

    UPROPERTY(BlueprintReadOnly)
    bool bPrimaryActionEnabled = false;
};

USTRUCT(BlueprintType)
struct FGreeislandEventViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName EventId;

    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly)
    EExplorationEventType Type = EExplorationEventType::Treasure;

    UPROPERTY(BlueprintReadOnly)
    bool bAvailable = false;

    UPROPERTY(BlueprintReadOnly)
    bool bCompleted = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsActive = false;

    UPROPERTY(BlueprintReadOnly)
    FString TypeLabel;

    UPROPERTY(BlueprintReadOnly)
    FString StatusSummary;

    UPROPERTY(BlueprintReadOnly)
    FString DetailSummary;

    UPROPERTY(BlueprintReadOnly)
    FString PrimaryActionId;

    UPROPERTY(BlueprintReadOnly)
    FString PrimaryActionLabel;

    UPROPERTY(BlueprintReadOnly)
    FName PrimaryActionNameArgument;

    UPROPERTY(BlueprintReadOnly)
    bool bHasPrimaryAction = false;

    UPROPERTY(BlueprintReadOnly)
    bool bPrimaryActionEnabled = false;
};

UCLASS()
class GREEISLAND_API UGreeislandGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult InitializeNewSessionFromFiles(
        const FString& CardJsonPath,
        const FString& EventJsonPath);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult RestoreSessionFromSaveSlot(
        const FString& SlotName,
        int32 UserIndex,
        const FString& CardJsonPath,
        const FString& EventJsonPath);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult SaveSessionToSlot(
        const FString& SlotName,
        int32 UserIndex,
        const FString& PlayerId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult ResolveEvent(FName EventId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult ResolveOrStartEvent(FName EventId, int32 OpeningDrawCount = 5);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult StartCombat(FName EventId, int32 OpeningDrawCount);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult PlayCombatCard(FName CardId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult RunEnemyTurn(int32 DrawCount);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult ApplyAiResponse(
        const FAiGmResponse& Response,
        const FString& PlayerChoice);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    FSessionActionResult GrantDeveloperCard(FName CardId, bool bAddToDeck = true);

    UFUNCTION(BlueprintPure, Category = "Greeisland|Runtime")
    FAiGmValidationResult ValidateAiResponseForActiveEvent(
        const FAiGmResponse& Response,
        const FString& PlayerChoice) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    bool BuildFallbackAiResponseForActiveEvent(
        const FString& PlayerChoice,
        FAiGmResponse& OutResponse) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    bool BuildAiRequest(
        FName EventId,
        const FString& PlayerChoice,
        FAiGmRequest& OutRequest) const;

    UFUNCTION(BlueprintPure, Category = "Greeisland|Runtime")
    FGreeislandUiSnapshot BuildUiSnapshot(int32 MaxLogLines = 12) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    void GetPlayableCombatCardIds(TArray<FName>& OutCardIds) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    void BuildOwnedCardViewData(TArray<FGreeislandCardViewData>& OutCards) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    void BuildHandCardViewData(TArray<FGreeislandCardViewData>& OutCards) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    bool GetCombatCardPlayResult(FName CardId, FCardPlayResult& OutResult) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    void BuildEventViewData(TArray<FGreeislandEventViewData>& OutEvents) const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Runtime")
    bool GetEventDefinition(FName EventId, FExplorationEventDefinition& OutEvent) const;

    UFUNCTION(BlueprintPure, Category = "Greeisland|Runtime")
    FGreeislandGameSession GetSession() const
    {
        return Session;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|Runtime")
    bool HasInitializedSession() const
    {
        return bHasInitializedSession;
    }

private:
    bool BuildCombatPlayContext(FCardPlayContext& OutContext) const;
    FSessionActionResult FailResult(const FString& Message) const;
    FSessionActionResult EnsureInitialized() const;
    void AppendRuntimeLogs(const FSessionActionResult& ActionResult);
    bool FindEventDisplayName(FName EventId, FText& OutDisplayName) const;
    bool FindKnownCard(FName CardId, FCardDefinition& OutCard) const;

    UPROPERTY()
    FGreeislandGameSession Session;

    UPROPERTY()
    bool bHasInitializedSession = false;

    UPROPERTY()
    TArray<FString> RuntimeLogLines;
};
