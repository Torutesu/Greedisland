#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandEventActor.generated.h"

class UGreeislandGameSubsystem;

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandEventActor : public AActor
{
    GENERATED_BODY()

public:
    AGreeislandEventActor();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Event")
    FSessionActionResult TriggerEvent();

    UFUNCTION(BlueprintPure, Category = "Greeisland|Event")
    bool GetBoundEventDefinition(FExplorationEventDefinition& OutEvent) const;

    UFUNCTION(BlueprintPure, Category = "Greeisland|Event")
    UGreeislandGameSubsystem* GetGreeislandSubsystem() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|Event")
    void OnEventTriggered(const FSessionActionResult& ActionResult);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    FName EventId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    int32 CombatOpeningDrawCount = 5;
};

