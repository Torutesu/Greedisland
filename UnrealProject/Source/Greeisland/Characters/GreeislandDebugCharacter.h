#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GreeislandDebugCharacter.generated.h"

class AGreeislandEventActor;
class UCameraComponent;
class USpringArmComponent;

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandDebugCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AGreeislandDebugCharacter();

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Character")
    void Interact();

    UFUNCTION(BlueprintPure, Category = "Greeisland|Character")
    AGreeislandEventActor* FindBestInteractableEventActor() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greeisland|Character")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greeisland|Character")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Character", meta=(ClampMin="100.0"))
    float InteractionSearchRadius = 240.0f;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void TurnAtRate(float Value);
    void LookUpAtRate(float Value);
};
