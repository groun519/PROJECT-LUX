#pragma once

#include "Animation/AnimInstance.h"
#include "CoreMinimal.h"
#include "LuxFirstPersonAnimInstance.generated.h"

UCLASS(Transient, Blueprintable)
class PROJECT_LUX_API ULuxFirstPersonAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetAiming(bool bNewAiming);

	UFUNCTION(BlueprintPure, Category = "Revolver|Presentation")
	bool IsAiming() const;

	// R21's project-owned derived AnimBP reads this typed property instead of a reflected vendor variable.
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Revolver|Presentation")
	bool LuxAim = false;
};
