#include "UI/GreeislandDebugHudWidget.h"

#include "Actors/GreeislandBootstrapActor.h"
#include "Actors/GreeislandEventActor.h"
#include "Characters/GreeislandDebugCharacter.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Runtime/GreeislandProjectSettings.h"

void UGreeislandDebugHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyProjectSettingsDefaults();
    BuildRecommendedHudChecklist();
    BuildRecommendedWalkthrough();
    BuildRecommendedBlueprintAssets();
    RefreshPresentation();
}

UGreeislandGameSubsystem* UGreeislandDebugHudWidget::GetGreeislandSubsystem() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UGreeislandGameSubsystem>();
}

void UGreeislandDebugHudWidget::RefreshPresentation()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        CurrentSnapshot = FGreeislandUiSnapshot();
        PlayableCombatCardIds.Reset();
        OwnedCardViewData.Reset();
        HandCardViewData.Reset();
        EventViewData.Reset();
        EventActorStatusViewData.Reset();
        RefreshBootstrapDiagnostics();
        RefreshFocusedEventPresentation();
        BuildEventActorStatusViewData();
        BuildWalkthroughProgress();
        OnPresentationUpdated();
        return;
    }

    CurrentSnapshot = Subsystem->BuildUiSnapshot(SnapshotLogLineCount);
    Subsystem->GetPlayableCombatCardIds(PlayableCombatCardIds);
    Subsystem->BuildOwnedCardViewData(OwnedCardViewData);
    Subsystem->BuildHandCardViewData(HandCardViewData);
    Subsystem->BuildEventViewData(EventViewData);
    RefreshBootstrapDiagnostics();
    RefreshFocusedEventPresentation();
    BuildEventActorStatusViewData();
    BuildWalkthroughProgress();
    OnPresentationUpdated();
}

FSessionActionResult UGreeislandDebugHudWidget::InitializeNewSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->InitializeNewSessionFromFiles(CardJsonPath, EventJsonPath));
}

FSessionActionResult UGreeislandDebugHudWidget::RestoreSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(
        Subsystem->RestoreSessionFromSaveSlot(
            SaveSlotName,
            SaveUserIndex,
            CardJsonPath,
            EventJsonPath));
}

FSessionActionResult UGreeislandDebugHudWidget::SaveSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->SaveSessionToSlot(SaveSlotName, SaveUserIndex, DefaultPlayerId));
}

FSessionActionResult UGreeislandDebugHudWidget::ResolveEventById(FName EventId)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->ResolveEvent(EventId));
}

FSessionActionResult UGreeislandDebugHudWidget::ResolveActiveEvent()
{
    return ResolveEventById(CurrentSnapshot.ActiveEventId);
}

FSessionActionResult UGreeislandDebugHudWidget::StartCombatForActiveEvent()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(
        Subsystem->StartCombat(CurrentSnapshot.ActiveEventId, DefaultOpeningDrawCount));
}

FSessionActionResult UGreeislandDebugHudWidget::PlayCombatCardById(FName CardId)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->PlayCombatCard(CardId));
}

FSessionActionResult UGreeislandDebugHudWidget::RunEnemyTurn()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->RunEnemyTurn(1));
}

FSessionActionResult UGreeislandDebugHudWidget::InteractWithFocusedEvent()
{
    AGreeislandDebugCharacter* Character = Cast<AGreeislandDebugCharacter>(GetOwningPlayerPawn());
    if (!Character)
    {
        return HandleActionResult(FailResult(TEXT("Debug character is unavailable.")));
    }

    AGreeislandEventActor* EventActor = Character->FindBestInteractableEventActor();
    if (!EventActor)
    {
        return HandleActionResult(FailResult(TEXT("No interactable event is in range.")));
    }

    return HandleActionResult(EventActor->TriggerEvent());
}

FSessionActionResult UGreeislandDebugHudWidget::GrantDeveloperCard(FName CardId, bool bAddToDeck)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    return HandleActionResult(Subsystem->GrantDeveloperCard(CardId, bAddToDeck));
}

FSessionActionResult UGreeislandDebugHudWidget::BootstrapSessionFromActor()
{
    AGreeislandBootstrapActor* BootstrapActor = FindBootstrapActor();
    if (!BootstrapActor)
    {
        return HandleActionResult(FailResult(TEXT("No GreeislandBootstrapActor was found in the current world.")));
    }

    const FSessionActionResult Result = BootstrapActor->BootstrapSession();
    RefreshBootstrapDiagnostics();
    return HandleActionResult(Result);
}

bool UGreeislandDebugHudWidget::BuildAiRequestForActiveEvent(
    const FString& PlayerChoice,
    FAiGmRequest& OutRequest)
{
    bHasLastBuiltAiRequest = false;
    LastBuiltAiRequest = FAiGmRequest();
    OutRequest = FAiGmRequest();

    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return false;
    }

    if (!Subsystem->BuildAiRequest(CurrentSnapshot.ActiveEventId, PlayerChoice, OutRequest))
    {
        return false;
    }

    bHasLastBuiltAiRequest = true;
    LastBuiltAiRequest = OutRequest;
    return true;
}

bool UGreeislandDebugHudWidget::BuildFallbackAiResponseForActiveEvent(
    const FString& PlayerChoice,
    FAiGmResponse& OutResponse)
{
    bHasLastAiResponse = false;
    LastAiResponse = FAiGmResponse();
    LastAiValidationResult = FAiGmValidationResult();
    OutResponse = FAiGmResponse();

    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return false;
    }

    if (!Subsystem->BuildFallbackAiResponseForActiveEvent(PlayerChoice, OutResponse))
    {
        return false;
    }

    bHasLastAiResponse = true;
    LastAiResponse = OutResponse;
    LastAiValidationResult = Subsystem->ValidateAiResponseForActiveEvent(OutResponse, PlayerChoice);
    return true;
}

FAiGmValidationResult UGreeislandDebugHudWidget::ValidateAiResponseForActiveEvent(
    const FAiGmResponse& Response,
    const FString& PlayerChoice)
{
    LastAiValidationResult = FAiGmValidationResult();

    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        LastAiValidationResult.Reasons.Add(TEXT("Game subsystem is unavailable."));
        return LastAiValidationResult;
    }

    LastAiValidationResult = Subsystem->ValidateAiResponseForActiveEvent(Response, PlayerChoice);
    return LastAiValidationResult;
}

FSessionActionResult UGreeislandDebugHudWidget::ApplyAiRewardResponse(
    const FString& SpeakerName,
    const FString& Dialogue,
    const TArray<FName>& AllowedRewardCardIds,
    const FString& PlayerChoice,
    EAiGmIntent Intent)
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    FAiGmResponse Response;
    Response.SpeakerName = SpeakerName;
    Response.Dialogue = Dialogue;
    Response.Intent = Intent;
    Response.AllowedRewardCardIds = AllowedRewardCardIds;
    Response.DifficultyHint = TEXT("medium");
    bHasLastAiResponse = true;
    LastAiResponse = Response;
    LastAiValidationResult = Subsystem->ValidateAiResponseForActiveEvent(Response, PlayerChoice);
    return HandleActionResult(Subsystem->ApplyAiResponse(Response, PlayerChoice));
}

void UGreeislandDebugHudWidget::ApplyProjectSettingsDefaults()
{
    if (!bUseProjectSettingsDefaults)
    {
        return;
    }

    const UGreeislandProjectSettings* Settings = GetDefault<UGreeislandProjectSettings>();
    if (!Settings)
    {
        return;
    }

    CardJsonPath = Settings->CardJsonPath;
    EventJsonPath = Settings->EventJsonPath;
    SaveSlotName = Settings->SaveSlotName;
    SaveUserIndex = Settings->SaveUserIndex;
    DefaultPlayerId = Settings->DefaultPlayerId;
    DefaultOpeningDrawCount = Settings->DefaultOpeningDrawCount;
    SnapshotLogLineCount = Settings->SnapshotLogLineCount;
}

void UGreeislandDebugHudWidget::BuildRecommendedHudChecklist()
{
    RecommendedHudChecklist.Reset();

    auto AddChecklistItem = [this](const FString& Group, const FString& Label, const FString& BindingHint, bool bRecommended = true)
    {
        FGreeislandHudChecklistItem Item;
        Item.Group = Group;
        Item.Label = Label;
        Item.BindingHint = BindingHint;
        Item.bRecommendedByDefault = bRecommended;
        RecommendedHudChecklist.Add(Item);
    };

    AddChecklistItem(TEXT("Status"), TEXT("Current Event Name"), TEXT("CurrentSnapshot.ActiveEventDisplayName"));
    AddChecklistItem(TEXT("Status"), TEXT("Bootstrap Issues"), TEXT("LastBootstrapDiagnostics.Issues"));
    AddChecklistItem(TEXT("Status"), TEXT("Focused Event Name"), TEXT("FocusedEventDisplayName"));
    AddChecklistItem(TEXT("Lists"), TEXT("Available Events"), TEXT("EventViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Hand Cards"), TEXT("HandCardViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Owned Cards"), TEXT("OwnedCardViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Recent Log Lines"), TEXT("CurrentSnapshot.RecentLogLines"));
    AddChecklistItem(TEXT("Lists"), TEXT("Expected Event Placements"), TEXT("LastBootstrapDiagnostics.ExpectedEventPlacements"));
    AddChecklistItem(TEXT("Lists"), TEXT("Expected Event Route"), TEXT("LastBootstrapDiagnostics.ExpectedEventRoute"));
    AddChecklistItem(TEXT("Lists"), TEXT("Event Actor Status"), TEXT("EventActorStatusViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Walkthrough Progress"), TEXT("WalkthroughProgress"));
    AddChecklistItem(TEXT("Actions"), TEXT("Bootstrap Session"), TEXT("BootstrapSessionFromActor"));
    AddChecklistItem(TEXT("Actions"), TEXT("Initialize New Session"), TEXT("InitializeNewSession"));
    AddChecklistItem(TEXT("Actions"), TEXT("Restore Session"), TEXT("RestoreSession"));
    AddChecklistItem(TEXT("Actions"), TEXT("Save Session"), TEXT("SaveSession"));
    AddChecklistItem(TEXT("Actions"), TEXT("Resolve Active Event"), TEXT("ResolveActiveEvent"));
    AddChecklistItem(TEXT("Actions"), TEXT("Start Combat For Active Event"), TEXT("StartCombatForActiveEvent"));
    AddChecklistItem(TEXT("Actions"), TEXT("Run Enemy Turn"), TEXT("RunEnemyTurn"));
    AddChecklistItem(TEXT("Actions"), TEXT("Grant Developer Card"), TEXT("GrantDeveloperCard"));
    AddChecklistItem(TEXT("Actions"), TEXT("Build AI Request"), TEXT("BuildAiRequestForActiveEvent"), false);
    AddChecklistItem(TEXT("Actions"), TEXT("Build Fallback AI Response"), TEXT("BuildFallbackAiResponseForActiveEvent"), false);
}

void UGreeislandDebugHudWidget::BuildRecommendedWalkthrough()
{
    RecommendedWalkthrough.Reset();

    auto AddWalkthroughStep =
        [this](int32 Order, const FString& Label, FName EventId, const FString& ActionHint, const FString& SuccessHint)
    {
        FGreeislandWalkthroughStep Step;
        Step.Order = Order;
        Step.Label = Label;
        Step.EventId = EventId;
        Step.ActionHint = ActionHint;
        Step.SuccessHint = SuccessHint;
        RecommendedWalkthrough.Add(Step);
    };

    AddWalkthroughStep(
        1,
        TEXT("Bootstrap Session"),
        NAME_None,
        TEXT("BootstrapSessionFromActor or InitializeNewSession"),
        TEXT("LastBootstrapDiagnostics.Issues が空で、CurrentSnapshot.bHasInitializedSession が true"));
    AddWalkthroughStep(
        2,
        TEXT("Wake Cache"),
        TEXT("event_wake_cache_001"),
        TEXT("開始地点の EventActor を E で起動、または ResolveEventById(event_wake_cache_001)"),
        TEXT("OwnedCardViewData に starter cards、EventViewData に event_contract_broker_001"));
    AddWalkthroughStep(
        3,
        TEXT("Contract Broker"),
        TEXT("event_contract_broker_001"),
        TEXT("契約屋リオへ接触。必要なら BuildFallbackAiResponseForActiveEvent で会話文面も確認"),
        TEXT("item_contract_001 / rule_party_proxy_001 が揃い、event_silent_shrine_001 と event_ridge_scout_001 が開放"));
    AddWalkthroughStep(
        4,
        TEXT("Silent Shrine"),
        TEXT("event_silent_shrine_001"),
        TEXT("Quest EventActor を起動して制約系カードを取得"),
        TEXT("CompletedQuestIds に event_silent_shrine_001、OwnedCardViewData に con_silent_oath_001 などが追加"));
    AddWalkthroughStep(
        5,
        TEXT("Ridge Scout Battle"),
        TEXT("event_ridge_scout_001"),
        TEXT("StartCombatForActiveEvent で戦闘開始し、HandCardViewData を使って勝利まで進める"),
        TEXT("con_four_party_001 が獲得され、event_proxy_gate_001 が開放"));
    AddWalkthroughStep(
        6,
        TEXT("Proxy Gate"),
        TEXT("event_proxy_gate_001"),
        TEXT("必要カードが揃ったらゲート EventActor を起動"),
        TEXT("key_zone_core_001 が獲得され、CurrentSnapshot.bZoneCleared が true"));
    AddWalkthroughStep(
        7,
        TEXT("Save And Restore"),
        NAME_None,
        TEXT("SaveSession のあと RestoreSession を実行"),
        TEXT("OwnedCardViewData と CompletedQuestIds が維持され、ゾーンクリア状態も復元される"));
}

void UGreeislandDebugHudWidget::BuildRecommendedBlueprintAssets()
{
    RecommendedBlueprintAssets.Reset();

    auto AddBlueprintAsset =
        [this](const FString& AssetName, const FString& ParentClassName, const FString& RequiredSetup, bool bRequired = true)
    {
        FGreeislandBlueprintAssetChecklistItem Item;
        Item.AssetName = AssetName;
        Item.ParentClassName = ParentClassName;
        Item.RequiredSetup = RequiredSetup;
        Item.bRequiredForMinimalLoop = bRequired;
        RecommendedBlueprintAssets.Add(Item);
    };

    AddBlueprintAsset(
        TEXT("BP_GreeislandDebugHudWidget"),
        TEXT("UGreeislandDebugHudWidget"),
        TEXT("RecommendedHudChecklist / RecommendedWalkthrough / WalkthroughProgress / EventActorStatusViewData / LastBootstrapDiagnostics を表示する"));
    AddBlueprintAsset(
        TEXT("BP_GreeislandDebugHud"),
        TEXT("AGreeislandDebugHud"),
        TEXT("DebugHudWidgetClass に BP_GreeislandDebugHudWidget を設定する"));
    AddBlueprintAsset(
        TEXT("BP_GreeislandDebugGameMode"),
        TEXT("AGreeislandDebugGameMode"),
        TEXT("必要なら HUD Class を BP_GreeislandDebugHud に差し替える"));
    AddBlueprintAsset(
        TEXT("BP_GreeislandBootstrapActor"),
        TEXT("AGreeislandBootstrapActor"),
        TEXT("bUseProjectSettingsDefaults を有効にし、レベルに1個だけ置く"));
    AddBlueprintAsset(
        TEXT("BP_GreeislandEventActor"),
        TEXT("AGreeislandEventActor"),
        TEXT("ExpectedEventPlacements に合わせて複製し、それぞれの EventId を設定する"));
    AddBlueprintAsset(
        TEXT("BP_GreeislandDebugCharacter"),
        TEXT("AGreeislandDebugCharacter"),
        TEXT("必要なら見た目メッシュを差し込み、WASD / Mouse / E 動作を確認する"));
}

void UGreeislandDebugHudWidget::BuildWalkthroughProgress()
{
    WalkthroughProgress.Reset();

    bool bAssignedCurrentFocus = false;

    auto AddProgress =
        [this, &bAssignedCurrentFocus](
            int32 Order,
            const FString& Label,
            FName EventId,
            bool bCompleted,
            bool bEventAvailable,
            const FString& Detail)
    {
        FGreeislandWalkthroughStepState StepState;
        StepState.Order = Order;
        StepState.Label = Label;
        StepState.EventId = EventId;
        StepState.bCompleted = bCompleted;
        StepState.bEventAvailable = bEventAvailable;
        StepState.bCurrentFocus = !bCompleted && !bAssignedCurrentFocus;
        StepState.StatusLabel = bCompleted
            ? TEXT("Completed")
            : (StepState.bCurrentFocus ? TEXT("Next Up") : (bEventAvailable ? TEXT("Available") : TEXT("Locked")));
        StepState.Detail = Detail;
        WalkthroughProgress.Add(StepState);

        if (StepState.bCurrentFocus)
        {
            bAssignedCurrentFocus = true;
        }
    };

    const bool bBootstrapOk =
        CurrentSnapshot.bHasInitializedSession &&
        LastBootstrapDiagnostics.Issues.Num() == 0;
    AddProgress(
        1,
        TEXT("Bootstrap Session"),
        NAME_None,
        bBootstrapOk,
        CurrentSnapshot.bHasInitializedSession,
        bBootstrapOk
            ? TEXT("Session initialized and bootstrap diagnostics are clean.")
            : (CurrentSnapshot.bHasInitializedSession
                ? TEXT("Session exists, but bootstrap diagnostics still report issues.")
                : TEXT("Run BootstrapSessionFromActor or InitializeNewSession first.")));

    const bool bWakeCacheDone =
        HasCompletedEvent(TEXT("event_wake_cache_001")) ||
        (HasOwnedCard(TEXT("act_strike_001")) &&
         HasOwnedCard(TEXT("act_guard_001")) &&
         HasOwnedCard(TEXT("act_patch_heal_001")) &&
         HasOwnedCard(TEXT("act_draw_001")));
    AddProgress(
        2,
        TEXT("Wake Cache"),
        TEXT("event_wake_cache_001"),
        bWakeCacheDone,
        CurrentSnapshot.AvailableEventIds.Contains(TEXT("event_wake_cache_001")),
        bWakeCacheDone
            ? TEXT("Starter combat cards are in the collection.")
            : TEXT("Resolve the opening treasure event to seed the first hand tools."));

    const bool bContractBrokerDone =
        HasCompletedEvent(TEXT("event_contract_broker_001")) ||
        (HasOwnedCard(TEXT("item_contract_001")) && HasOwnedCard(TEXT("rule_party_proxy_001")));
    AddProgress(
        3,
        TEXT("Contract Broker"),
        TEXT("event_contract_broker_001"),
        bContractBrokerDone,
        CurrentSnapshot.AvailableEventIds.Contains(TEXT("event_contract_broker_001")),
        bContractBrokerDone
            ? TEXT("Broker rewards are in the collection and the route split should be open.")
            : TEXT("Talk to Rio and confirm contract/proxy rule rewards land correctly."));

    const bool bSilentShrineDone =
        HasCompletedEvent(TEXT("event_silent_shrine_001")) ||
        CurrentSnapshot.CompletedQuestIds.Contains(TEXT("event_silent_shrine_001")) ||
        HasOwnedCard(TEXT("con_silent_oath_001"));
    AddProgress(
        4,
        TEXT("Silent Shrine"),
        TEXT("event_silent_shrine_001"),
        bSilentShrineDone,
        CurrentSnapshot.AvailableEventIds.Contains(TEXT("event_silent_shrine_001")),
        bSilentShrineDone
            ? TEXT("Quest reward card set is present and the shrine path is complete.")
            : TEXT("Resolve the quest event and confirm the oath/reward-hack cards appear."));

    const bool bRidgeScoutDone =
        HasCompletedEvent(TEXT("event_ridge_scout_001")) ||
        HasOwnedCard(TEXT("con_four_party_001"));
    AddProgress(
        5,
        TEXT("Ridge Scout Battle"),
        TEXT("event_ridge_scout_001"),
        bRidgeScoutDone,
        CurrentSnapshot.AvailableEventIds.Contains(TEXT("event_ridge_scout_001")),
        bRidgeScoutDone
            ? TEXT("Battle reward landed, so the proxy gate path should be satisfiable.")
            : (CurrentSnapshot.bCombatActive && CurrentSnapshot.ActiveEventId == TEXT("event_ridge_scout_001")
                ? TEXT("Combat is live. Finish the battle and confirm the four-party condition reward.")
                : TEXT("Start the first battle and verify combat -> reward resolution.")));

    const bool bProxyGateDone =
        HasCompletedEvent(TEXT("event_proxy_gate_001")) ||
        HasOwnedCard(TEXT("key_zone_core_001")) ||
        CurrentSnapshot.bZoneCleared;
    AddProgress(
        6,
        TEXT("Proxy Gate"),
        TEXT("event_proxy_gate_001"),
        bProxyGateDone,
        CurrentSnapshot.AvailableEventIds.Contains(TEXT("event_proxy_gate_001")),
        bProxyGateDone
            ? TEXT("Final key reward is owned and the zone clear condition is satisfied.")
            : TEXT("Use the contract + proxy + four-party combo to open the final gate."));

    const bool bSaveRestoreDone =
        bProxyGateDone &&
        LastBootstrapDiagnostics.bSaveExists &&
        CurrentSnapshot.bHasInitializedSession;
    AddProgress(
        7,
        TEXT("Save And Restore"),
        NAME_None,
        bSaveRestoreDone,
        LastBootstrapDiagnostics.bSaveExists,
        bSaveRestoreDone
            ? TEXT("A save slot exists after the zone-clear path, so restore verification is ready.")
            : (LastBootstrapDiagnostics.bSaveExists
                ? TEXT("A save exists. Run RestoreSession and confirm the clear state remains intact.")
                : TEXT("Run SaveSession after a meaningful state change, then verify RestoreSession.")));
}

void UGreeislandDebugHudWidget::BuildEventActorStatusViewData()
{
    EventActorStatusViewData.Reset();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const APawn* OwningPawn = GetOwningPlayerPawn();
    TMap<FName, int32> PlacementCounts;
    TMap<FName, float> NearestDistanceByEventId;

    for (TActorIterator<AGreeislandEventActor> It(World); It; ++It)
    {
        const AGreeislandEventActor* EventActor = *It;
        if (!EventActor)
        {
            continue;
        }

        const FName EventId = EventActor->GetEventId();
        if (EventId.IsNone())
        {
            continue;
        }

        PlacementCounts.FindOrAdd(EventId) += 1;

        if (OwningPawn)
        {
            const float Distance = FVector::Distance(EventActor->GetActorLocation(), OwningPawn->GetActorLocation());
            float& NearestDistance = NearestDistanceByEventId.FindOrAdd(EventId);
            if (NearestDistance <= 0.0f || Distance < NearestDistance)
            {
                NearestDistance = Distance;
            }
        }
    }

    for (const FGreeislandEventViewData& EventView : EventViewData)
    {
        FGreeislandEventActorStatusViewData Status;
        Status.EventId = EventView.EventId;
        Status.DisplayName = EventView.DisplayName;
        switch (EventView.Type)
        {
            case EExplorationEventType::Battle:
                Status.EventType = TEXT("Battle");
                break;
            case EExplorationEventType::Treasure:
                Status.EventType = TEXT("Treasure");
                break;
            case EExplorationEventType::Npc:
                Status.EventType = TEXT("Npc");
                break;
            case EExplorationEventType::Quest:
                Status.EventType = TEXT("Quest");
                break;
            case EExplorationEventType::KeyGate:
                Status.EventType = TEXT("KeyGate");
                break;
            default:
                Status.EventType = TEXT("Unknown");
                break;
        }

        Status.PlacementCount = PlacementCounts.FindRef(EventView.EventId);
        Status.bHasActorPlacement = Status.PlacementCount > 0;
        Status.bAvailable = EventView.bAvailable;
        Status.bCompleted = EventView.bCompleted;
        Status.bIsActive = EventView.bIsActive;
        Status.bIsFocused = bHasFocusedEvent && FocusedEventId == EventView.EventId;
        Status.bIsInteractableNow = Status.bAvailable && Status.bIsFocused;
        Status.NearestDistance = NearestDistanceByEventId.Contains(EventView.EventId)
            ? NearestDistanceByEventId.FindRef(EventView.EventId)
            : -1.0f;
        Status.StatusSummary = BuildEventActorStatusSummary(Status);
        EventActorStatusViewData.Add(Status);
    }
}

void UGreeislandDebugHudWidget::RefreshBootstrapDiagnostics()
{
    LastBootstrapDiagnostics = FGreeislandBootstrapDiagnostics();

    AGreeislandBootstrapActor* BootstrapActor = FindBootstrapActor();
    if (!BootstrapActor)
    {
        LastBootstrapDiagnostics.Issues.Add(TEXT("No GreeislandBootstrapActor was found in the current world."));
        return;
    }

    LastBootstrapDiagnostics = BootstrapActor->GetBootstrapDiagnostics();
}

void UGreeislandDebugHudWidget::RefreshFocusedEventPresentation()
{
    bHasFocusedEvent = false;
    FocusedEventId = NAME_None;
    FocusedEventDisplayName = FText::GetEmpty();

    AGreeislandDebugCharacter* Character = Cast<AGreeislandDebugCharacter>(GetOwningPlayerPawn());
    if (!Character)
    {
        return;
    }

    AGreeislandEventActor* EventActor = Character->FindBestInteractableEventActor();
    if (!EventActor)
    {
        return;
    }

    FExplorationEventDefinition EventDefinition;
    if (!EventActor->GetBoundEventDefinition(EventDefinition))
    {
        return;
    }

    bHasFocusedEvent = true;
    FocusedEventId = EventDefinition.EventId;
    FocusedEventDisplayName = FText::FromString(EventDefinition.DisplayName);
}

AGreeislandBootstrapActor* UGreeislandDebugHudWidget::FindBootstrapActor() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AGreeislandBootstrapActor> It(World); It; ++It)
    {
        return *It;
    }

    return nullptr;
}

const FGreeislandEventViewData* UGreeislandDebugHudWidget::FindEventViewDataById(FName EventId) const
{
    for (const FGreeislandEventViewData& Event : EventViewData)
    {
        if (Event.EventId == EventId)
        {
            return &Event;
        }
    }

    return nullptr;
}

bool UGreeislandDebugHudWidget::HasOwnedCard(FName CardId) const
{
    return CurrentSnapshot.OwnedCardIds.Contains(CardId);
}

bool UGreeislandDebugHudWidget::HasCompletedEvent(FName EventId) const
{
    if (const FGreeislandEventViewData* Event = FindEventViewDataById(EventId))
    {
        return Event->bCompleted;
    }

    return false;
}

FString UGreeislandDebugHudWidget::BuildEventActorStatusSummary(
    const FGreeislandEventActorStatusViewData& Status) const
{
    TArray<FString> Parts;

    Parts.Add(Status.bHasActorPlacement
        ? FString::Printf(TEXT("placed x%d"), Status.PlacementCount)
        : TEXT("missing actor"));
    Parts.Add(Status.bCompleted
        ? TEXT("completed")
        : (Status.bAvailable ? TEXT("available") : TEXT("locked")));

    if (Status.bIsActive)
    {
        Parts.Add(TEXT("active"));
    }

    if (Status.bIsFocused)
    {
        Parts.Add(TEXT("focused"));
    }

    if (Status.bIsInteractableNow)
    {
        Parts.Add(TEXT("interactable"));
    }

    if (Status.NearestDistance >= 0.0f)
    {
        Parts.Add(FString::Printf(TEXT("dist %.0f"), Status.NearestDistance));
    }

    return FString::Join(Parts, TEXT(" | "));
}

FSessionActionResult UGreeislandDebugHudWidget::FailResult(const FString& Message)
{
    FSessionActionResult Result;
    Result.Reasons.Add(Message);
    return Result;
}

FSessionActionResult UGreeislandDebugHudWidget::HandleActionResult(const FSessionActionResult& ActionResult)
{
    LastActionResult = ActionResult;
    RefreshPresentation();
    OnActionResultUpdated(LastActionResult);
    return LastActionResult;
}
