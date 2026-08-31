#include "Weapons/LuxRevolver.h"

#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const TCHAR* LexToString(ELuxRevolverRoundType RoundType)
	{
		switch (RoundType)
		{
		case ELuxRevolverRoundType::Empty:
			return TEXT("Empty");
		case ELuxRevolverRoundType::Live:
			return TEXT("Live");
		case ELuxRevolverRoundType::Blank:
			return TEXT("Blank");
		case ELuxRevolverRoundType::Rubber:
			return TEXT("Rubber");
		default:
			return TEXT("Unknown");
		}
	}
}

ALuxRevolver::ALuxRevolver()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetUseOwnerRelevancy = true;
	SetReplicates(true);
	SetReplicateMovement(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	for (ELuxRevolverRoundType& RoundType : ChamberRoundTypes)
	{
		RoundType = ELuxRevolverRoundType::Empty;
	}
}

void ALuxRevolver::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALuxRevolver, LoadedMask);
	DOREPLIFETIME(ALuxRevolver, CurrentChamberIndex);
	DOREPLIFETIME(ALuxRevolver, bCylinderOpen);
}

int32 ALuxRevolver::GetChamberCount() const
{
	return ChamberCount;
}

uint8 ALuxRevolver::GetLoadedMask() const
{
	return LoadedMask;
}

uint8 ALuxRevolver::GetCurrentChamberIndex() const
{
	return CurrentChamberIndex;
}

bool ALuxRevolver::IsCylinderOpen() const
{
	return bCylinderOpen;
}

bool ALuxRevolver::IsChamberLoaded(int32 ChamberIndex) const
{
	return IsValidChamberIndex(ChamberIndex) && (LoadedMask & (1u << ChamberIndex)) != 0;
}

ELuxRevolverRoundType ALuxRevolver::GetRoundTypeForAuthority(int32 ChamberIndex) const
{
	if (!HasAuthority() || !IsValidChamberIndex(ChamberIndex))
	{
		return ELuxRevolverRoundType::Empty;
	}

	return ChamberRoundTypes[ChamberIndex];
}

bool ALuxRevolver::SetChamberRoundTypeForDevelopment(int32 ChamberIndex, ELuxRevolverRoundType RoundType)
{
	if (
		!HasAuthority()
		|| !IsValidChamberIndex(ChamberIndex)
		|| RoundType > ELuxRevolverRoundType::Rubber
	)
	{
		return false;
	}

	ChamberRoundTypes[ChamberIndex] = RoundType;
	RefreshLoadedMask();
	ForceNetUpdate();
	return true;
}

bool ALuxRevolver::SetPublicStateForDevelopment(int32 ChamberIndex, bool bOpen)
{
	if (!HasAuthority() || !IsValidChamberIndex(ChamberIndex))
	{
		return false;
	}

	CurrentChamberIndex = static_cast<uint8>(ChamberIndex);
	bCylinderOpen = bOpen;
	ForceNetUpdate();
	return true;
}

FString ALuxRevolver::DescribeChambersForDevelopment() const
{
	if (!HasAuthority())
	{
		return TEXT("AuthorityOnly");
	}

	TArray<FString> ChamberDescriptions;
	ChamberDescriptions.Reserve(ChamberCount);
	for (const ELuxRevolverRoundType RoundType : ChamberRoundTypes)
	{
		ChamberDescriptions.Emplace(LexToString(RoundType));
	}

	return FString::Join(ChamberDescriptions, TEXT(","));
}

bool ALuxRevolver::IsValidChamberIndex(int32 ChamberIndex) const
{
	return ChamberIndex >= 0 && ChamberIndex < ChamberCount;
}

void ALuxRevolver::RefreshLoadedMask()
{
	uint8 NewLoadedMask = 0;
	for (int32 ChamberIndex = 0; ChamberIndex < ChamberCount; ++ChamberIndex)
	{
		if (ChamberRoundTypes[ChamberIndex] != ELuxRevolverRoundType::Empty)
		{
			NewLoadedMask |= static_cast<uint8>(1u << ChamberIndex);
		}
	}

	LoadedMask = NewLoadedMask;
}
