#include "Actors/GreeislandEventActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
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

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    MarkerMesh->SetupAttachment(SceneRoot);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MarkerMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.1f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMeshAsset(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (MarkerMeshAsset.Succeeded())
    {
        MarkerMesh->SetStaticMesh(MarkerMeshAsset.Object);
    }

    MarkerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerLabel"));
    MarkerLabel->SetupAttachment(SceneRoot);
    MarkerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
    MarkerLabel->SetHorizontalAlignment(EHTA_Center);
    MarkerLabel->SetVerticalAlignment(EVRTA_TextCenter);
    MarkerLabel->SetWorldSize(32.0f);
    MarkerLabel->SetText(FText::FromString(TEXT("Event")));
    MarkerLabel->SetTextRenderColor(FColor(230, 245, 255, 255));
}

void AGreeislandEventActor::SetEventId(FName NewEventId)
{
    EventId = NewEventId;
#if WITH_EDITOR
    SetActorLabel(EventId.IsNone() ? TEXT("Greeisland Event") : EventId.ToString());
#endif
    if (MarkerLabel)
    {
        MarkerLabel->SetText(FText::FromName(EventId));
    }

    if (!bAutoTriggerOnOverlap || !InteractionSphere || !IsEventAvailable())
    {
        return;
    }

    TArray<AActor*> OverlappingActors;
    InteractionSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
    if (OverlappingActors.Num() > 0)
    {
        TriggerEvent();
    }
}

void AGreeislandEventActor::SetAutoTriggerOnOverlap(bool bNewAutoTriggerOnOverlap)
{
    bAutoTriggerOnOverlap = bNewAutoTriggerOnOverlap;

    if (!bAutoTriggerOnOverlap || !InteractionSphere || !IsEventAvailable())
    {
        return;
    }

    TArray<AActor*> OverlappingActors;
    InteractionSphere->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
    if (OverlappingActors.Num() > 0)
    {
        UE_LOG(
            LogTemp,
            Display,
            TEXT("[Greeisland][EVENT_CONTACT] EventActor %s enabled auto-trigger while overlapping"),
            *EventId.ToString());
        TriggerEvent();
    }
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

    TArray<FGreeislandEventViewData> Events;
    Subsystem->BuildEventViewData(Events);
    for (const FGreeislandEventViewData& Event : Events)
    {
        if (Event.EventId == EventId)
        {
            return Event.bAvailable && !Event.bCompleted;
        }
    }

    return false;
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
        UE_LOG(
            LogTemp,
            Display,
            TEXT("[Greeisland][EVENT_CONTACT] EventActor %s overlapped by %s"),
            *EventId.ToString(),
            *OtherActor->GetName());
        TriggerEvent();
    }
}
