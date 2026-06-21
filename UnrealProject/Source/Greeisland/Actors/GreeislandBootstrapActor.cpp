#include "Actors/GreeislandBootstrapActor.h"

#include "HAL/FileManager.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GreeislandGameSubsystem.h"
#include "Runtime/GreeislandProjectSettings.h"
#include "Misc/Paths.h"

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

    if (!Diagnostics.bCardJsonExists)
    {
        Diagnostics.Issues.Add(FString::Printf(TEXT("Card JSON was not found: %s"), *ResolvedCardPath));
    }

    if (!Diagnostics.bEventJsonExists)
    {
        Diagnostics.Issues.Add(FString::Printf(TEXT("Event JSON was not found: %s"), *ResolvedEventPath));
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
