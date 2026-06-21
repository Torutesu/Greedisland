#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GreeislandSaveGame.generated.h"

UCLASS()
class GREEISLAND_API UGreeislandSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    int32 SaveVersion = 1;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    FString PlayerId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    FName CurrentZoneId = TEXT("zone_mvp_island_001");

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> OwnedCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> DeckCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> CompletedEventIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> AcquiredKeyCardIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> AvailableEventIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> ActiveQuestIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    TArray<FName> CompletedQuestIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    FName ActiveEventId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    bool bZoneCleared = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Greeisland|Save")
    int32 RandomSeed = 0;
};
