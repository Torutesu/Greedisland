#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GreeislandDebugGameMode.generated.h"

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandDebugGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AGreeislandDebugGameMode();

    virtual void BeginPlay() override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bAutoBuildMvpZone = true;

private:
    void BuildMvpZoneIfNeeded();
};
