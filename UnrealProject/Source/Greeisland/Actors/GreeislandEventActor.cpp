#include "Actors/GreeislandEventActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Runtime/GreeislandGameSubsystem.h"

AGreeislandEventActor::AGreeislandEventActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(SceneRoot);
    InteractionSphere->InitSphereRadius(InteractionRadius);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    InteractionSphere->SetGenerateOverlapEvents(true);
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AGreeislandEventActor::HandleInteractionSphereBeginOverlap);
}

void AGreeislandEventActor::SetEventId(FName NewEventId)
{
    EventId = NewEventId;
    SetActorLabel(EventId.IsNone() ? TEXT("Greeisland Event") : EventId.ToString());
}

FSessionActionResult AGreeislandEventActor::TriggerEvent()
{
    UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        FSessionActionResult Result;
        Result.Reasons.Add(TEXT("GreeislandGameSubsystem is unavailable."));
        OnEventTriggered(Result);
        return Result;
    }

    FSessionActionResult Result = Subsystem->ResolveOrStartEvent(EventId, CombatOpeningDrawCount);
    OnEventTriggered(Result);
    return Result;
}

bool AGreeislandEventActor::GetBoundEventDefinition(FExplorationEventDefinition& OutEvent) const
{
    const UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem)
    {
        return false;
    }

    return Subsystem->GetEventDefinition(EventId, OutEvent);
}

UGreeislandGameSubsystem* AGreeislandEventActor::GetGreeislandSubsystem() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UGreeislandGameSubsystem>();
}

bool AGreeislandEventActor::IsEventAvailable() const
{
    const UGreeislandGameSubsystem* Subsystem = GetGreeislandSubsystem();
    if (!Subsystem || !Subsystem->HasInitializedSession())
    {
        return false;
    }

    const FGreeislandUiSnapshot Snapshot = Subsystem->BuildUiSnapshot();
    return Snapshot.AvailableEventIds.Contains(EventId);
}

float AGreeislandEventActor::GetInteractionRadius() const
{
    return InteractionSphere ? InteractionSphere->GetScaledSphereRadius() : InteractionRadius;
}

void AGreeislandEventActor::HandleInteractionSphereBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    (void)OverlappedComponent;
    (void)OtherComp;
    (void)OtherBodyIndex;
    (void)bFromSweep;
    (void)SweepResult;

    if (!bAutoTriggerOnOverlap)
    {
        return;
    }

    if (OtherActor && OtherActor->IsA<APawn>() && IsEventAvailable())
    {
        TriggerEvent();
    }
}
