#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/GreeislandDebugHudWidget.h"
#include "GreeislandDebugHud.generated.h"

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandDebugHud : public AHUD
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    UGreeislandDebugHudWidget* EnsureDebugHudWidget();

    UFUNCTION(BlueprintCallable, Category = "Greeisland|UI")
    void RemoveDebugHudWidget();

    UFUNCTION(BlueprintPure, Category = "Greeisland|UI")
    UGreeislandDebugHudWidget* GetDebugHudWidget() const
    {
        return DebugHudWidget;
    }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|UI")
    TSubclassOf<UGreeislandDebugHudWidget> DebugHudWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|UI")
    bool bCreateWidgetOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|UI")
    int32 AddToViewportZOrder = 100;

private:
    UPROPERTY(Transient)
    UGreeislandDebugHudWidget* DebugHudWidget = nullptr;
};

