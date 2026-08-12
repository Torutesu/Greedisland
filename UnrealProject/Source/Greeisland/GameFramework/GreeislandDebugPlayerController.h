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

    virtual void SetupInputComponent() override;

private:
    class UGreeislandDebugHudWidget* GetDebugHudWidget() const;
    void InitializeSessionHotkey();
    void RestoreSessionHotkey();
    void SaveSessionHotkey();
    void ResolveActiveEventHotkey();
    void StartCombatHotkey();
    void RunEnemyTurnHotkey();
    void BuildFallbackAiHotkey();
    void ApplyAiHotkey();
    void PlayHandCardSlot1();
    void PlayHandCardSlot2();
    void PlayHandCardSlot3();
    void PlayHandCardSlot4();
    void PlayHandCardSlot5();
    void PlayHandCardAtSlot(int32 SlotIndex);
};
