#include "Player/LuxCharacterAnimInstance.h"

#include "GameFramework/Pawn.h"

void ULuxCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!IsValid(Pawn))
	{
		Speed = 0.0f;
		Direction = 0.0f;
		return;
	}

	FVector HorizontalVelocity = Pawn->GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	Speed = HorizontalVelocity.Size();

	if (Speed <= KINDA_SMALL_NUMBER)
	{
		Direction = 0.0f;
		return;
	}

	const FVector LocalVelocity = Pawn->GetActorQuat().UnrotateVector(HorizontalVelocity);
	Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
}
