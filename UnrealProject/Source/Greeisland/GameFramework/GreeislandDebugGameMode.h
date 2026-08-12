#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GreeislandDebugGameMode.generated.h"

class AActor;
class AController;

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandDebugGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGreeislandDebugGameMode();

    virtual void BeginPlay() override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bAutoBuildMvpZone = true;

private:
    void BuildMvpZoneIfNeeded();
    void RunNativeMvpSmokeTestIfRequested();
};
