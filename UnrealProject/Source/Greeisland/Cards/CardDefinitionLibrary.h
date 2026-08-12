#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Cards/CardTypes.h"
#include "CardDefinitionLibrary.generated.h"

UCLASS()
class GREEISLAND_API UCardDefinitionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Cards")
    static bool LoadCardsFromJsonFile(
        const FString& FilePath,
        TArray<FCardDefinition>& OutCards,
        TArray<FString>& OutErrors);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Cards")
    static bool FindCardById(
        const TArray<FCardDefinition>& Cards,
        FName CardId,
        FCardDefinition& OutCard);
};
