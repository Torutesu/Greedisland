#include "Actors/GreeislandBootstrapActor.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Runtime/GreeislandGameSubsystem.h"

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

    FSessionActionResult Result;
    switch (BootstrapMode)
    {
        case EBootstrapMode::InitializeNew:
            Result = Subsystem->InitializeNewSessionFromFiles(CardJsonPath, EventJsonPath);
            break;

        case EBootstrapMode::RestoreIfPossible:
            if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
            {
                Result = Subsystem->RestoreSessionFromSaveSlot(
                    SaveSlotName,
                    SaveUserIndex,
                    CardJsonPath,
                    EventJsonPath);
            }
            else
            {
                Result = Subsystem->InitializeNewSessionFromFiles(CardJsonPath, EventJsonPath);
            }
            break;

        case EBootstrapMode::RestoreOnly:
            Result = Subsystem->RestoreSessionFromSaveSlot(
                SaveSlotName,
                SaveUserIndex,
                CardJsonPath,
                EventJsonPath);
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

