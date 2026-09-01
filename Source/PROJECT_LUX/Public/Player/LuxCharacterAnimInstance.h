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
	FRotator NeckLookRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|ThirdPerson")
	FRotator HeadLookRotation = FRotator::ZeroRotator;
};
