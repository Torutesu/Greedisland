#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GreeislandProjectSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Greeisland"))
class GREEISLAND_API UGreeislandProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override
    {
        return TEXT("Game");
    }

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    FString CardJsonPath = TEXT("../../../data/cards/cards.mvp.json");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    FString EventJsonPath = TEXT("../../../data/events/events.mvp.json");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    FString SaveSlotName = TEXT("greeisland-dev-slot");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    int32 SaveUserIndex = 0;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    FString DefaultPlayerId = TEXT("dev-player");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    int32 DefaultOpeningDrawCount = 5;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Debug HUD")
    int32 SnapshotLogLineCount = 12;
};

