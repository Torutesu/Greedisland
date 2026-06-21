#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AiGm/AiGmTypes.h"
#include "Actors/GreeislandBootstrapActor.h"
#include "Runtime/GreeislandGameSubsystem.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandDebugHudWidget.generated.h"

USTRUCT(BlueprintType)
struct FGreeislandHudChecklistItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString Group;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString Label;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString BindingHint;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bRecommendedByDefault = true;
};

USTRUCT(BlueprintType)
struct FGreeislandWalkthroughStep
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    int32 Order = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString Label;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FName EventId;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString ActionHint;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString SuccessHint;
};

USTRUCT(BlueprintType)
struct FGreeislandBlueprintAssetChecklistItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString AssetName;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString ParentClassName;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString RequiredSetup;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bRequiredForMinimalLoop = true;
};

USTRUCT(BlueprintType)
struct FGreeislandWalkthroughStepState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    int32 Order = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString Label;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FName EventId;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bCurrentFocus = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bEventAvailable = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString StatusLabel;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FString Detail;
};

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
    FSessionActionResult BootstrapSessionFromActor();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    bool BuildAiRequestForActiveEvent(const FString& PlayerChoice, FAiGmRequest& OutRequest);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    bool BuildFallbackAiResponseForActiveEvent(const FString& PlayerChoice, FAiGmResponse& OutResponse);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    FAiGmValidationResult ValidateAiResponseForActiveEvent(
        const FAiGmResponse& Response,
        const FString& PlayerChoice);

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

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const FAiGmResponse& GetLastAiResponse() const
    {
        return LastAiResponse;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    bool HasLastAiResponse() const
    {
        return bHasLastAiResponse;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const FAiGmValidationResult& GetLastAiValidationResult() const
    {
        return LastAiValidationResult;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const FGreeislandBootstrapDiagnostics& GetLastBootstrapDiagnostics() const
    {
        return LastBootstrapDiagnostics;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandHudChecklistItem>& GetRecommendedHudChecklist() const
    {
        return RecommendedHudChecklist;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandWalkthroughStep>& GetRecommendedWalkthrough() const
    {
        return RecommendedWalkthrough;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandWalkthroughStepState>& GetWalkthroughProgress() const
    {
        return WalkthroughProgress;
    }

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    const TArray<FGreeislandBlueprintAssetChecklistItem>& GetRecommendedBlueprintAssets() const
    {
        return RecommendedBlueprintAssets;
    }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    bool bUseProjectSettingsDefaults = true;

    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|UI")
    void OnPresentationUpdated();

    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|UI")
    void OnActionResultUpdated(const FSessionActionResult& ActionResult);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    FString CardJsonPath = TEXT("../data/cards/cards.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Config")
    FString EventJsonPath = TEXT("../data/events/events.mvp.json");

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

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    bool bHasLastAiResponse = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FAiGmResponse LastAiResponse;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FAiGmValidationResult LastAiValidationResult;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    FGreeislandBootstrapDiagnostics LastBootstrapDiagnostics;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandHudChecklistItem> RecommendedHudChecklist;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandWalkthroughStep> RecommendedWalkthrough;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandWalkthroughStepState> WalkthroughProgress;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|UI")
    TArray<FGreeislandBlueprintAssetChecklistItem> RecommendedBlueprintAssets;

private:
    void ApplyProjectSettingsDefaults();
    void BuildRecommendedHudChecklist();
    void BuildRecommendedWalkthrough();
    void BuildRecommendedBlueprintAssets();
    void BuildWalkthroughProgress();
    void RefreshBootstrapDiagnostics();
    void RefreshFocusedEventPresentation();
    AGreeislandBootstrapActor* FindBootstrapActor() const;
    const FGreeislandEventViewData* FindEventViewDataById(FName EventId) const;
    bool HasOwnedCard(FName CardId) const;
    bool HasCompletedEvent(FName EventId) const;
    FSessionActionResult FailResult(const FString& Message);
    FSessionActionResult HandleActionResult(const FSessionActionResult& ActionResult);
};
