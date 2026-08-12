#include "GameFramework/GreeislandDebugGameMode.h"

#include "Actors/GreeislandBootstrapActor.h"
#include "Actors/GreeislandEventActor.h"
#include "Characters/GreeislandDebugCharacter.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/GreeislandDebugPlayerController.h"
#include "Kismet/GameplayStatics.h"
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
        Ground->SetActorLabel(TEXT("MVP Zone Ground"));
    }
}
