#include "UI/GreeislandDebugHudWidget.h"

#include "Algo/AnyOf.h"
#include "Actors/GreeislandBootstrapActor.h"
#include "Actors/GreeislandEventActor.h"
#include "Characters/GreeislandDebugCharacter.h"
#include "EngineUtils.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Runtime/GreeislandProjectSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
UTextBlock* AddNativeText(UWidgetTree* WidgetTree, UVerticalBox* Box, const FString& InitialText, float FontSize)
{
    if (!WidgetTree || !Box)
    {
        return nullptr;
    }

    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(InitialText));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = FontSize;
    Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.96f, 1.0f, 1.0f)));
    Box->AddChildToVerticalBox(Text);
    return Text;
}
}

void UGreeislandDebugHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildNativeLayout();
    ApplyProjectSettingsDefaults();
    BuildRecommendedHudChecklist();
    BuildRecommendedHudPanels();
    BuildRecommendedHudActions();
    BuildRecommendedWalkthrough();
    BuildRecommendedBlueprintAssets();
    RefreshPresentation();
}

void UGreeislandDebugHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bAutoRefreshPresentation)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const double RefreshInterval = FMath::Max(0.0, static_cast<double>(AutoRefreshIntervalSeconds));
    const double NowSeconds = World->GetTimeSeconds();
    if (LastPresentationRefreshTimeSeconds >= 0.0 &&
        (NowSeconds - LastPresentationRefreshTimeSeconds) < RefreshInterval)
    {
        return;
    }

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
    UWorld* World = GetWorld();
    LastPresentationRefreshTimeSeconds = World ? World->GetTimeSeconds() : -1.0;

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
        BuildHudActionStates();
        BuildHudActionButtons();
        BuildCurrentObjectiveAction();
        BuildVerificationChecks();
        BuildSessionStatusRows();
        OnPresentationUpdated();
        UpdateNativeLayout();
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
    BuildHudActionStates();
    BuildHudActionButtons();
    BuildCurrentObjectiveAction();
    BuildVerificationChecks();
    BuildSessionStatusRows();
    OnPresentationUpdated();
    UpdateNativeLayout();
}

void UGreeislandDebugHudWidget::BuildNativeLayout()
{
    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.07f, 0.9f));
    Panel->SetPadding(FMargin(18.0f));
    SetDesiredSizeInViewport(FVector2D(780.0f, 920.0f));

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Panel->SetContent(Layout);
    WidgetTree->RootWidget = Panel;

    AddNativeText(WidgetTree, Layout, TEXT("GREEISLAND // MVP ZONE"), 22.0f);
    NativeStatusText = AddNativeText(WidgetTree, Layout, TEXT("Loading session..."), 16.0f);
    NativeObjectiveText = AddNativeText(WidgetTree, Layout, TEXT("Objective: --"), 16.0f);
    NativeEventsText = AddNativeText(WidgetTree, Layout, TEXT("Events: --"), 14.0f);
    NativeCardsText = AddNativeText(WidgetTree, Layout, TEXT("Cards: --"), 14.0f);
    NativeAiText = AddNativeText(WidgetTree, Layout, TEXT("AI GM: --"), 14.0f);
    AddNativeText(
        WidgetTree,
        Layout,
        TEXT("WASD move   E interact   Use HUD Blueprint actions for combat/save/AI"),
        12.0f);
    AddNativeText(
        WidgetTree,
        Layout,
        TEXT("Keys: N new  O restore  K save  R resolve  C combat  1-5 play hand  Space enemy  F/T AI"),
        12.0f);
}

void UGreeislandDebugHudWidget::UpdateNativeLayout()
{
    if (!NativeStatusText)
    {
        return;
    }

    NativeStatusText->SetText(FText::FromString(FString::Printf(
        TEXT("Session: %s   Zone: %s   Active: %s   HP %d   Enemy %d   Energy %d"),
        CurrentSnapshot.bHasInitializedSession ? TEXT("READY") : TEXT("NOT READY"),
        CurrentSnapshot.ZoneId.IsNone() ? TEXT("--") : *CurrentSnapshot.ZoneId.ToString(),
        CurrentSnapshot.ActiveEventId.IsNone() ? TEXT("--") : *CurrentSnapshot.ActiveEventId.ToString(),
        CurrentSnapshot.PlayerHp,
        CurrentSnapshot.EnemyHp,
        CurrentSnapshot.Energy)));

    const FString Objective = CurrentObjectiveAction.bHasAction
        ? FString::Printf(
            TEXT("Objective %d: %s | %s | %s"),
            CurrentObjectiveAction.StepOrder,
            *CurrentObjectiveAction.StepLabel,
            *CurrentObjectiveAction.ActionLabel,
            *CurrentObjectiveAction.AvailabilityLabel)
        : (CurrentSnapshot.bZoneCleared ? TEXT("Objective: ZONE CLEARED") : TEXT("Objective: Explore the first cache"));
    NativeObjectiveText->SetText(FText::FromString(Objective));

    FString EventLines;
    for (const FGreeislandEventViewData& Event : EventViewData)
    {
        EventLines += FString::Printf(
            TEXT("%s  [%s]  %s%s\n"),
            *Event.DisplayName.ToString(),
            *Event.TypeLabel,
            *Event.StatusSummary,
            Event.bIsActive ? TEXT("  < ACTIVE >") : TEXT(""));
    }
    if (EventLines.IsEmpty())
    {
        EventLines = TEXT("No event data loaded.");
    }
    NativeEventsText->SetText(FText::FromString(FString::Printf(TEXT("EVENTS\n%s"), *EventLines)));

    FString CardLines;
    for (const FGreeislandCardViewData& Card : OwnedCardViewData)
    {
        CardLines += FString::Printf(
            TEXT("%s  [%s]  %s%s\n"),
            *Card.DisplayName.ToString(),
            *Card.KindLabel,
            *Card.StateSummary,
            Card.bPlayableNow ? TEXT("  < PLAYABLE >") : TEXT(""));
    }
    if (CardLines.IsEmpty())
    {
        CardLines = TEXT("No cards acquired yet.");
    }
    NativeCardsText->SetText(FText::FromString(FString::Printf(TEXT("CARDS\n%s"), *CardLines)));

    if (!bHasLastAiResponse && CurrentSnapshot.ActiveEventId == TEXT("event_contract_broker_001"))
    {
        FAiGmResponse FallbackResponse;
        BuildFallbackAiResponseForActiveEvent(TEXT("交渉する"), FallbackResponse);
    }

    const FString AiText = bHasLastAiResponse
        ? FString::Printf(
            TEXT("AI GM // %s\n%s\nIntent: %d  Rewards: %d  Validation: %s"),
            *LastAiResponse.SpeakerName,
            *LastAiResponse.Dialogue,
            static_cast<int32>(LastAiResponse.Intent),
            LastAiResponse.AllowedRewardCardIds.Num(),
            LastAiValidationResult.bAccepted ? TEXT("ACCEPTED") : TEXT("REJECTED"))
        : TEXT("AI GM // no response yet");
    NativeAiText->SetText(FText::FromString(AiText));
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
    LastAiPlayerChoice = PlayerChoice;
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
    LastAiPlayerChoice = PlayerChoice;
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
    LastAiPlayerChoice = PlayerChoice;

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
    LastAiPlayerChoice = PlayerChoice;
    LastAiValidationResult = Subsystem->ValidateAiResponseForActiveEvent(Response, PlayerChoice);
    return HandleActionResult(Subsystem->ApplyAiResponse(Response, PlayerChoice));
}

FSessionActionResult UGreeislandDebugHudWidget::ApplyLastAiResponse(const FString& PlayerChoice)
{
    if (!bHasLastAiResponse)
    {
        return HandleActionResult(FailResult(TEXT("No AI response is available to apply.")));
    }

    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return HandleActionResult(FailResult(TEXT("Game subsystem is unavailable.")));
    }

    const FString EffectivePlayerChoice = PlayerChoice.IsEmpty() ? LastAiPlayerChoice : PlayerChoice;
    LastAiPlayerChoice = EffectivePlayerChoice;
    LastAiValidationResult = Subsystem->ValidateAiResponseForActiveEvent(LastAiResponse, EffectivePlayerChoice);
    if (!LastAiValidationResult.bIsValid)
    {
        return HandleActionResult(FailResult(FString::Printf(
            TEXT("Last AI response is invalid and cannot be applied: %s"),
            *FString::Join(LastAiValidationResult.Reasons, TEXT(" | ")))));
    }

    return HandleActionResult(Subsystem->ApplyAiResponse(LastAiResponse, EffectivePlayerChoice));
}

FSessionActionResult UGreeislandDebugHudWidget::ExecuteHudActionById(
    const FString& ActionId,
    FName OptionalNameArgument,
    const FString& OptionalStringArgument,
    bool bOptionalFlag)
{
    const FGreeislandHudActionState* ActionState = FindHudActionStateById(ActionId);
    if (ActionState && !ActionState->bEnabled)
    {
        return HandleActionResult(FailResult(FString::Printf(
            TEXT("HUD action '%s' is currently disabled: %s"),
            *ActionId,
            *ActionState->Detail)));
    }

    if (ActionId == TEXT("bootstrap_session"))
    {
        return BootstrapSessionFromActor();
    }

    if (ActionId == TEXT("initialize_new_session"))
    {
        return InitializeNewSession();
    }

    if (ActionId == TEXT("restore_session"))
    {
        return RestoreSession();
    }

    if (ActionId == TEXT("save_session"))
    {
        return SaveSession();
    }

    if (ActionId == TEXT("interact_focused_event"))
    {
        return InteractWithFocusedEvent();
    }

    if (ActionId == TEXT("resolve_active_event"))
    {
        return ResolveActiveEvent();
    }

    if (ActionId == TEXT("start_active_combat"))
    {
        return StartCombatForActiveEvent();
    }

    if (ActionId == TEXT("run_enemy_turn"))
    {
        return RunEnemyTurn();
    }

    if (ActionId == TEXT("grant_developer_card"))
    {
        const FName CardId = OptionalNameArgument.IsNone() ? DefaultDeveloperGrantCardId : OptionalNameArgument;
        if (CardId.IsNone())
        {
            return HandleActionResult(FailResult(TEXT("No developer card id was provided.")));
        }

        return GrantDeveloperCard(CardId, bOptionalFlag);
    }

    if (ActionId == TEXT("play_combat_card"))
    {
        if (OptionalNameArgument.IsNone())
        {
            return HandleActionResult(FailResult(TEXT("play_combat_card requires OptionalNameArgument = CardId.")));
        }

        return PlayCombatCardById(OptionalNameArgument);
    }

    if (ActionId == TEXT("build_ai_request"))
    {
        FAiGmRequest Request;
        if (!BuildAiRequestForActiveEvent(OptionalStringArgument, Request))
        {
            return HandleActionResult(FailResult(TEXT("Failed to build AI request for the current active event.")));
        }

        FSessionActionResult Result;
        Result.bSuccess = true;
        Result.Reasons.Add(FString::Printf(
            TEXT("Built AI request for %s with choice '%s'."),
            CurrentSnapshot.ActiveEventId.IsNone() ? TEXT("no active event") : *CurrentSnapshot.ActiveEventId.ToString(),
            OptionalStringArgument.IsEmpty() ? TEXT("<empty>") : *OptionalStringArgument));
        RefreshPresentation();
        OnActionResultUpdated(Result);
        LastActionResult = Result;
        return LastActionResult;
    }

    if (ActionId == TEXT("build_fallback_ai"))
    {
        FAiGmResponse Response;
        if (!BuildFallbackAiResponseForActiveEvent(OptionalStringArgument, Response))
        {
            return HandleActionResult(FailResult(TEXT("Failed to build fallback AI response for the current active event.")));
        }

        FSessionActionResult Result;
        Result.bSuccess = true;
        Result.Reasons.Add(FString::Printf(
            TEXT("Built fallback AI response for %s with choice '%s'."),
            CurrentSnapshot.ActiveEventId.IsNone() ? TEXT("no active event") : *CurrentSnapshot.ActiveEventId.ToString(),
            OptionalStringArgument.IsEmpty() ? TEXT("<empty>") : *OptionalStringArgument));
        RefreshPresentation();
        OnActionResultUpdated(Result);
        LastActionResult = Result;
        return LastActionResult;
    }

    if (ActionId == TEXT("apply_last_ai_response"))
    {
        return ApplyLastAiResponse(OptionalStringArgument);
    }

    return HandleActionResult(FailResult(FString::Printf(
        TEXT("Unknown HUD action id '%s'."),
        *ActionId)));
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
    AddChecklistItem(TEXT("Status"), TEXT("Session Status Rows"), TEXT("SessionStatusRows"));
    AddChecklistItem(TEXT("Status"), TEXT("Auto Refresh State"), TEXT("bAutoRefreshPresentation / AutoRefreshIntervalSeconds"), false);
    AddChecklistItem(TEXT("Lists"), TEXT("Available Events"), TEXT("EventViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Hand Cards"), TEXT("HandCardViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Owned Cards"), TEXT("OwnedCardViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Recent Log Lines"), TEXT("CurrentSnapshot.RecentLogLines"));
    AddChecklistItem(TEXT("Lists"), TEXT("Expected Event Placements"), TEXT("LastBootstrapDiagnostics.ExpectedEventPlacements"));
    AddChecklistItem(TEXT("Lists"), TEXT("Expected Event Route"), TEXT("LastBootstrapDiagnostics.ExpectedEventRoute"));
    AddChecklistItem(TEXT("Lists"), TEXT("Event Actor Status"), TEXT("EventActorStatusViewData"));
    AddChecklistItem(TEXT("Lists"), TEXT("Walkthrough Progress"), TEXT("WalkthroughProgress"));
    AddChecklistItem(TEXT("Lists"), TEXT("Verification Checks"), TEXT("VerificationChecks"));
    AddChecklistItem(TEXT("Lists"), TEXT("HUD Action Buttons"), TEXT("HudActionButtons"));
    AddChecklistItem(TEXT("Status"), TEXT("Current Objective Action"), TEXT("CurrentObjectiveAction"));
    AddChecklistItem(TEXT("Actions"), TEXT("Bootstrap Session"), TEXT("BootstrapSessionFromActor"));
    AddChecklistItem(TEXT("Actions"), TEXT("Initialize New Session"), TEXT("InitializeNewSession"));
    AddChecklistItem(TEXT("Actions"), TEXT("Restore Session"), TEXT("RestoreSession"));
    AddChecklistItem(TEXT("Actions"), TEXT("Save Session"), TEXT("SaveSession"));
    AddChecklistItem(TEXT("Actions"), TEXT("Resolve Active Event"), TEXT("ResolveActiveEvent"));
    AddChecklistItem(TEXT("Actions"), TEXT("Start Combat For Active Event"), TEXT("StartCombatForActiveEvent"));
    AddChecklistItem(TEXT("Actions"), TEXT("Run Enemy Turn"), TEXT("RunEnemyTurn"));
    AddChecklistItem(TEXT("Actions"), TEXT("Grant Developer Card"), TEXT("GrantDeveloperCard"));
    AddChecklistItem(TEXT("Actions"), TEXT("Execute HUD Action"), TEXT("ExecuteHudActionById"));
    AddChecklistItem(TEXT("Actions"), TEXT("Build AI Request"), TEXT("BuildAiRequestForActiveEvent"), false);
    AddChecklistItem(TEXT("Actions"), TEXT("Build Fallback AI Response"), TEXT("BuildFallbackAiResponseForActiveEvent"), false);
}

void UGreeislandDebugHudWidget::BuildRecommendedHudPanels()
{
    RecommendedHudPanels.Reset();

    auto AddPanel =
        [this](
            const FString& PanelId,
            const FString& Title,
            const FString& Purpose,
            const FString& SuggestedWidgetType,
            int32 Priority,
            const TArray<FString>& BindingHints,
            bool bRecommended = true)
    {
        FGreeislandHudPanelDefinition Panel;
        Panel.PanelId = PanelId;
        Panel.Title = Title;
        Panel.Purpose = Purpose;
        Panel.SuggestedWidgetType = SuggestedWidgetType;
        Panel.Priority = Priority;
        Panel.bRecommendedForMinimalLoop = bRecommended;
        Panel.BindingHints = BindingHints;
        RecommendedHudPanels.Add(Panel);
    };

    AddPanel(
        TEXT("session_status"),
        TEXT("Session Status"),
        TEXT("起動直後の診断と現在地をまとめて確認する最上段パネル。"),
        TEXT("VerticalBox"),
        10,
        {
            TEXT("SessionStatusRows"),
            TEXT("CurrentSnapshot.ActiveEventDisplayName"),
            TEXT("LastBootstrapDiagnostics.Issues"),
            TEXT("FocusedEventDisplayName"),
            TEXT("WalkthroughProgress"),
            TEXT("VerificationChecks")
        });

    AddPanel(
        TEXT("event_reconciliation"),
        TEXT("Event Reconciliation"),
        TEXT("JSON 導線、レベル配置、プレイヤー位置の食い違いを切り分ける。"),
        TEXT("ListView"),
        20,
        {
            TEXT("LastBootstrapDiagnostics.ExpectedEventPlacements"),
            TEXT("LastBootstrapDiagnostics.ExpectedEventRoute"),
            TEXT("EventActorStatusViewData")
        });

    AddPanel(
        TEXT("exploration_state"),
        TEXT("Exploration State"),
        TEXT("現在開いているイベントと最近の進行ログを追う。"),
        TEXT("Splitter"),
        30,
        {
            TEXT("EventViewData"),
            TEXT("CurrentSnapshot.RecentLogLines")
        });

    AddPanel(
        TEXT("combat_hand"),
        TEXT("Combat Hand"),
        TEXT("手札の使用可否と戦闘進行を確認する。"),
        TEXT("WrapBox"),
        40,
        {
            TEXT("HandCardViewData"),
            TEXT("GetPlayableCombatCardIds"),
            TEXT("CurrentSnapshot.PlayerHp"),
            TEXT("CurrentSnapshot.EnemyHp"),
            TEXT("CurrentSnapshot.Energy")
        });

    AddPanel(
        TEXT("collection"),
        TEXT("Collection"),
        TEXT("取得済みカードとルールハックの状態を確認する。"),
        TEXT("ListView"),
        50,
        {
            TEXT("OwnedCardViewData")
        });

    AddPanel(
        TEXT("actions"),
        TEXT("Actions"),
        TEXT("起動、探索、戦闘、保存、開発用注入をまとめる操作列。"),
        TEXT("UniformGridPanel"),
        60,
        {
            TEXT("BootstrapSessionFromActor"),
            TEXT("InitializeNewSession"),
            TEXT("ResolveActiveEvent"),
            TEXT("StartCombatForActiveEvent"),
            TEXT("RunEnemyTurn"),
            TEXT("SaveSession"),
            TEXT("RestoreSession"),
            TEXT("GrantDeveloperCard")
        });

    AddPanel(
        TEXT("ai_debug"),
        TEXT("AI Debug"),
        TEXT("AI GM 入出力の確認専用。最小ループでは任意。"),
        TEXT("VerticalBox"),
        70,
        {
            TEXT("BuildAiRequestForActiveEvent"),
            TEXT("BuildFallbackAiResponseForActiveEvent"),
            TEXT("GetLastBuiltAiRequest"),
            TEXT("GetLastAiResponse"),
            TEXT("GetLastAiValidationResult")
        },
        false);
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

void UGreeislandDebugHudWidget::BuildRecommendedHudActions()
{
    RecommendedHudActions.Reset();

    auto AddAction =
        [this](
            const FString& ActionId,
            const FString& Label,
            const FString& MethodName,
            const FString& PanelId,
            const FString& Purpose,
            const FString& EnableWhen,
            const FString& InputHint,
            int32 Priority,
            bool bRecommended = true)
    {
        FGreeislandHudActionDefinition Action;
        Action.ActionId = ActionId;
        Action.Label = Label;
        Action.MethodName = MethodName;
        Action.PanelId = PanelId;
        Action.Purpose = Purpose;
        Action.EnableWhen = EnableWhen;
        Action.InputHint = InputHint;
        Action.Priority = Priority;
        Action.bRecommendedForMinimalLoop = bRecommended;
        RecommendedHudActions.Add(Action);
    };

    AddAction(
        TEXT("bootstrap_session"),
        TEXT("Bootstrap"),
        TEXT("BootstrapSessionFromActor"),
        TEXT("actions"),
        TEXT("BootstrapActor の設定でセッション初期化または復元を走らせる。"),
        TEXT("BootstrapActor が存在するとき"),
        TEXT("Button"),
        10);
    AddAction(
        TEXT("initialize_new_session"),
        TEXT("New Session"),
        TEXT("InitializeNewSession"),
        TEXT("actions"),
        TEXT("JSON から新規セッションを作る。"),
        TEXT("カード/イベント JSON が読めるとき"),
        TEXT("Button"),
        20);
    AddAction(
        TEXT("restore_session"),
        TEXT("Restore"),
        TEXT("RestoreSession"),
        TEXT("actions"),
        TEXT("既存 save slot からセッションを戻す。"),
        TEXT("SaveSession 実行済み、または save slot が存在するとき"),
        TEXT("Button"),
        30);
    AddAction(
        TEXT("save_session"),
        TEXT("Save"),
        TEXT("SaveSession"),
        TEXT("actions"),
        TEXT("現在の進行を保存する。"),
        TEXT("セッション初期化後"),
        TEXT("Button"),
        40);
    AddAction(
        TEXT("interact_focused_event"),
        TEXT("Interact Focused"),
        TEXT("InteractWithFocusedEvent"),
        TEXT("actions"),
        TEXT("近接中の EventActor をそのまま起動する。"),
        TEXT("FocusedEventDisplayName があり、近接イベントがあるとき"),
        TEXT("Key E"),
        50);
    AddAction(
        TEXT("resolve_active_event"),
        TEXT("Resolve Active"),
        TEXT("ResolveActiveEvent"),
        TEXT("actions"),
        TEXT("アクティブな非戦闘イベントを直接解決する。"),
        TEXT("セッション初期化済み、かつ battle event ではないとき"),
        TEXT("Button"),
        60);
    AddAction(
        TEXT("start_active_combat"),
        TEXT("Start Combat"),
        TEXT("StartCombatForActiveEvent"),
        TEXT("actions"),
        TEXT("アクティブな battle event の戦闘を開始する。"),
        TEXT("アクティブイベントが battle で、まだ combat 中ではないとき"),
        TEXT("Button"),
        70);
    AddAction(
        TEXT("run_enemy_turn"),
        TEXT("Enemy Turn"),
        TEXT("RunEnemyTurn"),
        TEXT("actions"),
        TEXT("現在の combat state を一手進める。"),
        TEXT("combat 中のとき"),
        TEXT("Button"),
        80);
    AddAction(
        TEXT("grant_developer_card"),
        TEXT("Grant Card"),
        TEXT("ExecuteHudActionById"),
        TEXT("actions"),
        TEXT("詰まった箇所を飛ばすために開発用カードを追加する。"),
        TEXT("セッション初期化後。OptionalNameArgument 未指定なら DefaultDeveloperGrantCardId を使う"),
        TEXT("Button"),
        90);
    AddAction(
        TEXT("build_ai_request"),
        TEXT("Build AI Request"),
        TEXT("ExecuteHudActionById"),
        TEXT("ai_debug"),
        TEXT("AI GM へ送る入力を確認する。"),
        TEXT("アクティブイベントがあるとき"),
        TEXT("Button"),
        100,
        false);
    AddAction(
        TEXT("build_fallback_ai"),
        TEXT("Fallback AI"),
        TEXT("ExecuteHudActionById"),
        TEXT("ai_debug"),
        TEXT("AI 失敗時の固定文面を確認する。"),
        TEXT("アクティブイベントがあるとき"),
        TEXT("Button"),
        110,
        false);
    AddAction(
        TEXT("apply_last_ai_response"),
        TEXT("Apply AI"),
        TEXT("ExecuteHudActionById"),
        TEXT("ai_debug"),
        TEXT("直前に生成または入力した AI GM 応答をそのままセッションへ反映する。"),
        TEXT("LastAiResponse があり、validation が通っているとき"),
        TEXT("Button"),
        120,
        false);
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
        TEXT("RecommendedHudChecklist / RecommendedWalkthrough / WalkthroughProgress / VerificationChecks / EventActorStatusViewData / LastBootstrapDiagnostics を表示し、bAutoRefreshPresentation を有効のまま使う"));
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

void UGreeislandDebugHudWidget::BuildHudActionStates()
{
    HudActionStates.Reset();

    for (const FGreeislandHudActionDefinition& Action : RecommendedHudActions)
    {
        FGreeislandHudActionState State;
        State.ActionId = Action.ActionId;

        if (Action.ActionId == TEXT("bootstrap_session"))
        {
            State.bEnabled = FindBootstrapActor() != nullptr;
        }
        else if (Action.ActionId == TEXT("initialize_new_session"))
        {
            State.bEnabled = true;
        }
        else if (Action.ActionId == TEXT("restore_session"))
        {
            State.bEnabled = LastBootstrapDiagnostics.bSaveExists;
        }
        else if (Action.ActionId == TEXT("save_session"))
        {
            State.bEnabled = CurrentSnapshot.bHasInitializedSession;
        }
        else if (Action.ActionId == TEXT("interact_focused_event"))
        {
            State.bEnabled = bHasFocusedEvent;
        }
        else if (Action.ActionId == TEXT("resolve_active_event"))
        {
            const FGreeislandEventViewData* Event = FindEventViewDataById(CurrentSnapshot.ActiveEventId);
            State.bEnabled = CurrentSnapshot.bHasInitializedSession &&
                Event != nullptr &&
                Event->Type != EExplorationEventType::Battle;
        }
        else if (Action.ActionId == TEXT("start_active_combat"))
        {
            const FGreeislandEventViewData* Event = FindEventViewDataById(CurrentSnapshot.ActiveEventId);
            State.bEnabled = CurrentSnapshot.bHasInitializedSession &&
                !CurrentSnapshot.bCombatActive &&
                Event != nullptr &&
                Event->Type == EExplorationEventType::Battle;
        }
        else if (Action.ActionId == TEXT("run_enemy_turn"))
        {
            State.bEnabled = CurrentSnapshot.bCombatActive;
        }
        else if (Action.ActionId == TEXT("grant_developer_card"))
        {
            State.bEnabled = CurrentSnapshot.bHasInitializedSession;
        }
        else if (Action.ActionId == TEXT("build_ai_request") || Action.ActionId == TEXT("build_fallback_ai"))
        {
            State.bEnabled = CurrentSnapshot.bHasInitializedSession && !CurrentSnapshot.ActiveEventId.IsNone();
        }
        else if (Action.ActionId == TEXT("apply_last_ai_response"))
        {
            State.bEnabled = CurrentSnapshot.bHasInitializedSession &&
                bHasLastAiResponse &&
                LastAiValidationResult.bIsValid;
        }

        State.AvailabilityLabel = State.bEnabled ? TEXT("Enabled") : TEXT("Disabled");
        State.Detail = BuildHudActionDetail(Action);
        HudActionStates.Add(State);
    }
}

void UGreeislandDebugHudWidget::BuildHudActionButtons()
{
    HudActionButtons.Reset();

    for (const FGreeislandHudActionDefinition& Action : RecommendedHudActions)
    {
        FGreeislandHudActionButtonViewData Button;
        Button.ActionId = Action.ActionId;
        Button.Label = Action.Label;
        Button.PanelId = Action.PanelId;
        Button.Purpose = Action.Purpose;
        Button.EnableWhen = Action.EnableWhen;
        Button.InputHint = Action.InputHint;
        Button.Priority = Action.Priority;
        Button.bRecommendedForMinimalLoop = Action.bRecommendedForMinimalLoop;
        Button.bDefaultFlag = true;

        if (const FGreeislandHudActionState* State = FindHudActionStateById(Action.ActionId))
        {
            Button.bEnabled = State->bEnabled;
            Button.AvailabilityLabel = State->AvailabilityLabel;
            Button.Detail = State->Detail;
        }
        else
        {
            Button.bEnabled = false;
            Button.AvailabilityLabel = TEXT("Disabled");
            Button.Detail = Action.EnableWhen;
        }

        if (Action.ActionId == TEXT("grant_developer_card"))
        {
            Button.DefaultNameArgument = DefaultDeveloperGrantCardId;
            Button.bDefaultFlag = true;
        }
        else if (Action.ActionId == TEXT("build_ai_request") || Action.ActionId == TEXT("build_fallback_ai"))
        {
            Button.DefaultStringArgument = TEXT("inspect rewards");
        }
        else if (Action.ActionId == TEXT("apply_last_ai_response"))
        {
            Button.DefaultStringArgument = LastAiPlayerChoice.IsEmpty() ? TEXT("inspect rewards") : LastAiPlayerChoice;
        }

        HudActionButtons.Add(Button);
    }
}

void UGreeislandDebugHudWidget::BuildCurrentObjectiveAction()
{
    CurrentObjectiveAction = FGreeislandCurrentObjectiveActionViewData();

    const FGreeislandWalkthroughStepState* CurrentStep = nullptr;
    for (const FGreeislandWalkthroughStepState& Step : WalkthroughProgress)
    {
        if (Step.bCurrentFocus)
        {
            CurrentStep = &Step;
            break;
        }
    }

    if (!CurrentStep)
    {
        return;
    }

    CurrentObjectiveAction.StepOrder = CurrentStep->Order;
    CurrentObjectiveAction.StepLabel = CurrentStep->Label;
    CurrentObjectiveAction.StepStatusLabel = CurrentStep->StatusLabel;
    CurrentObjectiveAction.Detail = CurrentStep->Detail;

    auto ApplyButton =
        [this](FGreeislandCurrentObjectiveActionViewData& Objective, const FString& ActionId)
    {
        if (const FGreeislandHudActionButtonViewData* Button = FindHudActionButtonById(ActionId))
        {
            Objective.ActionId = Button->ActionId;
            Objective.ActionLabel = Button->Label;
            Objective.AvailabilityLabel = Button->AvailabilityLabel;
            Objective.Detail = FString::Printf(TEXT("%s | %s"), *Objective.Detail, *Button->Detail);
            Objective.DefaultNameArgument = Button->DefaultNameArgument;
            Objective.DefaultStringArgument = Button->DefaultStringArgument;
            Objective.bDefaultFlag = Button->bDefaultFlag;
            Objective.bHasAction = true;
            Objective.bEnabled = Button->bEnabled;
            return true;
        }

        return false;
    };

    auto ApplyPlayCard =
        [this](FGreeislandCurrentObjectiveActionViewData& Objective, const FGreeislandCardViewData& Card)
    {
        Objective.ActionId = TEXT("play_combat_card");
        Objective.ActionLabel = FString::Printf(TEXT("Play %s"), *Card.DisplayName.ToString());
        Objective.AvailabilityLabel = Card.bPlayableNow ? TEXT("Enabled") : TEXT("Disabled");
        Objective.Detail = FString::Printf(TEXT("%s | %s | %s"), *Objective.Detail, *Card.StateSummary, *Card.DetailSummary);
        Objective.DefaultNameArgument = Card.CardId;
        Objective.bDefaultFlag = true;
        Objective.bHasAction = true;
        Objective.bEnabled = Card.bPlayableNow;
    };

    if (CurrentStep->Order == 1)
    {
        if (!ApplyButton(CurrentObjectiveAction, TEXT("bootstrap_session")))
        {
            ApplyButton(CurrentObjectiveAction, TEXT("initialize_new_session"));
        }
        else if (!CurrentObjectiveAction.bEnabled)
        {
            ApplyButton(CurrentObjectiveAction, TEXT("initialize_new_session"));
        }
        return;
    }

    if (CurrentStep->Order == 7)
    {
        if (!LastBootstrapDiagnostics.bSaveExists)
        {
            ApplyButton(CurrentObjectiveAction, TEXT("save_session"));
        }
        else
        {
            ApplyButton(CurrentObjectiveAction, TEXT("restore_session"));
        }
        return;
    }

    if (CurrentStep->Order == 5 && CurrentSnapshot.bCombatActive && CurrentSnapshot.ActiveEventId == CurrentStep->EventId)
    {
        for (const FGreeislandCardViewData& Card : HandCardViewData)
        {
            if (Card.bHasPrimaryAction && Card.bPrimaryActionEnabled)
            {
                ApplyPlayCard(CurrentObjectiveAction, Card);
                return;
            }
        }

        ApplyButton(CurrentObjectiveAction, TEXT("run_enemy_turn"));
        return;
    }

    if (bHasFocusedEvent && FocusedEventId == CurrentStep->EventId)
    {
        ApplyButton(CurrentObjectiveAction, TEXT("interact_focused_event"));
        return;
    }

    if (CurrentSnapshot.ActiveEventId == CurrentStep->EventId)
    {
        const FGreeislandEventViewData* Event = FindEventViewDataById(CurrentStep->EventId);
        if (Event && Event->bHasPrimaryAction)
        {
            CurrentObjectiveAction.ActionId = Event->PrimaryActionId;
            CurrentObjectiveAction.ActionLabel = Event->PrimaryActionLabel;
            CurrentObjectiveAction.AvailabilityLabel = Event->bPrimaryActionEnabled ? TEXT("Enabled") : TEXT("Disabled");
            CurrentObjectiveAction.Detail = FString::Printf(TEXT("%s | %s | %s"), *CurrentObjectiveAction.Detail, *Event->StatusSummary, *Event->DetailSummary);
            CurrentObjectiveAction.DefaultNameArgument = Event->PrimaryActionNameArgument;
            CurrentObjectiveAction.bDefaultFlag = true;
            CurrentObjectiveAction.bHasAction = true;
            CurrentObjectiveAction.bEnabled = Event->bPrimaryActionEnabled;
            return;
        }
    }

    ApplyButton(CurrentObjectiveAction, TEXT("interact_focused_event"));
}

void UGreeislandDebugHudWidget::BuildVerificationChecks()
{
    VerificationChecks.Reset();

    auto AddCheck =
        [this](
            const FString& CheckId,
            const FString& Category,
            const FString& Label,
            bool bPassed,
            const FString& Detail,
            bool bRequired = true)
    {
        FGreeislandVerificationCheckItem Item;
        Item.CheckId = CheckId;
        Item.Category = Category;
        Item.Label = Label;
        Item.bPassed = bPassed;
        Item.StatusLabel = bPassed ? TEXT("Pass") : TEXT("Needs Work");
        Item.Detail = Detail;
        Item.bRequiredForMinimalLoop = bRequired;
        VerificationChecks.Add(Item);
    };

    AddCheck(
        TEXT("session_initialized"),
        TEXT("Bootstrap"),
        TEXT("Session initialized"),
        CurrentSnapshot.bHasInitializedSession,
        CurrentSnapshot.bHasInitializedSession
            ? TEXT("Bootstrap または new session が成立している。")
            : TEXT("まず BootstrapSessionFromActor か InitializeNewSession が必要。"));

    AddCheck(
        TEXT("bootstrap_issues_clear"),
        TEXT("Bootstrap"),
        TEXT("Bootstrap issues clear"),
        LastBootstrapDiagnostics.Issues.Num() == 0,
        LastBootstrapDiagnostics.Issues.Num() == 0
            ? TEXT("JSON path / subsystem / save mode の診断エラーなし。")
            : FString::Join(LastBootstrapDiagnostics.Issues, TEXT(" | ")));

    AddCheck(
        TEXT("event_placements_complete"),
        TEXT("Level"),
        TEXT("All expected EventActors are placed"),
        CountMissingActorPlacements() == 0,
        CountMissingActorPlacements() == 0
            ? TEXT("必要 EventId はすべてレベルに存在する。")
            : FString::Printf(TEXT("Missing placements: %d"), CountMissingActorPlacements()));

    AddCheck(
        TEXT("event_placements_unique"),
        TEXT("Level"),
        TEXT("EventActor ids are unique"),
        CountDuplicateActorPlacements() == 0,
        CountDuplicateActorPlacements() == 0
            ? TEXT("重複 EventId はない。")
            : FString::Printf(TEXT("Duplicate placements: %d"), CountDuplicateActorPlacements()));

    AddCheck(
        TEXT("walkthrough_has_focus"),
        TEXT("Flow"),
        TEXT("Walkthrough has a next actionable step"),
        WalkthroughProgress.Num() == 0 || Algo::AnyOf(
            WalkthroughProgress,
            [](const FGreeislandWalkthroughStepState& Step) { return Step.bCurrentFocus || Step.bCompleted; }),
        WalkthroughProgress.Num() == 0
            ? TEXT("WalkthroughProgress は次の RefreshPresentation で埋まる。")
            : TEXT("進行中または完了済みのステップが追跡できている。"));

    AddCheck(
        TEXT("interactable_event_visible"),
        TEXT("World"),
        TEXT("At least one interactable event can be surfaced"),
        CountInteractableEventActors() > 0 || !CurrentSnapshot.bHasInitializedSession,
        CountInteractableEventActors() > 0
            ? FString::Printf(TEXT("Interactable events in status list: %d"), CountInteractableEventActors())
            : TEXT("プレイヤーをイベント地点へ近づけると Interact Focused が有効になる。"));

    AddCheck(
        TEXT("save_path_ready"),
        TEXT("Persistence"),
        TEXT("Save/restore path is ready"),
        CurrentSnapshot.bHasInitializedSession,
        LastBootstrapDiagnostics.bSaveExists
            ? TEXT("既存 save slot がある。Restore で検証できる。")
            : TEXT("SaveSession 実行後に RestoreSession を確認する。"));

    AddCheck(
        TEXT("zone_clear_verified"),
        TEXT("MVP"),
        TEXT("Zone clear reached"),
        CurrentSnapshot.bZoneCleared,
        CurrentSnapshot.bZoneCleared
            ? TEXT("key_zone_core_001 取得済みで zone clear が立っている。")
            : TEXT("WalkthroughProgress を進めて final gate まで到達する。"));

    AddCheck(
        TEXT("ai_debug_ready"),
        TEXT("AI"),
        TEXT("AI debug actions are available"),
        CurrentSnapshot.bHasInitializedSession && !CurrentSnapshot.ActiveEventId.IsNone(),
        CurrentSnapshot.ActiveEventId.IsNone()
            ? TEXT("アクティブイベントがないと AI request は組めない。")
            : TEXT("Build AI Request / Fallback AI を試せる。"),
        false);
}

void UGreeislandDebugHudWidget::BuildSessionStatusRows()
{
    SessionStatusRows.Reset();

    auto AddRow =
        [this](
            const FString& RowId,
            const FString& Label,
            const FString& Value,
            bool bHealthy,
            const FString& Detail,
            const FString& HealthyLabel = TEXT("Healthy"),
            const FString& UnhealthyLabel = TEXT("Needs Work"))
    {
        FGreeislandSessionStatusRow Row;
        Row.RowId = RowId;
        Row.Label = Label;
        Row.Value = Value;
        Row.bHealthy = bHealthy;
        Row.StatusLabel = bHealthy ? HealthyLabel : UnhealthyLabel;
        Row.Detail = Detail;
        SessionStatusRows.Add(Row);
    };

    const int32 MissingPlacements = CountMissingActorPlacements();
    const int32 DuplicatePlacements = CountDuplicateActorPlacements();
    const int32 InteractableEvents = CountInteractableEventActors();
    const int32 PlayableCardCount = PlayableCombatCardIds.Num();
    const FString ZoneLabel = CurrentSnapshot.ZoneId.IsNone()
        ? TEXT("No Zone")
        : CurrentSnapshot.ZoneId.ToString();

    AddRow(
        TEXT("session"),
        TEXT("Session"),
        CurrentSnapshot.bHasInitializedSession ? TEXT("Initialized") : TEXT("Not Initialized"),
        CurrentSnapshot.bHasInitializedSession,
        CurrentSnapshot.bHasInitializedSession
            ? FString::Printf(
                TEXT("%s | Available %d | Owned %d | Completed %d"),
                *ZoneLabel,
                CurrentSnapshot.AvailableEventIds.Num(),
                CurrentSnapshot.OwnedCardIds.Num(),
                CurrentSnapshot.CompletedQuestIds.Num())
            : TEXT("BootstrapSessionFromActor または InitializeNewSession を実行する。"),
        TEXT("Ready"));

    AddRow(
        TEXT("bootstrap"),
        TEXT("Bootstrap"),
        LastBootstrapDiagnostics.Issues.Num() == 0
            ? TEXT("Diagnostics Clean")
            : FString::Printf(TEXT("%d Issues"), LastBootstrapDiagnostics.Issues.Num()),
        LastBootstrapDiagnostics.Issues.Num() == 0,
        LastBootstrapDiagnostics.Issues.Num() == 0
            ? FString::Printf(
                TEXT("Save=%s | Missing=%d | Duplicate=%d"),
                LastBootstrapDiagnostics.bSaveExists ? TEXT("Present") : TEXT("None"),
                MissingPlacements,
                DuplicatePlacements)
            : FString::Join(LastBootstrapDiagnostics.Issues, TEXT(" | ")),
        TEXT("Clean"));

    AddRow(
        TEXT("active_event"),
        TEXT("Active Event"),
        CurrentSnapshot.ActiveEventId.IsNone()
            ? TEXT("No Active Event")
            : CurrentSnapshot.ActiveEventDisplayName.ToString(),
        !CurrentSnapshot.ActiveEventId.IsNone(),
        CurrentSnapshot.ActiveEventId.IsNone()
            ? TEXT("探索地点へ移動するとアクティブイベントが切り替わる。")
            : FString::Printf(
                TEXT("%s | Combat=%s | Interactable=%d"),
                *CurrentSnapshot.ActiveEventId.ToString(),
                CurrentSnapshot.bCombatActive ? TEXT("On") : TEXT("Off"),
                InteractableEvents),
        TEXT("Live"),
        TEXT("Idle"));

    AddRow(
        TEXT("focus"),
        TEXT("Focused Event"),
        bHasFocusedEvent ? FocusedEventDisplayName.ToString() : TEXT("No Focused Event"),
        bHasFocusedEvent,
        bHasFocusedEvent
            ? FString::Printf(TEXT("%s | Press E or use Interact Focused"), *FocusedEventId.ToString())
            : TEXT("イベント地点に近づくと近接フォーカスが入る。"),
        TEXT("In Range"),
        TEXT("Out Of Range"));

    const FGreeislandWalkthroughStepState* NextStep = nullptr;
    for (const FGreeislandWalkthroughStepState& Step : WalkthroughProgress)
    {
        if (Step.bCurrentFocus)
        {
            NextStep = &Step;
            break;
        }
    }

    const bool bWalkthroughHealthy = WalkthroughProgress.Num() > 0;
    AddRow(
        TEXT("walkthrough"),
        TEXT("Walkthrough"),
        NextStep != nullptr
            ? FString::Printf(TEXT("%d. %s"), NextStep->Order, *NextStep->Label)
            : (CurrentSnapshot.bZoneCleared ? TEXT("Zone Clear Reached") : TEXT("No Walkthrough State")),
        bWalkthroughHealthy,
        NextStep != nullptr
            ? FString::Printf(TEXT("%s | %s"), *NextStep->StatusLabel, *NextStep->Detail)
            : (CurrentSnapshot.bZoneCleared
                ? TEXT("Final gate clear 済み。Save/Restore 検証へ進める。")
                : TEXT("RefreshPresentation 後に進捗行が埋まる。")),
        TEXT("Tracked"),
        TEXT("Pending"));

    AddRow(
        TEXT("combat"),
        TEXT("Combat"),
        CurrentSnapshot.bCombatActive
            ? FString::Printf(
                TEXT("HP %d / Enemy %d / Energy %d"),
                CurrentSnapshot.PlayerHp,
                CurrentSnapshot.EnemyHp,
                CurrentSnapshot.Energy)
            : TEXT("Exploration"),
        !CurrentSnapshot.bCombatActive || CurrentSnapshot.PlayerHp > 0,
        CurrentSnapshot.bCombatActive
            ? FString::Printf(
                TEXT("Hand %d | Playable %d"),
                HandCardViewData.Num(),
                PlayableCardCount)
            : FString::Printf(
                TEXT("Available Events %d | Interactable %d"),
                CurrentSnapshot.AvailableEventIds.Num(),
                InteractableEvents),
        TEXT("Stable"),
        TEXT("At Risk"));

    AddRow(
        TEXT("progression"),
        TEXT("Progression"),
        CurrentSnapshot.bZoneCleared ? TEXT("Zone Cleared") : TEXT("In Progress"),
        CurrentSnapshot.bZoneCleared,
        FString::Printf(
            TEXT("Completed Quests %d | Deck %d | Hand %d"),
            CurrentSnapshot.CompletedQuestIds.Num(),
            CurrentSnapshot.DeckCardIds.Num(),
            CurrentSnapshot.HandCardIds.Num()),
        TEXT("Complete"),
        TEXT("Tracking"));
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

const FGreeislandHudActionState* UGreeislandDebugHudWidget::FindHudActionStateById(const FString& ActionId) const
{
    for (const FGreeislandHudActionState& State : HudActionStates)
    {
        if (State.ActionId == ActionId)
        {
            return &State;
        }
    }

    return nullptr;
}

const FGreeislandHudActionButtonViewData* UGreeislandDebugHudWidget::FindHudActionButtonById(
    const FString& ActionId) const
{
    for (const FGreeislandHudActionButtonViewData& Button : HudActionButtons)
    {
        if (Button.ActionId == ActionId)
        {
            return &Button;
        }
    }

    return nullptr;
}

FString UGreeislandDebugHudWidget::BuildHudActionDetail(const FGreeislandHudActionDefinition& Action) const
{
    if (Action.ActionId == TEXT("restore_session") && !LastBootstrapDiagnostics.bSaveExists)
    {
        return TEXT("Save slot がまだ無いので Restore は待機。");
    }

    if (Action.ActionId == TEXT("interact_focused_event") && !bHasFocusedEvent)
    {
        return TEXT("イベント地点に近づくと有効になる。");
    }

    if (Action.ActionId == TEXT("resolve_active_event") || Action.ActionId == TEXT("start_active_combat"))
    {
        if (const FGreeislandEventViewData* Event = FindEventViewDataById(CurrentSnapshot.ActiveEventId))
        {
            return FString::Printf(
                TEXT("Active=%s (%s)"),
                *Event->EventId.ToString(),
                Event->Type == EExplorationEventType::Battle ? TEXT("Battle") : TEXT("NonBattle"));
        }
    }

    if (Action.ActionId == TEXT("run_enemy_turn") && !CurrentSnapshot.bCombatActive)
    {
        return TEXT("Combat 開始後に有効になる。");
    }

    return Action.EnableWhen;
}

int32 UGreeislandDebugHudWidget::CountMissingActorPlacements() const
{
    int32 Count = 0;
    for (const FGreeislandEventActorStatusViewData& Status : EventActorStatusViewData)
    {
        if (!Status.bHasActorPlacement)
        {
            ++Count;
        }
    }
    return Count;
}

int32 UGreeislandDebugHudWidget::CountDuplicateActorPlacements() const
{
    int32 Count = 0;
    for (const FGreeislandEventActorStatusViewData& Status : EventActorStatusViewData)
    {
        if (Status.PlacementCount > 1)
        {
            ++Count;
        }
    }
    return Count;
}

int32 UGreeislandDebugHudWidget::CountInteractableEventActors() const
{
    int32 Count = 0;
    for (const FGreeislandEventActorStatusViewData& Status : EventActorStatusViewData)
    {
        if (Status.bIsInteractableNow)
        {
            ++Count;
        }
    }
    return Count;
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
