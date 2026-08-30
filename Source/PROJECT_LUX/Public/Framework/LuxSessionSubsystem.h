#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LuxSessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct PROJECT_LUX_API FLuxSessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 ResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString SessionId;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString OwningUserName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	bool bIsLAN = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLuxCreateSessionCompleteSignature,
	bool,
	bWasSuccessful,
	FName,
	SessionName
);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLuxFindSessionsCompleteSignature,
	bool,
	bWasSuccessful,
	int32,
	ResultCount
);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLuxJoinSessionCompleteSignature,
	bool,
	bWasSuccessful,
	FName,
	SessionName
);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLuxDestroySessionCompleteSignature,
	bool,
	bWasSuccessful,
	FName,
	SessionName
);

struct FLuxSessionSubsystemState;

UCLASS()
class PROJECT_LUX_API ULuxSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool CreateSession(
		int32 MaxPlayers = 6,
		bool bIsLAN = true,
		const FString& ListenMapPath = TEXT("/Game/LUX/Maps/L_FPS_TestFacility")
	);

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool FindSessions(int32 MaxSearchResults = 50, bool bIsLAN = true);

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool JoinSession(int32 SearchResultIndex);

	UFUNCTION(BlueprintCallable, Category = "Session")
	bool DestroySession();

	UFUNCTION(BlueprintPure, Category = "Session")
	FName GetActiveSessionName() const { return ActiveSessionName; }

	UFUNCTION(BlueprintPure, Category = "Session")
	FName GetOnlineSubsystemName() const { return OnlineSubsystemName; }

	UFUNCTION(BlueprintPure, Category = "Session")
	TArray<FLuxSessionSearchResult> GetSearchResults() const { return SearchResults; }

	UFUNCTION(BlueprintPure, Category = "Session")
	bool HasActiveSession() const;

	UFUNCTION(BlueprintPure, Category = "Session")
	bool IsOperationInProgress() const;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FLuxCreateSessionCompleteSignature OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FLuxFindSessionsCompleteSignature OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FLuxJoinSessionCompleteSignature OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Session")
	FLuxDestroySessionCompleteSignature OnDestroySessionComplete;

private:
	bool BeginCreateSession(int32 MaxPlayers, bool bIsLAN, const FString& ListenMapPath);
	bool BeginJoinSession(int32 SearchResultIndex);
	bool BeginDestroySession();
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void ClearOnlineDelegates();
	FName MakeRuntimeSessionName() const;

	TSharedPtr<FLuxSessionSubsystemState> State;
	FName ActiveSessionName = NAME_None;
	FName OnlineSubsystemName = NAME_None;

	UPROPERTY(Transient)
	TArray<FLuxSessionSearchResult> SearchResults;
};
