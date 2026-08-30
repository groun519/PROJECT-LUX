#include "Player/LuxPlayerController.h"

#include "Framework/LuxSessionSubsystem.h"

namespace
{
	ULuxSessionSubsystem* GetLuxSessionSubsystem(const APlayerController* PlayerController)
	{
		const UGameInstance* GameInstance = PlayerController ? PlayerController->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<ULuxSessionSubsystem>() : nullptr;
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
