#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "LuxRevolverAnimInstance.generated.h"

UCLASS(Transient, Blueprintable)
class PROJECT_LUX_API ULuxRevolverAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// Connect this value to an additive Transform (Modify) Bone node for Drum.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	float DrumRotationDegrees = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Revolver|Presentation", meta = (ClampMin = "60.0"))
	float DrumRotationSpeedDegreesPerSecond = 720.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Revolver|Presentation", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float DrumRotationDirection = 1.0f;

	// These replace R21's animation-authored bullet scales after the montage slot.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	FVector Bullet1Scale = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	FVector Bullet2Scale = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	FVector Bullet3Scale = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	FVector Bullet4Scale = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	FVector Bullet5Scale = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	FVector Bullet6Scale = FVector::ZeroVector;

private:
	int32 ObservedChamberIndex = INDEX_NONE;
	float TargetDrumRotationDegrees = 0.0f;
};
