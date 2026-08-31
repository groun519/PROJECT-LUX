#include "Player/LuxFirstPersonAnimInstance.h"

void ULuxFirstPersonAnimInstance::SetAiming(bool bNewAiming)
{
	LuxAim = bNewAiming;
}

bool ULuxFirstPersonAnimInstance::IsAiming() const
{
	return LuxAim;
}
