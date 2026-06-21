#include "GameFramework/GreeislandDebugGameMode.h"

#include "GameFramework/GreeislandDebugPlayerController.h"
#include "UI/GreeislandDebugHud.h"

AGreeislandDebugGameMode::AGreeislandDebugGameMode()
{
    PlayerControllerClass = AGreeislandDebugPlayerController::StaticClass();
    HUDClass = AGreeislandDebugHud::StaticClass();
}

