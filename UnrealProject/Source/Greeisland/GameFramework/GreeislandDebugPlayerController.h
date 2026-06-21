#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GreeislandDebugPlayerController.generated.h"

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandDebugPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AGreeislandDebugPlayerController();
};

