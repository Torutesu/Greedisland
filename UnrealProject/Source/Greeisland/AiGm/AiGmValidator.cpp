#include "AiGm/AiGmValidator.h"

namespace
{
bool CardExists(const TArray<FCardDefinition>& KnownCards, FName CardId)
{
    for (const FCardDefinition& Card : KnownCards)
    {
        if (Card.CardId == CardId)
        {
            return true;
        }
    }

    return false;
}

bool HasSuspiciousIpSimilarity(const FString& Text)
{
    static const TArray<FString> BlockedTerms = {
        TEXT("ハンター"),
        TEXT("グリードアイランド"),
        TEXT("念能力"),
        TEXT("指定ポケット"),
        TEXT("HxH"),
        TEXT("G.I.")
    };

    for (const FString& Term : BlockedTerms)
    {
        if (Text.Contains(*Term, ESearchCase::IgnoreCase, ESearchDir::FromStart))
        {
            return true;
        }
    }

    return false;
}

bool AllowedQuestEventExists(const FAiGmRequest& Request, FName EventId)
{
    return !EventId.IsNone() && Request.AllowedQuestEventIds.Contains(EventId);
}
}

FAiGmValidationResult UAiGmValidator::ValidateResponse(
    const FAiGmResponse& Response,
    const FAiGmRequest& Request,
    const TArray<FCardDefinition>& KnownCards)
{
    FAiGmValidationResult Result;

    if (Response.SpeakerName.TrimStartAndEnd().IsEmpty())
    {
        Result.Reasons.Add(TEXT("Speaker name is empty."));
    }

    if (Response.SpeakerName.Len() > 48)
    {
        Result.Reasons.Add(TEXT("Speaker name is too long."));
    }

    if (Response.Dialogue.TrimStartAndEnd().IsEmpty())
    {
        Result.Reasons.Add(TEXT("Dialogue is empty."));
    }

    if (Response.Dialogue.Len() > 420)
    {
        Result.Reasons.Add(TEXT("Dialogue is too long."));
    }

    if (HasSuspiciousIpSimilarity(Response.SpeakerName) || HasSuspiciousIpSimilarity(Response.Dialogue))
    {
        Result.Reasons.Add(TEXT("Response contains blocked IP-adjacent wording."));
    }

    if (Response.AllowedRewardCardIds.Num() > 3)
    {
        Result.Reasons.Add(TEXT("Too many reward card ids."));
    }

    if (Response.Intent == EAiGmIntent::QuestOffer)
    {
        if (Response.ProposedQuestId.IsNone())
        {
            Result.Reasons.Add(TEXT("Quest-offer intent requires a proposed quest event id."));
        }
        else if (!AllowedQuestEventExists(Request, Response.ProposedQuestId))
        {
            Result.Reasons.Add(FString::Printf(
                TEXT("Proposed quest event %s was not allowed by the request."),
                *Response.ProposedQuestId.ToString()));
        }
    }
    else if (!Response.ProposedQuestId.IsNone())
    {
        if (!AllowedQuestEventExists(Request, Response.ProposedQuestId))
        {
            Result.Reasons.Add(FString::Printf(
                TEXT("Proposed quest event %s was not allowed by the request."),
                *Response.ProposedQuestId.ToString()));
        }
    }

    TSet<FName> SeenRewardIds;
    for (const FName& RewardCardId : Response.AllowedRewardCardIds)
    {
        if (RewardCardId.IsNone())
        {
            Result.Reasons.Add(TEXT("Reward card id is empty."));
            continue;
        }

        if (SeenRewardIds.Contains(RewardCardId))
        {
            Result.Reasons.Add(FString::Printf(
                TEXT("Duplicate reward card id %s."),
                *RewardCardId.ToString()));
        }
        SeenRewardIds.Add(RewardCardId);

        if (!Request.AllowedRewardCardIds.Contains(RewardCardId))
        {
            Result.Reasons.Add(FString::Printf(
                TEXT("Reward card %s was not allowed by the request."),
                *RewardCardId.ToString()));
        }

        if (!CardExists(KnownCards, RewardCardId))
        {
            Result.Reasons.Add(FString::Printf(
                TEXT("Reward card %s is not defined."),
                *RewardCardId.ToString()));
        }
    }

    Result.bIsValid = Result.Reasons.Num() == 0;
    return Result;
}
