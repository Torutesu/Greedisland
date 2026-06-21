#include "Rules/RuleResolver.h"

namespace
{
bool ArrayContainsName(const TArray<FName>& Values, FName Needle)
{
    return Values.Contains(Needle);
}

int32 CountName(const TArray<FName>& Values, FName Needle)
{
    int32 Count = 0;
    for (const FName& Value : Values)
    {
        if (Value == Needle)
        {
            ++Count;
        }
    }
    return Count;
}

bool CardHasTag(const FCardDefinition& Card, FName Tag)
{
    return Card.Tags.Contains(Tag);
}

void SortPatchesForApplication(TArray<FRulePatch>& Patches)
{
    Patches.Sort([](const FRulePatch& Left, const FRulePatch& Right)
    {
        if (Left.Priority != Right.Priority)
        {
            return Left.Priority < Right.Priority;
        }
        return Left.SourceCardId.LexicalLess(Right.SourceCardId);
    });
}

bool PhaseIsDirectlyAllowed(const FCardDefinition& Card, EGamePhase Phase)
{
    return Card.PlayablePhases.Contains(Phase);
}

bool PhaseIsAllowedByPatch(
    const FCardDefinition& Card,
    EGamePhase Phase,
    const TArray<FRulePatch>& Patches)
{
    for (const FRulePatch& Patch : Patches)
    {
        if (Patch.Channel != ERuleChannel::CardPlayablePhase ||
            Patch.Operation != ERuleOperation::AddPhase ||
            Patch.Phase != Phase)
        {
            continue;
        }

        if (Patch.TargetTag.IsNone() || CardHasTag(Card, Patch.TargetTag))
        {
            return true;
        }
    }

    return false;
}
}

void URuleResolver::GatherRulePatches(
    const FCardPlayContext& Context,
    TArray<FRulePatch>& OutPatches)
{
    OutPatches.Reset();

    for (const FCardDefinition& SourceCard : Context.ActiveRuleCards)
    {
        for (const FCardEffectSpec& Effect : SourceCard.Effects)
        {
            if (Effect.Type != ECardEffectType::ModifyRule)
            {
                continue;
            }

            FRulePatch Patch;
            Patch.SourceCardId = SourceCard.CardId;
            Patch.Channel = Effect.Channel;
            Patch.Operation = Effect.Operation;
            Patch.Value = Effect.Value;
            Patch.TargetTag = Effect.TargetTag;
            Patch.Phase = Effect.Phase;
            Patch.Duration = Effect.Duration;
            Patch.Priority = Effect.Priority;
            OutPatches.Add(Patch);
        }
    }

    SortPatchesForApplication(OutPatches);
}

int32 URuleResolver::ResolveIntRule(
    ERuleChannel Channel,
    int32 BaseValue,
    const FCardPlayContext& Context,
    TArray<FRulePatch>& OutAppliedPatches)
{
    TArray<FRulePatch> Patches;
    GatherRulePatches(Context, Patches);

    int32 Value = BaseValue;
    OutAppliedPatches.Reset();

    for (const FRulePatch& Patch : Patches)
    {
        if (Patch.Channel != Channel)
        {
            continue;
        }

        switch (Patch.Operation)
        {
            case ERuleOperation::AddValue:
                Value += Patch.Value;
                OutAppliedPatches.Add(Patch);
                break;

            case ERuleOperation::SetValue:
                Value = Patch.Value;
                OutAppliedPatches.Add(Patch);
                break;

            case ERuleOperation::MultiplyValue:
                Value *= Patch.Value;
                OutAppliedPatches.Add(Patch);
                break;

            case ERuleOperation::AddMatchingTagCount:
            {
                const int32 PerTagValue = Patch.Value == 0 ? 1 : Patch.Value;
                Value += CountName(Context.CollectionTags, Patch.TargetTag) * PerTagValue;
                OutAppliedPatches.Add(Patch);
                break;
            }

            case ERuleOperation::AddPhase:
            case ERuleOperation::SetFlag:
                break;
        }
    }

    return Value;
}

FCardPlayResult URuleResolver::CanPlayCard(
    const FCardDefinition& Card,
    const FCardPlayContext& Context)
{
    FCardPlayResult Result;

    TArray<FRulePatch> AllPatches;
    GatherRulePatches(Context, AllPatches);
    Result.AppliedPatches = AllPatches;

    TArray<FRulePatch> PartyPatches;
    Result.EffectivePartySize = ResolveIntRule(
        ERuleChannel::PartyRequirement,
        Context.BasePartySize,
        Context,
        PartyPatches);

    for (const FRulePatch& Patch : PartyPatches)
    {
        Result.AppliedPatches.Add(Patch);
    }

    if (Card.Kind == ECardKind::Key)
    {
        Result.Reasons.Add(TEXT("Key cards are progression state and cannot be played directly."));
    }

    if (Card.Cost.Energy > Context.EnergyAvailable)
    {
        Result.Reasons.Add(FString::Printf(
            TEXT("Not enough energy: need %d, have %d."),
            Card.Cost.Energy,
            Context.EnergyAvailable));
    }

    const bool bPhaseAllowed =
        PhaseIsDirectlyAllowed(Card, Context.CurrentPhase) ||
        PhaseIsAllowedByPatch(Card, Context.CurrentPhase, AllPatches);

    if (!bPhaseAllowed)
    {
        Result.Reasons.Add(TEXT("Card is not playable in the current phase."));
    }

    for (const FCardConstraintSpec& Constraint : Card.Constraints)
    {
        switch (Constraint.Type)
        {
            case ECardConstraintType::HasTagInCollection:
                if (CountName(Context.CollectionTags, Constraint.Tag) < Constraint.MinCount)
                {
                    Result.Reasons.Add(FString::Printf(
                        TEXT("Requires at least %d collected card(s) with tag '%s'."),
                        Constraint.MinCount,
                        *Constraint.Tag.ToString()));
                }
                break;

            case ECardConstraintType::EffectivePartySizeAtLeast:
                if (Result.EffectivePartySize < Constraint.MinCount)
                {
                    Result.Reasons.Add(FString::Printf(
                        TEXT("Requires effective party size %d, currently %d."),
                        Constraint.MinCount,
                        Result.EffectivePartySize));
                }
                break;

            case ECardConstraintType::HandCountAtMost:
                if (Context.HandCount > Constraint.Value)
                {
                    Result.Reasons.Add(FString::Printf(
                        TEXT("Requires hand count at most %d, currently %d."),
                        Constraint.Value,
                        Context.HandCount));
                }
                break;

            case ECardConstraintType::HasStatus:
                if (!ArrayContainsName(Context.ActiveStatusIds, Constraint.StatusId))
                {
                    Result.Reasons.Add(FString::Printf(
                        TEXT("Requires status '%s'."),
                        *Constraint.StatusId.ToString()));
                }
                break;

            case ECardConstraintType::NotHasStatus:
                if (ArrayContainsName(Context.ActiveStatusIds, Constraint.StatusId))
                {
                    Result.Reasons.Add(FString::Printf(
                        TEXT("Cannot be used while status '%s' is active."),
                        *Constraint.StatusId.ToString()));
                }
                break;
        }
    }

    Result.bCanPlay = Result.Reasons.Num() == 0;
    return Result;
}
