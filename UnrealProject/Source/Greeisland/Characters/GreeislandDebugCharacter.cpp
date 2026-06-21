#include "Characters/GreeislandDebugCharacter.h"

#include "Actors/GreeislandEventActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

AGreeislandDebugCharacter::AGreeislandDebugCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 480.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

void AGreeislandDebugCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AGreeislandDebugCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AGreeislandDebugCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AGreeislandDebugCharacter::TurnAtRate);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AGreeislandDebugCharacter::LookUpAtRate);
    PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AGreeislandDebugCharacter::Interact);
}

void AGreeislandDebugCharacter::Interact()
{
    AGreeislandEventActor* EventActor = FindBestInteractableEventActor();
    if (EventActor)
    {
        EventActor->TriggerEvent();
    }
}

AGreeislandEventActor* AGreeislandDebugCharacter::FindBestInteractableEventActor() const
{
    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGreeislandEventActor::StaticClass(), Actors);

    AGreeislandEventActor* BestActor = nullptr;
    float BestDistanceSq = TNumericLimits<float>::Max();

    for (AActor* Actor : Actors)
    {
        AGreeislandEventActor* EventActor = Cast<AGreeislandEventActor>(Actor);
        if (!EventActor || !EventActor->IsEventAvailable())
        {
            continue;
        }

        const float MaxDistance = FMath::Min(InteractionSearchRadius, EventActor->GetInteractionRadius());
        const float DistanceSq = FVector::DistSquared(EventActor->GetActorLocation(), GetActorLocation());
        if (DistanceSq <= FMath::Square(MaxDistance) && DistanceSq < BestDistanceSq)
        {
            BestDistanceSq = DistanceSq;
            BestActor = EventActor;
        }
    }

    return BestActor;
}

void AGreeislandDebugCharacter::MoveForward(float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    AddMovementInput(Direction, Value);
}

void AGreeislandDebugCharacter::MoveRight(float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, Rotation.Yaw, 0.0f);
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    AddMovementInput(Direction, Value);
}

void AGreeislandDebugCharacter::TurnAtRate(float Value)
{
    AddControllerYawInput(Value);
}

void AGreeislandDebugCharacter::LookUpAtRate(float Value)
{
    AddControllerPitchInput(Value);
}
