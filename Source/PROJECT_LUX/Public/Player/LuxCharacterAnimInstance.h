#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "LuxCharacterAnimInstance.generated.h"

UCLASS(Transient, Blueprintable)
class PROJECT_LUX_API ULuxCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	float RevolverUpperBodyWeight = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	float RevolverAimWeight = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator Spine01AimRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator Spine02AimRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator Spine03AimRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator RightHandAimRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator NeckLookRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator HeadLookRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson|Debug")
	FVector AimTargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson|Debug")
	float MuzzleAimErrorDegrees = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson|Debug")
	float RawAimObstructionDistance = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson|Debug")
	bool bAimConvergenceClamped = false;

private:
	void UpdateRightHandAim(const class ALuxCharacter* Character);

	FRotator SmoothedLookRotation = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|ThirdPerson|Aim", meta = (ClampMin = "50.0"))
	float MinimumAimConvergenceDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|ThirdPerson|Aim", meta = (ClampMin = "1.0", ClampMax = "45.0"))
	float MaximumAimParallaxDegrees = 20.0f;
};
