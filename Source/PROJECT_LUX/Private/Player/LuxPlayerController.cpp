#include "Player/LuxPlayerController.h"

#include "Framework/LuxSessionSubsystem.h"
#include "Player/LuxCharacter.h"
#include "Weapons/LuxRevolver.h"

namespace
{
	ULuxSessionSubsystem* GetLuxSessionSubsystem(const APlayerController* PlayerController)
	{
		const UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<ULuxSessionSubsystem>() : nullptr;
	}

	ALuxRevolver* GetEquippedRevolver(const APlayerController* PlayerController)
	{
		const ALuxCharacter* LuxCharacter =
			PlayerController ? Cast<ALuxCharacter>(PlayerController->GetPawn()) : nullptr;
		return LuxCharacter ? LuxCharacter->GetEquippedRevolver() : nullptr;
	}
}

void ALuxPlayerController::LuxSessionCreate(int32 MaxPlayers, bool bIsLAN, FString ListenMapPath)
{
	if (ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this))
	{
		SessionSubsystem->CreateSession(MaxPlayers, bIsLAN, ListenMapPath);
	}
}

void ALuxPlayerController::LuxSessionFind(int32 MaxSearchResults, bool bIsLAN)
{
	if (ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this))
	{
		SessionSubsystem->FindSessions(MaxSearchResults, bIsLAN);
	}
}

void ALuxPlayerController::LuxSessionJoin(int32 SearchResultIndex)
{
	if (ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this))
	{
		SessionSubsystem->JoinSession(SearchResultIndex);
	}
}

void ALuxPlayerController::LuxSessionFindAndJoin(bool bIsLAN)
{
	ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this);
	if (!SessionSubsystem || bDevelopmentJoinFirstResult)
	{
		return;
	}

	// Development-only convenience for session validation; this does not define the final join UX.
	bDevelopmentJoinFirstResult = true;
	SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &ALuxPlayerController::HandleDevelopmentFindSessionsComplete);
	SessionSubsystem->OnFindSessionsComplete.AddDynamic(this, &ALuxPlayerController::HandleDevelopmentFindSessionsComplete);
	if (!SessionSubsystem->FindSessions(50, bIsLAN))
	{
		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &ALuxPlayerController::HandleDevelopmentFindSessionsComplete);
		bDevelopmentJoinFirstResult = false;
	}
}

void ALuxPlayerController::LuxSessionDestroy()
{
	if (ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this))
	{
		SessionSubsystem->DestroySession();
	}
}

void ALuxPlayerController::LuxSessionStatus()
{
	const ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this);
	if (!SessionSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("LuxSessionStatus: Session Subsystem is unavailable."));
		return;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("LuxSessionStatus: OSS=%s Active=%s Busy=%s SearchResults=%d"),
		*SessionSubsystem->GetOnlineSubsystemName().ToString(),
		*SessionSubsystem->GetActiveSessionName().ToString(),
		SessionSubsystem->IsOperationInProgress() ? TEXT("true") : TEXT("false"),
		SessionSubsystem->GetSearchResults().Num()
	);
}

void ALuxPlayerController::LuxLoadRound(FString RoundType, int32 ReloadPosition)
{
#if UE_BUILD_SHIPPING
	(void)RoundType;
	(void)ReloadPosition;
	return;
#else
	if (ReloadPosition < 1 || ReloadPosition > ALuxRevolver::ChamberCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("LuxLoadRound: reload position must be between 1 and 6."));
		return;
	}

	ELuxRevolverRoundType ParsedRoundType = ELuxRevolverRoundType::Empty;
	if (RoundType.Equals(TEXT("Live"), ESearchCase::IgnoreCase))
	{
		ParsedRoundType = ELuxRevolverRoundType::Live;
	}
	else if (RoundType.Equals(TEXT("Blank"), ESearchCase::IgnoreCase))
	{
		ParsedRoundType = ELuxRevolverRoundType::Blank;
	}
	else if (RoundType.Equals(TEXT("Rubber"), ESearchCase::IgnoreCase))
	{
		ParsedRoundType = ELuxRevolverRoundType::Rubber;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("LuxLoadRound: expected Live, Blank, or Rubber."));
		return;
	}

	if (HasAuthority())
	{
		if (ALuxRevolver* Revolver = GetEquippedRevolver(this))
		{
			// This development driver intentionally enters through the production insertion path.
			Revolver->BeginRoundInsertion(ReloadPosition, ParsedRoundType);
		}
		return;
	}

	ServerLoadRoundForDevelopment(ParsedRoundType, static_cast<uint8>(ReloadPosition));
#endif
}

void ALuxPlayerController::ServerLoadRoundForDevelopment_Implementation(
	ELuxRevolverRoundType RoundType,
	uint8 ReloadPosition)
{
#if UE_BUILD_SHIPPING
	(void)RoundType;
	(void)ReloadPosition;
#else
	if (ALuxRevolver* Revolver = GetEquippedRevolver(this))
	{
		// This development driver intentionally enters through the production insertion path.
		Revolver->BeginRoundInsertion(ReloadPosition, RoundType);
	}
#endif
}

void ALuxPlayerController::HandleDevelopmentFindSessionsComplete(bool bWasSuccessful, int32 ResultCount)
{
	ULuxSessionSubsystem* SessionSubsystem = GetLuxSessionSubsystem(this);
	if (!SessionSubsystem)
	{
		bDevelopmentJoinFirstResult = false;
		return;
	}

	SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &ALuxPlayerController::HandleDevelopmentFindSessionsComplete);
	bDevelopmentJoinFirstResult = false;
	if (bWasSuccessful && ResultCount > 0)
	{
		SessionSubsystem->JoinSession(0);
	}
}
