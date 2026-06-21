#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandBootstrapActor.generated.h"

class UGreeislandGameSubsystem;

UENUM(BlueprintType)
enum class EBootstrapMode : uint8
{
    InitializeNew,
    RestoreIfPossible,
    RestoreOnly
};

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandBootstrapActor : public AActor
{
    GENERATED_BODY()

public:
    AGreeislandBootstrapActor();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Bootstrap")
    FSessionActionResult BootstrapSession();

    UFUNCTION(BlueprintPure, Category = "Greeisland|Bootstrap")
    UGreeislandGameSubsystem* GetGreeislandSubsystem() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|Bootstrap")
    void OnBootstrapFinished(const FSessionActionResult& ActionResult);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bBootstrapOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    EBootstrapMode BootstrapMode = EBootstrapMode::RestoreIfPossible;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString CardJsonPath = TEXT("../data/cards/cards.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString EventJsonPath = TEXT("../data/events/events.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString SaveSlotName = TEXT("greeisland-dev-slot");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    int32 SaveUserIndex = 0;
};
