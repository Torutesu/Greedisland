#include "Cards/CardDefinitionLibrary.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
template <typename TEnum>
bool ReadEnumValue(const FString& RawValue, TEnum& OutValue, FString& OutError)
{
    const UEnum* Enum = StaticEnum<TEnum>();
    if (!Enum)
    {
        OutError = TEXT("enum metadata not found");
        return false;
    }

    int64 Value = Enum->GetValueByNameString(RawValue);
    if (Value == INDEX_NONE)
    {
        const FString ScopedValue = FString::Printf(TEXT("%s::%s"), *Enum->GetName(), *RawValue);
        Value = Enum->GetValueByNameString(ScopedValue);
    }

    if (Value == INDEX_NONE)
    {
        OutError = FString::Printf(TEXT("unknown enum value '%s' for %s"), *RawValue, *Enum->GetName());
        return false;
    }

    OutValue = static_cast<TEnum>(Value);
    return true;
}

bool TryReadString(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FString& OutValue)
{
    return Object.IsValid() && Object->TryGetStringField(FieldName, OutValue);
}

void ReadOptionalInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, int32& OutValue)
{
    double Number = 0.0;
    if (Object.IsValid() && Object->TryGetNumberField(FieldName, Number))
    {
        OutValue = FMath::RoundToInt(Number);
    }
}

void ReadNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, TArray<FName>& OutValues)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, Values))
    {
        return;
    }

    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        const FString StringValue = Value.IsValid() ? Value->AsString() : FString();
        if (!StringValue.IsEmpty())
        {
            OutValues.Add(FName(*StringValue));
        }
    }
}

template <typename TEnum>
void ReadEnumArray(
    const TSharedPtr<FJsonObject>& Object,
    const TCHAR* FieldName,
    TArray<TEnum>& OutValues,
    TArray<FString>& Errors,
    const FString& Path)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, Values))
    {
        return;
    }

    for (int32 Index = 0; Index < Values->Num(); ++Index)
    {
        const FString StringValue = (*Values)[Index].IsValid() ? (*Values)[Index]->AsString() : FString();
        TEnum EnumValue {};
        FString Error;
        if (ReadEnumValue(StringValue, EnumValue, Error))
        {
            OutValues.Add(EnumValue);
        }
        else
        {
            Errors.Add(FString::Printf(TEXT("%s.%s[%d]: %s"), *Path, FieldName, Index, *Error));
        }
    }
}

FCardDurationSpec ReadDuration(
    const TSharedPtr<FJsonObject>& Object,
    TArray<FString>& Errors,
    const FString& Path)
{
    FCardDurationSpec Duration;
    const TSharedPtr<FJsonObject>* DurationObject = nullptr;
    if (!Object.IsValid() || !Object->TryGetObjectField(TEXT("duration"), DurationObject) || !DurationObject->IsValid())
    {
        Errors.Add(FString::Printf(TEXT("%s.duration: missing duration object"), *Path));
        return Duration;
    }

    FString TypeString;
    if (TryReadString(*DurationObject, TEXT("type"), TypeString))
    {
        FString Error;
        if (!ReadEnumValue(TypeString, Duration.Type, Error))
        {
            Errors.Add(FString::Printf(TEXT("%s.duration.type: %s"), *Path, *Error));
        }
    }
    else
    {
        Errors.Add(FString::Printf(TEXT("%s.duration.type: missing string"), *Path));
    }

    ReadOptionalInt(*DurationObject, TEXT("value"), Duration.Value);
    return Duration;
}

FCardConstraintSpec ReadConstraint(
    const TSharedPtr<FJsonObject>& Object,
    TArray<FString>& Errors,
    const FString& Path)
{
    FCardConstraintSpec Constraint;

    FString TypeString;
    if (TryReadString(Object, TEXT("type"), TypeString))
    {
        FString Error;
        if (!ReadEnumValue(TypeString, Constraint.Type, Error))
        {
            Errors.Add(FString::Printf(TEXT("%s.type: %s"), *Path, *Error));
        }
    }
    else
    {
        Errors.Add(FString::Printf(TEXT("%s.type: missing string"), *Path));
    }

    FString StringValue;
    if (TryReadString(Object, TEXT("tag"), StringValue))
    {
        Constraint.Tag = FName(*StringValue);
    }
    if (TryReadString(Object, TEXT("statusId"), StringValue))
    {
        Constraint.StatusId = FName(*StringValue);
    }

    ReadOptionalInt(Object, TEXT("minCount"), Constraint.MinCount);
    ReadOptionalInt(Object, TEXT("value"), Constraint.Value);
    return Constraint;
}

FCardEffectSpec ReadEffect(
    const TSharedPtr<FJsonObject>& Object,
    TArray<FString>& Errors,
    const FString& Path)
{
    FCardEffectSpec Effect;

    FString StringValue;
    if (TryReadString(Object, TEXT("type"), StringValue))
    {
        FString Error;
        if (!ReadEnumValue(StringValue, Effect.Type, Error))
        {
            Errors.Add(FString::Printf(TEXT("%s.type: %s"), *Path, *Error));
        }
    }
    else
    {
        Errors.Add(FString::Printf(TEXT("%s.type: missing string"), *Path));
    }

    if (TryReadString(Object, TEXT("channel"), StringValue))
    {
        FString Error;
        if (!ReadEnumValue(StringValue, Effect.Channel, Error))
        {
            Errors.Add(FString::Printf(TEXT("%s.channel: %s"), *Path, *Error));
        }
    }

    if (TryReadString(Object, TEXT("operation"), StringValue))
    {
        FString Error;
        if (!ReadEnumValue(StringValue, Effect.Operation, Error))
        {
            Errors.Add(FString::Printf(TEXT("%s.operation: %s"), *Path, *Error));
        }
    }

    if (TryReadString(Object, TEXT("phase"), StringValue))
    {
        FString Error;
        if (!ReadEnumValue(StringValue, Effect.Phase, Error))
        {
            Errors.Add(FString::Printf(TEXT("%s.phase: %s"), *Path, *Error));
        }
    }

    if (TryReadString(Object, TEXT("targetTag"), StringValue))
    {
        Effect.TargetTag = FName(*StringValue);
    }
    if (TryReadString(Object, TEXT("cardId"), StringValue))
    {
        Effect.CardId = FName(*StringValue);
    }
    if (TryReadString(Object, TEXT("statusId"), StringValue))
    {
        Effect.StatusId = FName(*StringValue);
    }

    ReadOptionalInt(Object, TEXT("amount"), Effect.Amount);
    ReadOptionalInt(Object, TEXT("value"), Effect.Value);
    ReadOptionalInt(Object, TEXT("priority"), Effect.Priority);
    Effect.Duration = ReadDuration(Object, Errors, Path);
    return Effect;
}
}

bool UCardDefinitionLibrary::LoadCardsFromJsonFile(
    const FString& FilePath,
    TArray<FCardDefinition>& OutCards,
    TArray<FString>& OutErrors)
{
    OutCards.Reset();
    OutErrors.Reset();

    FString ResolvedPath = FilePath;
    if (FPaths::IsRelative(FilePath))
    {
        ResolvedPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
    }

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *ResolvedPath))
    {
        OutErrors.Add(FString::Printf(TEXT("failed to read card file: %s"), *ResolvedPath));
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutErrors.Add(FString::Printf(TEXT("failed to parse card JSON: %s"), *ResolvedPath));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* CardValues = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("cards"), CardValues))
    {
        OutErrors.Add(TEXT("cards: missing card array"));
        return false;
    }

    TSet<FName> SeenCardIds;
    for (int32 CardIndex = 0; CardIndex < CardValues->Num(); ++CardIndex)
    {
        const FString CardPath = FString::Printf(TEXT("cards[%d]"), CardIndex);
        const TSharedPtr<FJsonObject> CardObject = (*CardValues)[CardIndex].IsValid()
            ? (*CardValues)[CardIndex]->AsObject()
            : nullptr;

        if (!CardObject.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("%s: expected object"), *CardPath));
            continue;
        }

        FCardDefinition Card;
        FString StringValue;

        if (TryReadString(CardObject, TEXT("cardId"), StringValue))
        {
            Card.CardId = FName(*StringValue);
            if (SeenCardIds.Contains(Card.CardId))
            {
                OutErrors.Add(FString::Printf(TEXT("%s.cardId: duplicate id '%s'"), *CardPath, *StringValue));
            }
            SeenCardIds.Add(Card.CardId);
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("%s.cardId: missing string"), *CardPath));
        }

        if (TryReadString(CardObject, TEXT("displayName"), StringValue))
        {
            Card.DisplayName = FText::FromString(StringValue);
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("%s.displayName: missing string"), *CardPath));
        }

        if (TryReadString(CardObject, TEXT("kind"), StringValue))
        {
            FString Error;
            if (!ReadEnumValue(StringValue, Card.Kind, Error))
            {
                OutErrors.Add(FString::Printf(TEXT("%s.kind: %s"), *CardPath, *Error));
            }
        }

        if (TryReadString(CardObject, TEXT("rarity"), StringValue))
        {
            FString Error;
            if (!ReadEnumValue(StringValue, Card.Rarity, Error))
            {
                OutErrors.Add(FString::Printf(TEXT("%s.rarity: %s"), *CardPath, *Error));
            }
        }

        ReadNameArray(CardObject, TEXT("tags"), Card.Tags);
        ReadEnumArray(CardObject, TEXT("playablePhases"), Card.PlayablePhases, OutErrors, CardPath);

        const TSharedPtr<FJsonObject>* CostObject = nullptr;
        if (CardObject->TryGetObjectField(TEXT("cost"), CostObject) && CostObject->IsValid())
        {
            ReadOptionalInt(*CostObject, TEXT("energy"), Card.Cost.Energy);
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("%s.cost: missing object"), *CardPath));
        }

        const TArray<TSharedPtr<FJsonValue>>* ConstraintValues = nullptr;
        if (CardObject->TryGetArrayField(TEXT("constraints"), ConstraintValues))
        {
            for (int32 ConstraintIndex = 0; ConstraintIndex < ConstraintValues->Num(); ++ConstraintIndex)
            {
                const TSharedPtr<FJsonObject> ConstraintObject = (*ConstraintValues)[ConstraintIndex].IsValid()
                    ? (*ConstraintValues)[ConstraintIndex]->AsObject()
                    : nullptr;
                if (!ConstraintObject.IsValid())
                {
                    OutErrors.Add(FString::Printf(TEXT("%s.constraints[%d]: expected object"), *CardPath, ConstraintIndex));
                    continue;
                }
                Card.Constraints.Add(ReadConstraint(
                    ConstraintObject,
                    OutErrors,
                    FString::Printf(TEXT("%s.constraints[%d]"), *CardPath, ConstraintIndex)));
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* EffectValues = nullptr;
        if (CardObject->TryGetArrayField(TEXT("effects"), EffectValues))
        {
            for (int32 EffectIndex = 0; EffectIndex < EffectValues->Num(); ++EffectIndex)
            {
                const TSharedPtr<FJsonObject> EffectObject = (*EffectValues)[EffectIndex].IsValid()
                    ? (*EffectValues)[EffectIndex]->AsObject()
                    : nullptr;
                if (!EffectObject.IsValid())
                {
                    OutErrors.Add(FString::Printf(TEXT("%s.effects[%d]: expected object"), *CardPath, EffectIndex));
                    continue;
                }
                Card.Effects.Add(ReadEffect(
                    EffectObject,
                    OutErrors,
                    FString::Printf(TEXT("%s.effects[%d]"), *CardPath, EffectIndex)));
            }
        }

        if (TryReadString(CardObject, TEXT("flavorText"), StringValue))
        {
            Card.FlavorText = FText::FromString(StringValue);
        }

        OutCards.Add(Card);
    }

    return OutErrors.Num() == 0;
}

bool UCardDefinitionLibrary::FindCardById(
    const TArray<FCardDefinition>& Cards,
    FName CardId,
    FCardDefinition& OutCard)
{
    for (const FCardDefinition& Card : Cards)
    {
        if (Card.CardId == CardId)
        {
            OutCard = Card;
            return true;
        }
    }

    return false;
}

