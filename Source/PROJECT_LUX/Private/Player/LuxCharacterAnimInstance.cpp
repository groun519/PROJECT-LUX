#include "Player/LuxCharacterAnimInstance.h"

#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/LuxCharacter.h"

void ULuxCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!IsValid(Pawn))
	{
		Speed = 0.0f;
		Direction = 0.0f;
		RevolverUpperBodyWeight = 0.0f;
		RevolverAimWeight = 0.0f;
		NeckLookRotation = FRotator::ZeroRotator;
		HeadLookRotation = FRotator::ZeroRotator;
		return;
	}

	FVector HorizontalVelocity = Pawn->GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	Speed = HorizontalVelocity.Size();

	if (Speed <= KINDA_SMALL_NUMBER)
	{
		Direction = 0.0f;
	}
	else
	{
		const FVector LocalVelocity = Pawn->GetActorQuat().UnrotateVector(HorizontalVelocity);
		Direction = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}

	const ALuxCharacter* Character = Cast<ALuxCharacter>(Pawn);
	const bool bUseRevolverUpperBody = Character && !Character->IsDead();
	RevolverUpperBodyWeight = FMath::FInterpTo(
		RevolverUpperBodyWeight,
		bUseRevolverUpperBody ? 1.0f : 0.0f,
		DeltaSeconds,
		12.0f
	);
	RevolverAimWeight = FMath::FInterpTo(
		RevolverAimWeight,
		Character && Character->IsAiming() ? 1.0f : 0.0f,
		DeltaSeconds,
		12.0f
	);

	const FRotator AimDelta = Character
		? UKismetMathLibrary::NormalizedDeltaRotator(
			Character->GetBaseAimRotation(),
			Character->GetActorRotation()
		)
		: FRotator::ZeroRotator;
	const float ClampedLookPitch = FMath::Clamp(AimDelta.Pitch, -50.0f, 50.0f);
	const float ClampedLookYaw = FMath::Clamp(AimDelta.Yaw, -60.0f, 60.0f);
	// CharacterMesh0 is yawed -90 degrees relative to the actor. In mesh component
	// space, camera pitch therefore rotates around -X (Roll), while Yaw remains Z.
	const FRotator TargetLookRotation(0.0f, ClampedLookYaw, -ClampedLookPitch);
	const FRotator SmoothedLook = FMath::RInterpTo(
		NeckLookRotation + HeadLookRotation,
		TargetLookRotation,
		DeltaSeconds,
		10.0f
	);
	NeckLookRotation = SmoothedLook * 0.35f;
	HeadLookRotation = SmoothedLook * 0.65f;
}
