#include "Framework/LuxSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogLuxSession, Log, All);

namespace
{
	const FName GLuxSessionMarker(TEXT("LUX_SESSION"));

	enum class ELuxPendingSessionAction : uint8
	{
		None,
		Create,
		Join
	};
}

struct FLuxSessionSubsystemState
{
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	TArray<int32> RawSearchResultIndices;
	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle FindSessionsCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle DestroySessionCompleteHandle;
	ELuxPendingSessionAction PendingAction = ELuxPendingSessionAction::None;
	int32 PendingMaxPlayers = 6;
	bool bPendingIsLAN = true;
	FString PendingListenMapPath;
	int32 PendingSearchResultIndex = INDEX_NONE;
	bool bOperationInProgress = false;
};

void ULuxSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	State = MakeShared<FLuxSessionSubsystemState>();

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (!OnlineSubsystem)
	{
		UE_LOG(LogLuxSession, Error, TEXT("No Online Subsystem is available."));
		return;
	}

	OnlineSubsystemName = OnlineSubsystem->GetSubsystemName();
	State->SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!State->SessionInterface.IsValid())
	{
		UE_LOG(LogLuxSession, Error, TEXT("Online Subsystem [%s] has no Session Interface."), *OnlineSubsystemName.ToString());
		return;
	}

	UE_LOG(LogLuxSession, Display, TEXT("Initialized with Online Subsystem [%s]."), *OnlineSubsystemName.ToString());
}

void ULuxSessionSubsystem::Deinitialize()
{
	if (State && State->SessionInterface.IsValid())
	{
		ClearOnlineDelegates();
		if (!ActiveSessionName.IsNone() && State->SessionInterface->GetNamedSession(ActiveSessionName))
		{
			State->SessionInterface->DestroySession(ActiveSessionName);
		}
	}

	SearchResults.Reset();
	ActiveSessionName = NAME_None;
	OnlineSubsystemName = NAME_None;
	State.Reset();
	Super::Deinitialize();
}

bool ULuxSessionSubsystem::CreateSession(int32 MaxPlayers, bool bIsLAN, const FString& ListenMapPath)
{
	if (!State || !State->SessionInterface.IsValid() || State->bOperationInProgress)
	{
		UE_LOG(LogLuxSession, Warning, TEXT("CreateSession rejected because the Session Interface is unavailable or busy."));
		return false;
	}
	if (MaxPlayers < 2)
	{
		UE_LOG(LogLuxSession, Warning, TEXT("CreateSession requires at least two public connections."));
		return false;
	}
	if (!FPackageName::IsValidLongPackageName(ListenMapPath))
	{
		UE_LOG(LogLuxSession, Warning, TEXT("CreateSession rejected invalid map path [%s]."), *ListenMapPath);
		return false;
	}

	if (!ActiveSessionName.IsNone() && State->SessionInterface->GetNamedSession(ActiveSessionName))
	{
		State->PendingAction = ELuxPendingSessionAction::Create;
		State->PendingMaxPlayers = MaxPlayers;
		State->bPendingIsLAN = bIsLAN;
		State->PendingListenMapPath = ListenMapPath;
		return BeginDestroySession();
	}

	return BeginCreateSession(MaxPlayers, bIsLAN, ListenMapPath);
}

bool ULuxSessionSubsystem::BeginCreateSession(int32 MaxPlayers, bool bIsLAN, const FString& ListenMapPath)
{
	ActiveSessionName = MakeRuntimeSessionName();
	State->PendingListenMapPath = ListenMapPath;

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = bIsLAN;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bAllowInvites = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = !bIsLAN;
	SessionSettings.bAllowJoinViaPresence = !bIsLAN;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	SessionSettings.bUseLobbiesIfAvailable = !bIsLAN;
	SessionSettings.Set(GLuxSessionMarker, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(SETTING_MAPNAME, ListenMapPath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	TWeakObjectPtr<ULuxSessionSubsystem> WeakThis(this);
	State->CreateSessionCompleteHandle = State->SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateLambda(
			[WeakThis](FName SessionName, bool bWasSuccessful)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleCreateSessionComplete(SessionName, bWasSuccessful);
				}
			}
		)
	);
	State->bOperationInProgress = true;

	UE_LOG(
		LogLuxSession,
		Display,
		TEXT("Creating session [%s] with %d public connections using %s mode."),
		*ActiveSessionName.ToString(),
		MaxPlayers,
		bIsLAN ? TEXT("LAN") : TEXT("online")
	);

	if (State->SessionInterface->CreateSession(0, ActiveSessionName, SessionSettings))
	{
		return true;
	}

	State->SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(State->CreateSessionCompleteHandle);
	State->CreateSessionCompleteHandle.Reset();
	State->bOperationInProgress = false;
	const FName FailedSessionName = ActiveSessionName;
	ActiveSessionName = NAME_None;
	OnCreateSessionComplete.Broadcast(false, FailedSessionName);
	return false;
}

void ULuxSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	State->SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(State->CreateSessionCompleteHandle);
	State->CreateSessionCompleteHandle.Reset();
	State->bOperationInProgress = false;

	if (!bWasSuccessful)
	{
		UE_LOG(LogLuxSession, Error, TEXT("CreateSession failed for [%s]."), *SessionName.ToString());
		ActiveSessionName = NAME_None;
		OnCreateSessionComplete.Broadcast(false, SessionName);
		return;
	}

	UE_LOG(LogLuxSession, Display, TEXT("Created session [%s]."), *SessionName.ToString());
	OnCreateSessionComplete.Broadcast(true, SessionName);

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLuxSession, Error, TEXT("Created session but no World is available for Listen Travel."));
		return;
	}

	const FString TravelURL = State->PendingListenMapPath + TEXT("?listen");
	UE_LOG(LogLuxSession, Display, TEXT("Host Listen Travel to [%s]."), *TravelURL);
	World->ServerTravel(TravelURL, true);
}

bool ULuxSessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLAN)
{
	if (!State || !State->SessionInterface.IsValid() || State->bOperationInProgress)
	{
		UE_LOG(LogLuxSession, Warning, TEXT("FindSessions rejected because the Session Interface is unavailable or busy."));
		return false;
	}

	SearchResults.Reset();
	State->RawSearchResultIndices.Reset();
	State->SessionSearch = MakeShared<FOnlineSessionSearch>();
	State->SessionSearch->MaxSearchResults = FMath::Max(1, MaxSearchResults);
	State->SessionSearch->bIsLanQuery = bIsLAN;
	if (!bIsLAN)
	{
		State->SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	TWeakObjectPtr<ULuxSessionSubsystem> WeakThis(this);
	State->FindSessionsCompleteHandle = State->SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateLambda(
			[WeakThis](bool bWasSuccessful)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleFindSessionsComplete(bWasSuccessful);
				}
			}
		)
	);
	State->bOperationInProgress = true;

	UE_LOG(LogLuxSession, Display, TEXT("Searching for up to %d %s sessions."), MaxSearchResults, bIsLAN ? TEXT("LAN") : TEXT("online"));
	if (State->SessionInterface->FindSessions(0, State->SessionSearch.ToSharedRef()))
	{
		return true;
	}

	State->SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(State->FindSessionsCompleteHandle);
	State->FindSessionsCompleteHandle.Reset();
	State->bOperationInProgress = false;
	OnFindSessionsComplete.Broadcast(false, 0);
	return false;
}

void ULuxSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	State->SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(State->FindSessionsCompleteHandle);
	State->FindSessionsCompleteHandle.Reset();
	State->bOperationInProgress = false;
	SearchResults.Reset();
	State->RawSearchResultIndices.Reset();

	if (bWasSuccessful && State->SessionSearch.IsValid())
	{
		for (int32 RawIndex = 0; RawIndex < State->SessionSearch->SearchResults.Num(); ++RawIndex)
		{
			const FOnlineSessionSearchResult& RawResult = State->SessionSearch->SearchResults[RawIndex];
			bool bIsLuxSession = false;
			RawResult.Session.SessionSettings.Get(GLuxSessionMarker, bIsLuxSession);
			if (!RawResult.IsValid() || !bIsLuxSession)
			{
				continue;
			}

			FLuxSessionSearchResult& Result = SearchResults.AddDefaulted_GetRef();
			Result.ResultIndex = SearchResults.Num() - 1;
			Result.SessionId = RawResult.GetSessionIdStr();
			Result.OwningUserName = RawResult.Session.OwningUserName;
			Result.PingMs = RawResult.PingInMs;
			Result.MaxPlayers = RawResult.Session.SessionSettings.NumPublicConnections;
			Result.CurrentPlayers = FMath::Max(0, Result.MaxPlayers - RawResult.Session.NumOpenPublicConnections);
			Result.bIsLAN = RawResult.Session.SessionSettings.bIsLANMatch;
			State->RawSearchResultIndices.Add(RawIndex);
		}
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogLuxSession, Display, TEXT("Session search completed with %d LUX results."), SearchResults.Num());
	}
	else
	{
		UE_LOG(LogLuxSession, Error, TEXT("Session search failed."));
	}
	OnFindSessionsComplete.Broadcast(bWasSuccessful, SearchResults.Num());
}

bool ULuxSessionSubsystem::JoinSession(int32 SearchResultIndex)
{
	if (!State || !State->SessionInterface.IsValid() || State->bOperationInProgress)
	{
		UE_LOG(LogLuxSession, Warning, TEXT("JoinSession rejected because the Session Interface is unavailable or busy."));
		return false;
	}
	if (!State->SessionSearch.IsValid() || !State->RawSearchResultIndices.IsValidIndex(SearchResultIndex))
	{
		UE_LOG(LogLuxSession, Warning, TEXT("JoinSession rejected invalid search result index [%d]."), SearchResultIndex);
		return false;
	}

	if (!ActiveSessionName.IsNone() && State->SessionInterface->GetNamedSession(ActiveSessionName))
	{
		State->PendingAction = ELuxPendingSessionAction::Join;
		State->PendingSearchResultIndex = SearchResultIndex;
		return BeginDestroySession();
	}

	return BeginJoinSession(SearchResultIndex);
}

bool ULuxSessionSubsystem::BeginJoinSession(int32 SearchResultIndex)
{
	const int32 RawIndex = State->RawSearchResultIndices[SearchResultIndex];
	if (!State->SessionSearch->SearchResults.IsValidIndex(RawIndex))
	{
		return false;
	}

	ActiveSessionName = MakeRuntimeSessionName();
	TWeakObjectPtr<ULuxSessionSubsystem> WeakThis(this);
	State->JoinSessionCompleteHandle = State->SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateLambda(
			[WeakThis](FName SessionName, EOnJoinSessionCompleteResult::Type Result)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleJoinSessionComplete(SessionName, Result == EOnJoinSessionCompleteResult::Success);
				}
			}
		)
	);
	State->bOperationInProgress = true;

	UE_LOG(LogLuxSession, Display, TEXT("Joining search result [%d] as local session [%s]."), SearchResultIndex, *ActiveSessionName.ToString());
	if (State->SessionInterface->JoinSession(0, ActiveSessionName, State->SessionSearch->SearchResults[RawIndex]))
	{
		return true;
	}

	State->SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(State->JoinSessionCompleteHandle);
	State->JoinSessionCompleteHandle.Reset();
	State->bOperationInProgress = false;
	const FName FailedSessionName = ActiveSessionName;
	ActiveSessionName = NAME_None;
	OnJoinSessionComplete.Broadcast(false, FailedSessionName);
	return false;
}

void ULuxSessionSubsystem::HandleJoinSessionComplete(FName SessionName, bool bWasSuccessful)
{
	State->SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(State->JoinSessionCompleteHandle);
	State->JoinSessionCompleteHandle.Reset();
	State->bOperationInProgress = false;

	FString ConnectString;
	if (bWasSuccessful)
	{
		bWasSuccessful = State->SessionInterface->GetResolvedConnectString(SessionName, ConnectString);
	}

	APlayerController* PlayerController = GetGameInstance() ? GetGameInstance()->GetFirstLocalPlayerController() : nullptr;
	if (bWasSuccessful && PlayerController)
	{
		UE_LOG(LogLuxSession, Display, TEXT("Joined session [%s]; Client Travel to [%s]."), *SessionName.ToString(), *ConnectString);
		OnJoinSessionComplete.Broadcast(true, SessionName);
		PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
		return;
	}

	UE_LOG(LogLuxSession, Error, TEXT("JoinSession failed for [%s]."), *SessionName.ToString());
	ActiveSessionName = NAME_None;
	OnJoinSessionComplete.Broadcast(false, SessionName);
}

bool ULuxSessionSubsystem::DestroySession()
{
	if (!State || !State->SessionInterface.IsValid() || State->bOperationInProgress)
	{
		UE_LOG(LogLuxSession, Warning, TEXT("DestroySession rejected because the Session Interface is unavailable or busy."));
		return false;
	}

	State->PendingAction = ELuxPendingSessionAction::None;
	if (ActiveSessionName.IsNone() || !State->SessionInterface->GetNamedSession(ActiveSessionName))
	{
		const FName PreviousSessionName = ActiveSessionName;
		ActiveSessionName = NAME_None;
		OnDestroySessionComplete.Broadcast(true, PreviousSessionName);
		return true;
	}

	return BeginDestroySession();
}

bool ULuxSessionSubsystem::BeginDestroySession()
{
	TWeakObjectPtr<ULuxSessionSubsystem> WeakThis(this);
	State->DestroySessionCompleteHandle = State->SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateLambda(
			[WeakThis](FName SessionName, bool bWasSuccessful)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleDestroySessionComplete(SessionName, bWasSuccessful);
				}
			}
		)
	);
	State->bOperationInProgress = true;

	UE_LOG(LogLuxSession, Display, TEXT("Destroying session [%s]."), *ActiveSessionName.ToString());
	if (State->SessionInterface->DestroySession(ActiveSessionName))
	{
		return true;
	}

	State->SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(State->DestroySessionCompleteHandle);
	State->DestroySessionCompleteHandle.Reset();
	State->bOperationInProgress = false;
	State->PendingAction = ELuxPendingSessionAction::None;
	OnDestroySessionComplete.Broadcast(false, ActiveSessionName);
	return false;
}

void ULuxSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	State->SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(State->DestroySessionCompleteHandle);
	State->DestroySessionCompleteHandle.Reset();
	State->bOperationInProgress = false;

	const ELuxPendingSessionAction PendingAction = State->PendingAction;
	const int32 PendingMaxPlayers = State->PendingMaxPlayers;
	const bool bPendingIsLAN = State->bPendingIsLAN;
	const FString PendingListenMapPath = State->PendingListenMapPath;
	const int32 PendingSearchResultIndex = State->PendingSearchResultIndex;
	State->PendingAction = ELuxPendingSessionAction::None;
	State->PendingSearchResultIndex = INDEX_NONE;

	if (bWasSuccessful || !State->SessionInterface->GetNamedSession(SessionName))
	{
		ActiveSessionName = NAME_None;
		bWasSuccessful = true;
	}

	if (bWasSuccessful)
	{
		UE_LOG(LogLuxSession, Display, TEXT("Destroyed session [%s]."), *SessionName.ToString());
	}
	else
	{
		UE_LOG(LogLuxSession, Error, TEXT("DestroySession failed for [%s]."), *SessionName.ToString());
	}
	OnDestroySessionComplete.Broadcast(bWasSuccessful, SessionName);

	if (!bWasSuccessful)
	{
		return;
	}
	if (PendingAction == ELuxPendingSessionAction::Create)
	{
		BeginCreateSession(PendingMaxPlayers, bPendingIsLAN, PendingListenMapPath);
	}
	else if (PendingAction == ELuxPendingSessionAction::Join)
	{
		BeginJoinSession(PendingSearchResultIndex);
	}
}

bool ULuxSessionSubsystem::HasActiveSession() const
{
	return State && State->SessionInterface.IsValid() && !ActiveSessionName.IsNone()
		&& State->SessionInterface->GetNamedSession(ActiveSessionName) != nullptr;
}

bool ULuxSessionSubsystem::IsOperationInProgress() const
{
	return State && State->bOperationInProgress;
}

void ULuxSessionSubsystem::ClearOnlineDelegates()
{
	if (!State || !State->SessionInterface.IsValid())
	{
		return;
	}
	if (State->CreateSessionCompleteHandle.IsValid())
	{
		State->SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(State->CreateSessionCompleteHandle);
	}
	if (State->FindSessionsCompleteHandle.IsValid())
	{
		State->SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(State->FindSessionsCompleteHandle);
	}
	if (State->JoinSessionCompleteHandle.IsValid())
	{
		State->SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(State->JoinSessionCompleteHandle);
	}
	if (State->DestroySessionCompleteHandle.IsValid())
	{
		State->SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(State->DestroySessionCompleteHandle);
	}
}

FName ULuxSessionSubsystem::MakeRuntimeSessionName() const
{
	const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(12);
	return FName(*FString::Printf(TEXT("LuxSession_%s"), *Suffix));
}
