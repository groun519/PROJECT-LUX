#include "Weapons/LuxRevolverAnimInstance.h"

#include "Weapons/LuxRevolver.h"

void ULuxRevolverAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ObservedChamberIndex = INDEX_NONE;
	TargetDrumRotationDegrees = 0.0f;
	DrumRotationDegrees = 0.0f;
}

void ULuxRevolverAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ALuxRevolver* Revolver = Cast<ALuxRevolver>(GetOwningActor());
	if (!IsValid(Revolver))
	{
		ObservedChamberIndex = INDEX_NONE;
		TargetDrumRotationDegrees = 0.0f;
		DrumRotationDegrees = 0.0f;
		Bullet1Scale = FVector::ZeroVector;
		Bullet2Scale = FVector::ZeroVector;
		Bullet3Scale = FVector::ZeroVector;
		Bullet4Scale = FVector::ZeroVector;
		Bullet5Scale = FVector::ZeroVector;
		Bullet6Scale = FVector::ZeroVector;
		return;
	}

	const int32 ChamberIndex = Revolver->GetCurrentChamberIndex();
	Bullet1Scale = Revolver->IsChamberLoaded(0) ? FVector::OneVector : FVector::ZeroVector;
	Bullet2Scale = Revolver->IsChamberLoaded(1) ? FVector::OneVector : FVector::ZeroVector;
	Bullet3Scale = Revolver->IsChamberLoaded(2) ? FVector::OneVector : FVector::ZeroVector;
	Bullet4Scale = Revolver->IsChamberLoaded(3) ? FVector::OneVector : FVector::ZeroVector;
	Bullet5Scale = Revolver->IsChamberLoaded(4) ? FVector::OneVector : FVector::ZeroVector;
	Bullet6Scale = Revolver->IsChamberLoaded(5) ? FVector::OneVector : FVector::ZeroVector;
	if (ObservedChamberIndex == INDEX_NONE)
	{
		ObservedChamberIndex = ChamberIndex;
		TargetDrumRotationDegrees = ChamberIndex * 60.0f * DrumRotationDirection;
		DrumRotationDegrees = TargetDrumRotationDegrees;
	}
	else if (ObservedChamberIndex != ChamberIndex)
	{
		const int32 AdvancedSteps =
			(ChamberIndex - ObservedChamberIndex + ALuxRevolver::ChamberCount)
			% ALuxRevolver::ChamberCount;
		TargetDrumRotationDegrees += AdvancedSteps * 60.0f * DrumRotationDirection;
		ObservedChamberIndex = ChamberIndex;
	}

	DrumRotationDegrees = FMath::FInterpConstantTo(
		DrumRotationDegrees,
		TargetDrumRotationDegrees,
		DeltaSeconds,
		DrumRotationSpeedDegreesPerSecond
	);

	// Keep long-running sessions numerically stable after an interpolation completes.
	if (
		FMath::IsNearlyEqual(DrumRotationDegrees, TargetDrumRotationDegrees, KINDA_SMALL_NUMBER)
		&& FMath::Abs(TargetDrumRotationDegrees) >= 3600.0f
	)
	{
		TargetDrumRotationDegrees = FMath::Fmod(TargetDrumRotationDegrees, 360.0f);
		DrumRotationDegrees = TargetDrumRotationDegrees;
	}
}
