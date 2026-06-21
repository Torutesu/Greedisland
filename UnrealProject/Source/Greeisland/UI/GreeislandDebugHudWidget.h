#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AiGm/AiGmTypes.h"
#include "Runtime/GreeislandGameSubsystem.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandDebugHudWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class GREEISLAND_API UGreeislandDebugHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    UGreeislandGameSubsystem* GetGreeislandSubsystem() const;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    void RefreshPresentation();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult InitializeNewSession();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult RestoreSession();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult SaveSession();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult ResolveEventById(FName EventId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult ResolveActiveEvent();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult StartCombatForActiveEvent();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult PlayCombatCardById(FName CardId);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult RunEnemyTurn();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult InteractWithFocusedEvent();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult GrantDeveloperCard(FName CardId, bool bAddToDeck = true);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    bool BuildAiRequestForActiveEvent(const FString& PlayerChoice, FAiGmRequest& OutRequest);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FSessionActionResult ApplyAiRewardResponse(
        const FString& SpeakerName,
        const FString& Dialogue,
        const TArray<FName>& AllowedRewardCardIds,
        const FString& PlayerChoice,
        EAiGmIntent Intent = EAiGmIntent::Reward);

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const FGreeislandUiSnapshot& GetCurrentSnapshot() const
    {
        return CurrentSnapshot;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FName>& GetPlayableCombatCardIds() const
    {
        return PlayableCombatCardIds;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandCardViewData>& GetOwnedCardViewData() const
    {
        return OwnedCardViewData;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandCardViewData>& GetHandCardViewData() const
    {
        return HandCardViewData;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandEventViewData>& GetEventViewData() const
    {
        return EventViewData;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const FSessionActionResult& GetLastActionResult() const
    {
        return LastActionResult;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    FName GetFocusedEventId() const
    {
        return FocusedEventId;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    FText GetFocusedEventDisplayName() const
    {
        return FocusedEventDisplayName;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    bool HasFocusedEvent() const
    {
        return bHasFocusedEvent;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const FAiGmRequest& GetLastBuiltAiRequest() const
    {
        return LastBuiltAiRequest;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    bool HasLastBuiltAiRequest() const
    {
        return bHasLastBuiltAiRequest;
    }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    bool bUseProjectSettingsDefaults = true;

    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|UI")
    void OnPresentationUpdated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|UI")
    void OnActionResultUpdated(const FSessionActionResult& ActionResult);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    FString CardJsonPath = TEXT("../../../data/cards/cards.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    FString EventJsonPath = TEXT("../../../data/events/events.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    FString SaveSlotName = TEXT("greeisland-dev-slot");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    int32 SaveUserIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    FString DefaultPlayerId = TEXT("dev-player");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    int32 DefaultOpeningDrawCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    int32 SnapshotLogLineCount = 12;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FGreeislandUiSnapshot CurrentSnapshot;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FName> PlayableCombatCardIds;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandCardViewData> OwnedCardViewData;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandCardViewData> HandCardViewData;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandEventViewData> EventViewData;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FSessionActionResult LastActionResult;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bHasFocusedEvent = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FName FocusedEventId;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FText FocusedEventDisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bHasLastBuiltAiRequest = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FAiGmRequest LastBuiltAiRequest;

private:
    void ApplyProjectSettingsDefaults();
    void RefreshFocusedEventPresentation();
    FSessionActionResult FailResult(const FString& Message);
    FSessionActionResult HandleActionResult(const FSessionActionResult& ActionResult);
};
