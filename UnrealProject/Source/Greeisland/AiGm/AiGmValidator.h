#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AiGm/AiGmTypes.h"
#include "Cards/CardTypes.h"
#include "AiGmValidator.generated.h"

USTRUCT(BlueprintType)
struct FAiGmValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bIsValid = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> Reasons;
};

UCLASS()
class GREEISLAND_API UAiGmValidator : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Greeisland|AI GM")
    static FAiGmValidationResult ValidateResponse(
        const FAiGmResponse& Response,
        const FAiGmRequest& Request,
        const TArray<FCardDefinition>& KnownCards);
};

