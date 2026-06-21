#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Save/GreeislandSaveGame.h"
#include "Session/GameSessionTypes.h"
#include "SaveGameMapper.generated.h"

UCLASS()
class GREEISLAND_API USaveGameMapper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Greeisland|Save")
    static void CopySessionToSaveGame(
        const FGreeislandGameSession& Session,
        UPARAM(ref) UGreeislandSaveGame*& SaveGame);

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Save")
    static FSessionActionResult ApplySaveGameToSession(
        const UGreeislandSaveGame* SaveGame,
        const TArray<FCardDefinition>& KnownCards,
        const FZoneEventSet& ZoneEventSet,
        FGreeislandGameSession& OutSession);
};

