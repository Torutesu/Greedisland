#include "GameFramework/GreeislandDebugGameMode.h"

#include "Actors/GreeislandBootstrapActor.h"
#include "Actors/GreeislandEventActor.h"
#include "Characters/GreeislandDebugCharacter.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GreeislandDebugPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Parse.h"
#include "Runtime/GreeislandGameSubsystem.h"
#include "Runtime/GreeislandProjectSettings.h"
#include "UI/GreeislandDebugHud.h"

namespace
{
const TArray<FName> MvpEventIds =
{
    TEXT("event_wake_cache_001"),
    TEXT("event_contract_broker_001"),
    TEXT("event_silent_shrine_001"),
    TEXT("event_ridge_scout_001"),
    TEXT("event_proxy_gate_001")
};

const TArray<FVector> MvpEventLocations =
{
    FVector(0.0f, 0.0f, 100.0f),
    FVector(550.0f, 0.0f, 100.0f),
    FVector(1050.0f, 260.0f, 100.0f),
    FVector(1050.0f, -260.0f, 100.0f),
    FVector(1600.0f, 0.0f, 100.0f)
};
}

AGreeislandDebugGameMode::AGreeislandDebugGameMode()
{
    DefaultPawnClass = AGreeislandDebugCharacter::StaticClass();
    PlayerControllerClass = AGreeislandDebugPlayerController::StaticClass();
    HUDClass = AGreeislandDebugHud::StaticClass();
}

void AGreeislandDebugGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoBuildMvpZone)
    {
        BuildMvpZoneIfNeeded();
    }

    RunNativeMvpSmokeTestIfRequested();
}

void AGreeislandDebugGameMode::BuildMvpZoneIfNeeded()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AActor*> ExistingBootstrapActors;
    UGameplayStatics::GetAllActorsOfClass(World, AGreeislandBootstrapActor::StaticClass(), ExistingBootstrapActors);
    if (ExistingBootstrapActors.Num() == 0)
    {
        World->SpawnActor<AGreeislandBootstrapActor>(FVector::ZeroVector, FRotator::ZeroRotator);
    }

    TArray<AActor*> ExistingPlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), ExistingPlayerStarts);
    if (ExistingPlayerStarts.Num() == 0)
    {
        APlayerStart* PlayerStart = World->SpawnActor<APlayerStart>(
            FVector(0.0f, -220.0f, 120.0f), FRotator::ZeroRotator);
#if WITH_EDITOR
        if (PlayerStart)
        {
            PlayerStart->SetActorLabel(TEXT("MVP Player Start"));
        }
#endif
    }

    TArray<AActor*> ExistingEventActors;
    UGameplayStatics::GetAllActorsOfClass(World, AGreeislandEventActor::StaticClass(), ExistingEventActors);
    TSet<FName> ExistingEventIds;
    for (AActor* ExistingActor : ExistingEventActors)
    {
        if (const AGreeislandEventActor* EventActor = Cast<AGreeislandEventActor>(ExistingActor))
        {
            ExistingEventIds.Add(EventActor->GetEventId());
        }
    }

    for (int32 Index = 0; Index < MvpEventIds.Num(); ++Index)
    {
        if (ExistingEventIds.Contains(MvpEventIds[Index]))
        {
            continue;
        }

        AGreeislandEventActor* EventActor = World->SpawnActor<AGreeislandEventActor>(
            MvpEventLocations.IsValidIndex(Index) ? MvpEventLocations[Index] : FVector::ZeroVector,
            FRotator::ZeroRotator);
        if (EventActor)
        {
            EventActor->SetEventId(MvpEventIds[Index]);
            EventActor->SetAutoTriggerOnOverlap(true);
            ExistingEventIds.Add(MvpEventIds[Index]);
        }
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        return;
    }

    AStaticMeshActor* Ground = World->SpawnActor<AStaticMeshActor>(
        FVector(800.0f, 0.0f, -50.0f), FRotator::ZeroRotator);
    if (Ground && Ground->GetStaticMeshComponent())
    {
        Ground->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
        Ground->GetStaticMeshComponent()->SetWorldScale3D(FVector(24.0f, 8.0f, 0.5f));
#if WITH_EDITOR
        Ground->SetActorLabel(TEXT("MVP Zone Ground"));
#endif
    }
}

void AGreeislandDebugGameMode::RunNativeMvpSmokeTestIfRequested()
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("GreeislandMvpSmoke")))
    {
        return;
    }

    auto Fail = [](const FString& Message)
    {
        UE_LOG(LogTemp, Error, TEXT("[Greeisland][MVP_SMOKE] FAIL: %s"), *Message);
        FPlatformMisc::RequestExit(false);
    };

    auto Require = [&Fail](const FSessionActionResult& Result, const FString& Step) -> bool
    {
        if (Result.bSuccess)
        {
            UE_LOG(LogTemp, Display, TEXT("[Greeisland][MVP_SMOKE] PASS STEP: %s"), *Step);
            return true;
        }

        Fail(FString::Printf(
            TEXT("%s: %s"),
            *Step,
            Result.Reasons.Num() > 0 ? *FString::Join(Result.Reasons, TEXT(" | ")) : TEXT("no reason")));
        return false;
    };

    UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UGreeislandGameSubsystem* Subsystem = GameInstance
        ? GameInstance->GetSubsystem<UGreeislandGameSubsystem>()
        : nullptr;
    const UGreeislandProjectSettings* Settings = GetDefault<UGreeislandProjectSettings>();
    if (!Subsystem || !Settings)
    {
        Fail(TEXT("Game subsystem or project settings are unavailable."));
        return;
    }

    if (!Require(
        Subsystem->InitializeNewSessionFromFiles(Settings->CardJsonPath, Settings->EventJsonPath),
        TEXT("initialize session")))
    {
        return;
    }
    if (!Require(Subsystem->ResolveEvent(TEXT("event_wake_cache_001")), TEXT("resolve wake cache")))
    {
        return;
    }
    if (!Require(Subsystem->ResolveEvent(TEXT("event_contract_broker_001")), TEXT("resolve contract broker")))
    {
        return;
    }

    FAiGmResponse AiResponse;
    if (!Subsystem->BuildFallbackAiResponseForActiveEvent(TEXT("交渉する"), AiResponse))
    {
        Fail(TEXT("build fallback AI response"));
        return;
    }
    if (!Require(Subsystem->ApplyAiResponse(AiResponse, TEXT("交渉する")), TEXT("apply fallback AI response")))
    {
        return;
    }
    if (!Require(Subsystem->ResolveEvent(TEXT("event_silent_shrine_001")), TEXT("resolve silent shrine")))
    {
        return;
    }
    if (!Require(Subsystem->GrantDeveloperCard(TEXT("act_strike_001")), TEXT("grant second strike for combat")))
    {
        return;
    }
    if (!Require(Subsystem->StartCombat(TEXT("event_ridge_scout_001"), 5), TEXT("start ridge scout combat")))
    {
        return;
    }

    for (int32 Turn = 0; Turn < 12 && Subsystem->GetSession().bCombatActive; ++Turn)
    {
        const FGreeislandGameSession Session = Subsystem->GetSession();
        FName StrikeCardId = NAME_None;
        for (const FName& CardId : Session.CombatState.Hand)
        {
            if (CardId == TEXT("act_strike_001"))
            {
                StrikeCardId = CardId;
                break;
            }
        }

        if (!StrikeCardId.IsNone())
        {
            if (!Require(Subsystem->PlayCombatCard(StrikeCardId), TEXT("play combat card")))
            {
                return;
            }
        }
        else if (!Require(Subsystem->RunEnemyTurn(1), TEXT("run enemy turn")))
        {
            return;
        }
    }

    if (Subsystem->GetSession().bCombatActive)
    {
        Fail(TEXT("combat did not finish within the smoke-test turn limit"));
        return;
    }
    if (!Require(Subsystem->ResolveEvent(TEXT("event_proxy_gate_001")), TEXT("resolve proxy gate")))
    {
        return;
    }
    if (!Require(
        Subsystem->SaveSessionToSlot(TEXT("greeisland-mvp-smoke"), 0, TEXT("mvp-smoke")),
        TEXT("save completed session")))
    {
        return;
    }
    if (!Require(
        Subsystem->RestoreSessionFromSaveSlot(
            TEXT("greeisland-mvp-smoke"),
            0,
            Settings->CardJsonPath,
            Settings->EventJsonPath),
        TEXT("restore completed session")))
    {
        return;
    }

    if (!Subsystem->GetSession().bZoneCleared)
    {
        Fail(TEXT("restored session is not marked as zone cleared"));
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("[Greeisland][MVP_SMOKE] PASS: one-zone MVP completed and restored."));
    FPlatformMisc::RequestExit(false);
}
