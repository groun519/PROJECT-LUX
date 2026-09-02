#include "Player/LuxCharacterAnimInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/LuxCharacter.h"
#include "Weapons/LuxRevolver.h"

namespace
{
	const FName RightHandBone(TEXT("hand_r"));
	constexpr float AimTraceRange = 10000.0f;
}

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
		Spine01AimRotation = FRotator::ZeroRotator;
		Spine02AimRotation = FRotator::ZeroRotator;
		Spine03AimRotation = FRotator::ZeroRotator;
		RightHandAimRotation = FRotator::ZeroRotator;
		NeckLookRotation = FRotator::ZeroRotator;
		HeadLookRotation = FRotator::ZeroRotator;
		SmoothedLookRotation = FRotator::ZeroRotator;
		AimTargetWorldLocation = FVector::ZeroVector;
		MuzzleAimErrorDegrees = 0.0f;
		RawAimObstructionDistance = 0.0f;
		bAimConvergenceClamped = false;
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
	SmoothedLookRotation = FMath::RInterpTo(
		SmoothedLookRotation,
		TargetLookRotation,
		DeltaSeconds,
		10.0f
	);

	// While aiming, move most of the view delta into the torso so the arm does
	// not solve the entire correction at the wrist. Outside ADS the head keeps
	// the full look response used by the locomotion presentation.
	Spine01AimRotation = SmoothedLookRotation * 0.15f;
	Spine02AimRotation = SmoothedLookRotation * 0.20f;
	Spine03AimRotation = SmoothedLookRotation * 0.25f;
	const float HeadLookShare = 1.0f - (0.60f * RevolverAimWeight);
	NeckLookRotation = SmoothedLookRotation * (0.35f * HeadLookShare);
	HeadLookRotation = SmoothedLookRotation * (0.65f * HeadLookShare);

	UpdateRightHandAim(Character);
}

void ULuxCharacterAnimInstance::UpdateRightHandAim(const ALuxCharacter* Character)
{
	USkeletalMeshComponent* BodyMesh = GetSkelMeshComponent();
	if (!Character || !BodyMesh || !BodyMesh->DoesSocketExist(RightHandBone))
	{
		RightHandAimRotation = FRotator::ZeroRotator;
		AimTargetWorldLocation = FVector::ZeroVector;
		MuzzleAimErrorDegrees = 0.0f;
		RawAimObstructionDistance = 0.0f;
		bAimConvergenceClamped = false;
		return;
	}

	const FTransform HandWorldTransform = BodyMesh->GetSocketTransform(
		RightHandBone,
		RTS_World
	);
	RightHandAimRotation = BodyMesh->GetSocketTransform(
		RightHandBone,
		RTS_Component
	).Rotator();

	const ALuxRevolver* Revolver = Character->GetEquippedRevolver();
	FTransform MuzzleWorldTransform;
	if (
		RevolverAimWeight <= KINDA_SMALL_NUMBER
		|| !Revolver
		|| !Revolver->GetThirdPersonMuzzleTransform(MuzzleWorldTransform)
	)
	{
		AimTargetWorldLocation = FVector::ZeroVector;
		MuzzleAimErrorDegrees = 0.0f;
		RawAimObstructionDistance = 0.0f;
		bAimConvergenceClamped = false;
		return;
	}

	const FVector TraceStart = Character->GetPawnViewLocation();
	const FVector ViewForward = Character->GetBaseAimRotation().Vector();
	const FVector TraceEnd = TraceStart + ViewForward * AimTraceRange;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LuxThirdPersonAim), true, Character);
	QueryParams.AddIgnoredActor(Revolver);
	FHitResult HitResult;
	UWorld* World = Character->GetWorld();
	const bool bHit = World && World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);
	const FVector RawAimTarget = bHit ? HitResult.ImpactPoint : TraceEnd;
	RawAimObstructionDistance = FVector::DotProduct(RawAimTarget - TraceStart, ViewForward);
	const float PresentationConvergenceDistance = FMath::Max(
		RawAimObstructionDistance,
		MinimumAimConvergenceDistance
	);
	bAimConvergenceClamped = PresentationConvergenceDistance > RawAimObstructionDistance + KINDA_SMALL_NUMBER;
	AimTargetWorldLocation = TraceStart + ViewForward * PresentationConvergenceDistance;

	const FVector MuzzleForward = MuzzleWorldTransform.GetUnitAxis(EAxis::X);
	FVector DesiredMuzzleForward = (
		AimTargetWorldLocation - MuzzleWorldTransform.GetLocation()
	).GetSafeNormal();
	if (DesiredMuzzleForward.IsNearlyZero())
	{
		MuzzleAimErrorDegrees = 0.0f;
		return;
	}

	const float ViewDot = FMath::Clamp(
		FVector::DotProduct(ViewForward, DesiredMuzzleForward),
		-1.0f,
		1.0f
	);
	const float ParallaxDegrees = FMath::RadiansToDegrees(FMath::Acos(ViewDot));
	if (ParallaxDegrees > MaximumAimParallaxDegrees)
	{
		const FVector RotationAxis = FVector::CrossProduct(
			ViewForward,
			DesiredMuzzleForward
		).GetSafeNormal();
		DesiredMuzzleForward = RotationAxis.IsNearlyZero()
			? ViewForward
			: FQuat(
				RotationAxis,
				FMath::DegreesToRadians(MaximumAimParallaxDegrees)
			).RotateVector(ViewForward).GetSafeNormal();
		bAimConvergenceClamped = true;
	}

	MuzzleAimErrorDegrees = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(
			FVector::DotProduct(MuzzleForward, DesiredMuzzleForward),
			-1.0f,
			1.0f
		))
	);

	const FQuat DesiredMuzzleWorldRotation = FRotationMatrix::MakeFromXZ(
		DesiredMuzzleForward,
		MuzzleWorldTransform.GetUnitAxis(EAxis::Z)
	).ToQuat();
	const FQuat HandToMuzzleRotation =
		HandWorldTransform.GetRotation().Inverse() * MuzzleWorldTransform.GetRotation();
	const FQuat DesiredHandWorldRotation =
		DesiredMuzzleWorldRotation * HandToMuzzleRotation.Inverse();
	const FQuat DesiredHandComponentRotation =
		BodyMesh->GetComponentQuat().Inverse() * DesiredHandWorldRotation;
	RightHandAimRotation = DesiredHandComponentRotation.Rotator();
}
