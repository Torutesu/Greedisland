#include "UI/GreeislandDebugHudWidget.h"

#include "Actors/GreeislandBootstrapActor.h"
#include "Actors/GreeislandEventActor.h"
#include "Characters/GreeislandDebugCharacter.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "Runtime/GreeislandProjectSettings.h"

void UGreeislandDebugHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    ApplyProjectSettingsDefaults();
    BuildRecommendedHudChecklist();
    BuildRecommendedWalkthrough();
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
        RefreshBootstrapDiagnostics();
        RefreshFocusedEventPresentation();
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
