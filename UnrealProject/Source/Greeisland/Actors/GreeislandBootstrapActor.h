#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Session/GameSessionTypes.h"
#include "GreeislandBootstrapActor.generated.h"

class UGreeislandGameSubsystem;

USTRUCT(BlueprintType)
struct FGreeislandExpectedEventPlacement
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FName EventId;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString EventType;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    int32 PlacementCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bIsPlaced = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bIsDuplicate = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FName> NextEventIds;
};

USTRUCT(BlueprintType)
struct FGreeislandBootstrapDiagnostics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bUsingProjectSettingsDefaults = true;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString EffectiveCardJsonPath;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString EffectiveEventJsonPath;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bCardJsonExists = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bEventJsonExists = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bSaveExists = false;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    int32 BootstrapActorCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    int32 EventActorCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FName> ExpectedEventIds;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FName> MissingEventActorIds;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FName> DuplicateEventActorIds;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FName> UnexpectedEventActorIds;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FGreeislandExpectedEventPlacement> ExpectedEventPlacements;

    UPROPERTY(BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    TArray<FString> Issues;
};

UENUM(BlueprintType)
enum class EBootstrapMode : uint8
{
    InitializeNew,
    RestoreIfPossible,
    RestoreOnly
};

UCLASS(Blueprintable)
class GREEISLAND_API AGreeislandBootstrapActor : public AActor
{
    GENERATED_BODY()

public:
    AGreeislandBootstrapActor();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "Greeisland|Bootstrap")
    FSessionActionResult BootstrapSession();

    UFUNCTION(BlueprintPure, Category = "Greeisland|Bootstrap")
    UGreeislandGameSubsystem* GetGreeislandSubsystem() const;

    UFUNCTION(BlueprintPure, Category = "Greeisland|Bootstrap")
    FGreeislandBootstrapDiagnostics GetBootstrapDiagnostics() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Greeisland|Bootstrap")
    void OnBootstrapFinished(const FSessionActionResult& ActionResult);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bBootstrapOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    EBootstrapMode BootstrapMode = EBootstrapMode::RestoreIfPossible;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    bool bUseProjectSettingsDefaults = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString CardJsonPath = TEXT("../data/cards/cards.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString EventJsonPath = TEXT("../data/events/events.mvp.json");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    FString SaveSlotName = TEXT("greeisland-dev-slot");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Greeisland|Bootstrap")
    int32 SaveUserIndex = 0;

private:
    void GetEffectiveBootstrapSettings(
        FString& OutCardJsonPath,
        FString& OutEventJsonPath,
        FString& OutSaveSlotName,
        int32& OutSaveUserIndex) const;
};
