#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LuxPlayerController.generated.h"

enum class ELuxRevolverRoundType : uint8;

UCLASS()
class PROJECT_LUX_API ALuxPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void LuxSessionCreate(
		int32 MaxPlayers = 6,
		bool bIsLAN = true,
		FString ListenMapPath = TEXT("/Game/LUX/Maps/L_FPS_TestFacility")
	);

	UFUNCTION(Exec)
	void LuxSessionFind(int32 MaxSearchResults = 50, bool bIsLAN = true);

	UFUNCTION(Exec)
	void LuxSessionJoin(int32 SearchResultIndex = 0);

	UFUNCTION(Exec)
	void LuxSessionFindAndJoin(bool bIsLAN = true);

	UFUNCTION(Exec)
	void LuxSessionDestroy();

	UFUNCTION(Exec)
	void LuxSessionStatus();

	UFUNCTION(Exec, BlueprintCallable, Category = "Development|Revolver", meta = (DevelopmentOnly))
	void LuxLoadRound(FString RoundType);

private:
	UFUNCTION(Server, Reliable)
	void ServerLoadRoundForDevelopment(ELuxRevolverRoundType RoundType);

	UFUNCTION()
	void HandleDevelopmentFindSessionsComplete(bool bWasSuccessful, int32 ResultCount);

	bool bDevelopmentJoinFirstResult = false;
};
