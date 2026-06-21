#include "Actors/GreeislandEventActor.h"

#include "Engine/GameInstance.h"
#include "Runtime/GreeislandGameSubsystem.h"

AGreeislandEventActor::AGreeislandEventActor()
{
    PrimaryActorTick.bCanEverTick = false;
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

