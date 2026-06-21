#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Exploration/ExplorationTypes.h"
#include "ExplorationEventLibrary.generated.h"

USTRUCT(BlueprintType)
struct FZoneEventSet
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly)
    FName ZoneId;

    UPROPERTY(BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> ClearRequiredCardIds;

    UPROPERTY(BlueprintReadOnly)
    TArray<FExplorationEventDefinition> Events;
};

UCLASS()
class GREEISLAND_API UExplorationEventLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Exploration")
    static bool LoadZoneEventsFromJsonFile(
        const FString& FilePath,
        FZoneEventSet& OutEventSet,
        TArray<FString>& OutErrors);

    UFUNCTION(BlueprintPure, Category = "Greeisland|Exploration")
    static bool FindEventById(
        const FZoneEventSet& EventSet,
        FName EventId,
        FExplorationEventDefinition& OutEvent);
};

