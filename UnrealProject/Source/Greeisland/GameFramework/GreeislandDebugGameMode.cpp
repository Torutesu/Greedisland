#include "GameFramework/GreeislandDebugGameMode.h"

#include "Characters/GreeislandDebugCharacter.h"
#include "GameFramework/GreeislandDebugPlayerController.h"
#include "UI/GreeislandDebugHud.h"

AGreeislandDebugGameMode::AGreeislandDebugGameMode()
{
    DefaultPawnClass = AGreeislandDebugCharacter::StaticClass();
    PlayerControllerClass = AGreeislandDebugPlayerController::StaticClass();
    HUDClass = AGreeislandDebugHud::StaticClass();
}
