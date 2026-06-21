#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandEventActor.generated.h"

class UGreeislandGameSubsystem;
class UPrimitiveComponent;
class USceneComponent;
class USphereComponent;

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

    UFUNCTION(BlueprintPure, Category = "Greeisland|Event")
    bool IsEventAvailable() const;

    UFUNCTION(BlueprintPure, Category = "Greeisland|Event")
    float GetInteractionRadius() const;

    UFUNCTION(BlueprintPure, Category = "Greeisland|Event")
    FName GetEventId() const
    {
        return EventId;
    }

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    TObjectPtr<USphereComponent> InteractionSphere;

    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|Event")
    void OnEventTriggered(const FSessionActionResult& ActionResult);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    FName EventId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    int32 CombatOpeningDrawCount = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Event")
    bool bAutoTriggerOnOverlap = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Event", meta=(ClampMin="50.0"))
    float InteractionRadius = 180.0f;

    UFUNCTION()
    void HandleInteractionSphereBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);
};
