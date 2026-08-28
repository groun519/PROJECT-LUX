#include "Framework/LuxGameMode.h"

#include "Framework/LuxGameState.h"
#include "Player/LuxCharacter.h"
#include "Player/LuxPlayerController.h"
#include "Player/LuxPlayerState.h"

ALuxGameMode::ALuxGameMode()
{
	DefaultPawnClass = ALuxCharacter::StaticClass();
	PlayerControllerClass = ALuxPlayerController::StaticClass();
	PlayerStateClass = ALuxPlayerState::StaticClass();
	GameStateClass = ALuxGameState::StaticClass();
}
