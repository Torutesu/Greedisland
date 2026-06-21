#include "Exploration/ExplorationEventLibrary.h"

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
}

bool UExplorationEventLibrary::LoadZoneEventsFromJsonFile(
    const FString& FilePath,
    FZoneEventSet& OutEventSet,
    TArray<FString>& OutErrors)
{
    OutEventSet = FZoneEventSet();
    OutErrors.Reset();

    FString ResolvedPath = FilePath;
    if (FPaths::IsRelative(FilePath))
    {
        ResolvedPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), FilePath);
    }

    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *ResolvedPath))
    {
        OutErrors.Add(FString::Printf(TEXT("failed to read event file: %s"), *ResolvedPath));
        return false;
    }

    TSharedPtr<FJsonObject> RootObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        OutErrors.Add(FString::Printf(TEXT("failed to parse event JSON: %s"), *ResolvedPath));
        return false;
    }

    double SchemaVersion = 1.0;
    if (RootObject->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion))
    {
        OutEventSet.SchemaVersion = FMath::RoundToInt(SchemaVersion);
    }

    FString StringValue;
    if (TryReadString(RootObject, TEXT("zoneId"), StringValue))
    {
        OutEventSet.ZoneId = FName(*StringValue);
    }
    else
    {
        OutErrors.Add(TEXT("zoneId: missing string"));
    }

    if (TryReadString(RootObject, TEXT("displayName"), StringValue))
    {
        OutEventSet.DisplayName = FText::FromString(StringValue);
    }
    else
    {
        OutErrors.Add(TEXT("displayName: missing string"));
    }

    ReadNameArray(RootObject, TEXT("clearRequiredCardIds"), OutEventSet.ClearRequiredCardIds);

    const TArray<TSharedPtr<FJsonValue>>* EventValues = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("events"), EventValues))
    {
        OutErrors.Add(TEXT("events: missing array"));
        return false;
    }

    TSet<FName> SeenEventIds;
    for (int32 EventIndex = 0; EventIndex < EventValues->Num(); ++EventIndex)
    {
        const FString EventPath = FString::Printf(TEXT("events[%d]"), EventIndex);
        const TSharedPtr<FJsonObject> EventObject = (*EventValues)[EventIndex].IsValid()
            ? (*EventValues)[EventIndex]->AsObject()
            : nullptr;

        if (!EventObject.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("%s: expected object"), *EventPath));
            continue;
        }

        FExplorationEventDefinition Event;
        if (TryReadString(EventObject, TEXT("eventId"), StringValue))
        {
            Event.EventId = FName(*StringValue);
            if (SeenEventIds.Contains(Event.EventId))
            {
                OutErrors.Add(FString::Printf(TEXT("%s.eventId: duplicate id '%s'"), *EventPath, *StringValue));
            }
            SeenEventIds.Add(Event.EventId);
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("%s.eventId: missing string"), *EventPath));
        }

        if (TryReadString(EventObject, TEXT("type"), StringValue))
        {
            FString Error;
            if (!ReadEnumValue(StringValue, Event.Type, Error))
            {
                OutErrors.Add(FString::Printf(TEXT("%s.type: %s"), *EventPath, *Error));
            }
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("%s.type: missing string"), *EventPath));
        }

        if (TryReadString(EventObject, TEXT("displayName"), StringValue))
        {
            Event.DisplayName = FText::FromString(StringValue);
        }
        else
        {
            OutErrors.Add(FString::Printf(TEXT("%s.displayName: missing string"), *EventPath));
        }

        if (TryReadString(EventObject, TEXT("description"), StringValue))
        {
            Event.Description = FText::FromString(StringValue);
        }

        if (TryReadString(EventObject, TEXT("npcId"), StringValue))
        {
            Event.NpcId = FName(*StringValue);
        }

        if (TryReadString(EventObject, TEXT("enemyId"), StringValue))
        {
            Event.EnemyId = FName(*StringValue);
        }

        ReadOptionalInt(EventObject, TEXT("enemyHp"), Event.EnemyHp);
        ReadOptionalInt(EventObject, TEXT("enemyAttack"), Event.EnemyAttack);
        ReadNameArray(EventObject, TEXT("rewardCardIds"), Event.RewardCardIds);
        ReadNameArray(EventObject, TEXT("requiredCardIds"), Event.RequiredCardIds);
        ReadNameArray(EventObject, TEXT("allowedAiRewardCardIds"), Event.AllowedAiRewardCardIds);
        ReadNameArray(EventObject, TEXT("nextEventIds"), Event.NextEventIds);
        OutEventSet.Events.Add(Event);
    }

    return OutErrors.Num() == 0;
}

bool UExplorationEventLibrary::FindEventById(
    const FZoneEventSet& EventSet,
    FName EventId,
    FExplorationEventDefinition& OutEvent)
{
    for (const FExplorationEventDefinition& Event : EventSet.Events)
    {
        if (Event.EventId == EventId)
        {
            OutEvent = Event;
            return true;
        }
    }

    return false;
}
