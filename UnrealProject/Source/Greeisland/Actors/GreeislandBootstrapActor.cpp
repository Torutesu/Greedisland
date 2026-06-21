#include "Actors/GreeislandBootstrapActor.h"

#include "Actors/GreeislandEventActor.h"
#include "HAL/FileManager.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Exploration/ExplorationEventLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GreeislandGameSubsystem.h"
#include "Runtime/GreeislandProjectSettings.h"
#include "Misc/Paths.h"

namespace
{
FString EventTypeToString(EExplorationEventType EventType)
{
    switch (EventType)
    {
        case EExplorationEventType::Battle:
            return TEXT("Battle");
        case EExplorationEventType::Treasure:
            return TEXT("Treasure");
        case EExplorationEventType::Npc:
            return TEXT("Npc");
        case EExplorationEventType::Quest:
            return TEXT("Quest");
        case EExplorationEventType::KeyGate:
            return TEXT("KeyGate");
    }

    return TEXT("Unknown");
}
}

AGreeislandBootstrapActor::AGreeislandBootstrapActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGreeislandBootstrapActor::BeginPlay()
{
    Super::BeginPlay();

    if (bBootstrapOnBeginPlay)
    {
        BootstrapSession();
    }
}

FSessionActionResult AGreeislandBootstrapActor::BootstrapSession()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        FSessionActionResult Result;
        Result.Reasons.Add(TEXT("GreeislandGameSubsystem is unavailable."));
        OnBootstrapFinished(Result);
        return Result;
    }

    FString EffectiveCardJsonPath;
    FString EffectiveEventJsonPath;
    FString EffectiveSaveSlotName;
    int32 EffectiveSaveUserIndex = 0;
    GetEffectiveBootstrapSettings(
        EffectiveCardJsonPath,
        EffectiveEventJsonPath,
        EffectiveSaveSlotName,
        EffectiveSaveUserIndex);

    FSessionActionResult Result;
    switch (BootstrapMode)
    {
        case EBootstrapMode::InitializeNew:
            Result = Subsystem->InitializeNewSessionFromFiles(EffectiveCardJsonPath, EffectiveEventJsonPath);
            break;

        case EBootstrapMode::RestoreIfPossible:
            if (UGameplayStatics::DoesSaveGameExist(EffectiveSaveSlotName, EffectiveSaveUserIndex))
            {
                Result = Subsystem->RestoreSessionFromSaveSlot(
                    EffectiveSaveSlotName,
                    EffectiveSaveUserIndex,
                    EffectiveCardJsonPath,
                    EffectiveEventJsonPath);
            }
            else
            {
                Result = Subsystem->InitializeNewSessionFromFiles(EffectiveCardJsonPath, EffectiveEventJsonPath);
            }
            break;

        case EBootstrapMode::RestoreOnly:
            Result = Subsystem->RestoreSessionFromSaveSlot(
                EffectiveSaveSlotName,
                EffectiveSaveUserIndex,
                EffectiveCardJsonPath,
                EffectiveEventJsonPath);
            break;
    }

    OnBootstrapFinished(Result);
    return Result;
}

UGreeislandGameSubsystem* AGreeislandBootstrapActor::GetGreeislandSubsystem() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UGreeislandGameSubsystem>();
}

FGreeislandBootstrapDiagnostics AGreeislandBootstrapActor::GetBootstrapDiagnostics() const
{
    FGreeislandBootstrapDiagnostics Diagnostics;
    Diagnostics.bUsingProjectSettingsDefaults = bUseProjectSettingsDefaults;

    FString SaveSlotNameValue;
    int32 SaveUserIndexValue = 0;
    GetEffectiveBootstrapSettings(
        Diagnostics.EffectiveCardJsonPath,
        Diagnostics.EffectiveEventJsonPath,
        SaveSlotNameValue,
        SaveUserIndexValue);

    const FString ResolvedCardPath = FPaths::IsRelative(Diagnostics.EffectiveCardJsonPath)
        ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Diagnostics.EffectiveCardJsonPath)
        : Diagnostics.EffectiveCardJsonPath;
    const FString ResolvedEventPath = FPaths::IsRelative(Diagnostics.EffectiveEventJsonPath)
        ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Diagnostics.EffectiveEventJsonPath)
        : Diagnostics.EffectiveEventJsonPath;

    Diagnostics.EffectiveCardJsonPath = ResolvedCardPath;
    Diagnostics.EffectiveEventJsonPath = ResolvedEventPath;
    Diagnostics.bCardJsonExists = IFileManager::Get().FileExists(*ResolvedCardPath);
    Diagnostics.bEventJsonExists = IFileManager::Get().FileExists(*ResolvedEventPath);
    Diagnostics.bSaveExists = UGameplayStatics::DoesSaveGameExist(SaveSlotNameValue, SaveUserIndexValue);

    UWorld* World = GetWorld();
    if (World)
    {
        TMap<FName, int32> SeenEventActorIds;
        for (TActorIterator<AGreeislandBootstrapActor> BootstrapIt(World); BootstrapIt; ++BootstrapIt)
        {
            ++Diagnostics.BootstrapActorCount;
        }

        for (TActorIterator<AGreeislandEventActor> EventIt(World); EventIt; ++EventIt)
        {
            ++Diagnostics.EventActorCount;
            const FName EventId = EventIt->GetEventId();
            if (!EventId.IsNone())
            {
                SeenEventActorIds.FindOrAdd(EventId) += 1;
            }
        }

        FZoneEventSet EventSet;
        TArray<FString> LoadErrors;
        if (Diagnostics.bEventJsonExists &&
            UExplorationEventLibrary::LoadZoneEventsFromJsonFile(Diagnostics.EffectiveEventJsonPath, EventSet, LoadErrors))
        {
            TSet<FName> ExpectedIds;
            for (const FExplorationEventDefinition& Event : EventSet.Events)
            {
                Diagnostics.ExpectedEventIds.Add(Event.EventId);
                ExpectedIds.Add(Event.EventId);

                const int32 Count = SeenEventActorIds.FindRef(Event.EventId);
                FGreeislandExpectedEventPlacement Placement;
                Placement.EventId = Event.EventId;
                Placement.DisplayName = Event.DisplayName;
                Placement.EventType = EventTypeToString(Event.Type);
                Placement.PlacementCount = Count;
                Placement.bIsPlaced = Count > 0;
                Placement.bIsDuplicate = Count > 1;
                Placement.NextEventIds = Event.NextEventIds;
                Diagnostics.ExpectedEventPlacements.Add(Placement);

                if (Count == 0)
                {
                    Diagnostics.MissingEventActorIds.Add(Event.EventId);
                }
                else if (Count > 1)
                {
                    Diagnostics.DuplicateEventActorIds.Add(Event.EventId);
                }
            }

            for (const TPair<FName, int32>& Pair : SeenEventActorIds)
            {
                if (!ExpectedIds.Contains(Pair.Key))
                {
                    Diagnostics.UnexpectedEventActorIds.Add(Pair.Key);
                }
            }
        }
        else
        {
            for (const FString& Error : LoadErrors)
            {
                Diagnostics.Issues.Add(FString::Printf(TEXT("Event JSON diagnostic load failed: %s"), *Error));
            }
        }
    }

    if (!Diagnostics.bCardJsonExists)
    {
        Diagnostics.Issues.Add(FString::Printf(TEXT("Card JSON was not found: %s"), *ResolvedCardPath));
    }

    if (!Diagnostics.bEventJsonExists)
    {
        Diagnostics.Issues.Add(FString::Printf(TEXT("Event JSON was not found: %s"), *ResolvedEventPath));
    }

    if (Diagnostics.BootstrapActorCount > 1)
    {
        Diagnostics.Issues.Add(FString::Printf(
            TEXT("Multiple GreeislandBootstrapActor instances were found: %d"),
            Diagnostics.BootstrapActorCount));
    }

    if (Diagnostics.MissingEventActorIds.Num() > 0)
    {
        Diagnostics.Issues.Add(FString::Printf(
            TEXT("Missing EventActor placements for %d event id(s)."),
            Diagnostics.MissingEventActorIds.Num()));
    }

    if (Diagnostics.DuplicateEventActorIds.Num() > 0)
    {
        Diagnostics.Issues.Add(FString::Printf(
            TEXT("Duplicate EventActor placements found for %d event id(s)."),
            Diagnostics.DuplicateEventActorIds.Num()));
    }

    if (BootstrapMode == EBootstrapMode::RestoreOnly && !Diagnostics.bSaveExists)
    {
        Diagnostics.Issues.Add(FString::Printf(
            TEXT("Save slot %s (%d) does not exist for RestoreOnly mode."),
            *SaveSlotNameValue,
            SaveUserIndexValue));
    }

    if (!GetGreeislandSubsystem())
    {
        Diagnostics.Issues.Add(TEXT("GreeislandGameSubsystem is unavailable in the current world."));
    }

    return Diagnostics;
}

void AGreeislandBootstrapActor::GetEffectiveBootstrapSettings(
    FString& OutCardJsonPath,
    FString& OutEventJsonPath,
    FString& OutSaveSlotName,
    int32& OutSaveUserIndex) const
{
    OutCardJsonPath = CardJsonPath;
    OutEventJsonPath = EventJsonPath;
    OutSaveSlotName = SaveSlotName;
    OutSaveUserIndex = SaveUserIndex;

    if (!bUseProjectSettingsDefaults)
    {
        return;
    }

    const UGreeislandProjectSettings* Settings = GetDefault<UGreeislandProjectSettings>();
    if (!Settings)
    {
        return;
    }

    OutCardJsonPath = Settings->CardJsonPath;
    OutEventJsonPath = Settings->EventJsonPath;
    OutSaveSlotName = Settings->SaveSlotName;
    OutSaveUserIndex = Settings->SaveUserIndex;
}
