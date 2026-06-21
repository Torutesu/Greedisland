#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Exploration/ExplorationTypes.h"
#include "ExplorationEngine.generated.h"

UCLASS()
class GREEISLAND_API UExplorationEngine : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Greeisland|Exploration")
    static FExplorationResolveResult CanResolveEvent(
        const FExplorationEventDefinition& Event,
        const TArray<FName>& OwnedCardIds,
        const FZoneProgressState& ZoneProgress);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Exploration")
    static FExplorationResolveResult ResolveEvent(
        const FExplorationEventDefinition& Event,
        UPARAM(ref) TArray<FName>& OwnedCardIds,
        UPARAM(ref) FZoneProgressState& ZoneProgress);
};

