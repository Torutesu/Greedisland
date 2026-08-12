#include "Combat/CombatEngine.h"

namespace
{
int32 ApplyIncomingDamage(FCombatantState& Target, int32 Damage)
{
    const int32 Blocked = FMath::Min(Target.Block, Damage);
    Target.Block -= Blocked;

    const int32 RemainingDamage = Damage - Blocked;
    Target.CurrentHp = FMath::Clamp(Target.CurrentHp - RemainingDamage, 0, Target.MaxHp);
    return RemainingDamage;
}

void MoveCardFromHandToDiscard(FCombatState& State, FName CardId)
{
    State.Hand.RemoveSingle(CardId);
    State.DiscardPile.Add(CardId);
}

void UpdateOutcome(FCombatState& State)
{
    if (State.Enemy.CurrentHp <= 0)
    {
        State.Outcome = ECombatOutcome::PlayerVictory;
        return;
    }

    if (State.Player.CurrentHp <= 0)
    {
        State.Outcome = ECombatOutcome::PlayerDefeat;
        return;
    }

    State.Outcome = ECombatOutcome::InProgress;
}

FString CardNameForLog(const FCardDefinition& Card)
{
    if (!Card.DisplayName.IsEmpty())
    {
        return Card.DisplayName.ToString();
    }
    return Card.CardId.ToString();
}
}

FCombatState UCombatEngine::CreateCombatState(
    const TArray<FName>& DeckCardIds,
    FName EnemyId,
    int32 EnemyHp,
    int32 StartingEnergy)
{
    FCombatState State;
    State.Player.CombatantId = TEXT("player");
    State.Player.MaxHp = 30;
    State.Player.CurrentHp = 30;
    State.Player.Block = 0;

    State.Enemy.CombatantId = EnemyId;
    State.Enemy.MaxHp = FMath::Max(1, EnemyHp);
    State.Enemy.CurrentHp = State.Enemy.MaxHp;
    State.Enemy.Block = 0;

    State.TurnNumber = 1;
    State.Energy = FMath::Max(0, StartingEnergy);
    State.DrawPile = DeckCardIds;
    State.Hand.Reset();
    State.DiscardPile.Reset();
    State.ClaimedRewardCardIds.Reset();
    State.Outcome = ECombatOutcome::InProgress;
    return State;
}

void UCombatEngine::DrawCards(
    FCombatState& State,
    int32 Count,
    TArray<FString>& OutLogLines)
{
    const int32 SafeCount = FMath::Max(0, Count);
    for (int32 DrawIndex = 0; DrawIndex < SafeCount; ++DrawIndex)
    {
        if (State.DrawPile.Num() == 0)
        {
            if (State.DiscardPile.Num() == 0)
            {
                OutLogLines.Add(TEXT("No cards left to draw."));
                return;
            }

            State.DrawPile = State.DiscardPile;
            State.DiscardPile.Reset();
            OutLogLines.Add(TEXT("Discard pile recycled into draw pile."));
        }

        FName CardId = State.DrawPile[0];
        State.DrawPile.RemoveAt(0);
        State.Hand.Add(CardId);
        OutLogLines.Add(FString::Printf(TEXT("Drew %s."), *CardId.ToString()));
    }
}

FCombatActionResult UCombatEngine::PlayCard(
    FCombatState& State,
    const FCardDefinition& Card,
    const FCardPlayContext& BaseContext)
{
    FCombatActionResult Result;
    Result.Outcome = State.Outcome;

    if (State.Outcome != ECombatOutcome::InProgress)
    {
        Result.Reasons.Add(TEXT("Combat is already finished."));
        return Result;
    }

    if (!State.Hand.Contains(Card.CardId))
    {
        Result.Reasons.Add(FString::Printf(TEXT("Card %s is not in hand."), *Card.CardId.ToString()));
        return Result;
    }

    if (Card.Kind == ECardKind::Rule || Card.Kind == ECardKind::Constraint)
    {
        Result.Reasons.Add(TEXT("Rule and Constraint cards are active while owned and cannot be played directly."));
        return Result;
    }

    if (Card.Kind == ECardKind::Key)
    {
        Result.Reasons.Add(TEXT("Key cards are progression state and cannot be played directly."));
        return Result;
    }

    FCardPlayContext Context = BaseContext;
    Context.CurrentPhase = EGamePhase::Combat;
    Context.EnergyAvailable = State.Energy;
    Context.HandCount = State.Hand.Num();

    const FCardPlayResult PlayResult = URuleResolver::CanPlayCard(Card, Context);
    if (!PlayResult.bCanPlay)
    {
        Result.Reasons = PlayResult.Reasons;
        return Result;
    }

    State.Energy -= Card.Cost.Energy;
    Result.LogLines.Add(FString::Printf(
        TEXT("Played %s for %d energy."),
        *CardNameForLog(Card),
        Card.Cost.Energy));

    for (const FCardEffectSpec& Effect : Card.Effects)
    {
        switch (Effect.Type)
        {
            case ECardEffectType::Damage:
            {
                const int32 Applied = ApplyIncomingDamage(State.Enemy, Effect.Amount);
                Result.LogLines.Add(FString::Printf(
                    TEXT("%s dealt %d damage."),
                    *CardNameForLog(Card),
                    Applied));
                break;
            }

            case ECardEffectType::GainBlock:
                State.Player.Block += Effect.Amount;
                Result.LogLines.Add(FString::Printf(
                    TEXT("Player gained %d block."),
                    Effect.Amount));
                break;

            case ECardEffectType::Heal:
            {
                const int32 Before = State.Player.CurrentHp;
                State.Player.CurrentHp = FMath::Clamp(
                    State.Player.CurrentHp + Effect.Amount,
                    0,
                    State.Player.MaxHp);
                Result.LogLines.Add(FString::Printf(
                    TEXT("Player healed %d HP."),
                    State.Player.CurrentHp - Before));
                break;
            }

            case ECardEffectType::DrawCards:
                DrawCards(State, Effect.Amount, Result.LogLines);
                break;

            case ECardEffectType::ClaimReward:
                State.ClaimedRewardCardIds.Add(Card.CardId);
                Result.LogLines.Add(TEXT("A reward claim was reserved."));
                break;

            case ECardEffectType::ModifyRule:
            case ECardEffectType::GainCard:
            case ECardEffectType::AddStatus:
            case ECardEffectType::RemoveStatus:
            case ECardEffectType::PreventFailure:
                Result.LogLines.Add(FString::Printf(
                    TEXT("Effect %d is recorded for a later system."),
                    static_cast<int32>(Effect.Type)));
                break;
        }
    }

    MoveCardFromHandToDiscard(State, Card.CardId);
    UpdateOutcome(State);

    Result.bSuccess = true;
    Result.Outcome = State.Outcome;
    return Result;
}

FCombatActionResult UCombatEngine::RunEnemyTurn(
    FCombatState& State,
    int32 EnemyAttackDamage,
    int32 DrawCount)
{
    FCombatActionResult Result;
    Result.Outcome = State.Outcome;

    if (State.Outcome != ECombatOutcome::InProgress)
    {
        Result.Reasons.Add(TEXT("Combat is already finished."));
        return Result;
    }

    const int32 AppliedDamage = ApplyIncomingDamage(State.Player, FMath::Max(0, EnemyAttackDamage));
    Result.LogLines.Add(FString::Printf(TEXT("Enemy dealt %d damage."), AppliedDamage));

    State.Player.Block = 0;
    State.Enemy.Block = 0;
    State.TurnNumber += 1;
    State.Energy = 3;
    DrawCards(State, DrawCount, Result.LogLines);
    UpdateOutcome(State);

    Result.bSuccess = true;
    Result.Outcome = State.Outcome;
    return Result;
}
