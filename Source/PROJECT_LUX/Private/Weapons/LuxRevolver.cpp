#include "Weapons/LuxRevolver.h"

#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Player/LuxCharacter.h"
#include "TimerManager.h"

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
	SingleRoundReloadMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Reload/SingleBullet/AM_Reload_SingleBullet_Revolver.AM_Reload_SingleBullet_Revolver")
	));

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
	DOREPLIFETIME(ALuxRevolver, FireSequence);
	DOREPLIFETIME(ALuxRevolver, DryFireSequence);
	DOREPLIFETIME(ALuxRevolver, bRoundInsertionPending);
	DOREPLIFETIME(ALuxRevolver, ReloadSequence);
	DOREPLIFETIME(ALuxRevolver, RoundInsertSequence);
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

void ALuxRevolver::RequestFire()
{
	if (HasAuthority())
	{
		ServerFire_Implementation();
		return;
	}

	ServerFire();
}

void ALuxRevolver::RequestOpenCylinder()
{
	if (HasAuthority())
	{
		TryOpenCylinder();
		return;
	}

	ServerOpenCylinder();
}

void ALuxRevolver::RequestCloseCylinder()
{
	if (HasAuthority())
	{
		TryCloseCylinder();
		return;
	}

	ServerCloseCylinder();
}

void ALuxRevolver::RequestCancelReload()
{
	if (HasAuthority())
	{
		TryCancelReload();
		return;
	}

	ServerCancelReload();
}

bool ALuxRevolver::BeginRoundInsertion(ELuxRevolverRoundType RoundType)
{
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	const int32 EmptyChamberIndex = FindNextEmptyChamber(ReloadChamberIndex);
	if (
		!HasAuthority()
		|| !CanManipulateCylinder(EquippedCharacter)
		|| !bCylinderOpen
		|| bRoundInsertionPending
		|| RoundType <= ELuxRevolverRoundType::Empty
		|| RoundType > ELuxRevolverRoundType::Rubber
		|| !IsValidChamberIndex(EmptyChamberIndex)
	)
	{
		LastReloadResultForDevelopment = TEXT("Rejected");
		return false;
	}

	PendingRoundType = RoundType;
	PendingRoundChamberIndex = static_cast<uint8>(EmptyChamberIndex);
	bRoundInsertionPending = true;
	++ReloadSequence;
	LastReloadResultForDevelopment = TEXT("InsertPending");
	ForceNetUpdate();

	if (RoundInsertCommitDelaySeconds <= 0.0f || !GetWorld())
	{
		CommitPendingRoundInsertion();
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(
			RoundInsertionTimer,
			this,
			&ALuxRevolver::CommitPendingRoundInsertion,
			RoundInsertCommitDelaySeconds,
			false
		);
	}

	return true;
}

bool ALuxRevolver::IsRoundInsertionPending() const
{
	return bRoundInsertionPending;
}

int32 ALuxRevolver::GetReloadSequence() const
{
	return ReloadSequence;
}

int32 ALuxRevolver::GetRoundInsertSequence() const
{
	return RoundInsertSequence;
}

UAnimMontage* ALuxRevolver::GetSingleRoundReloadMontage() const
{
	return SingleRoundReloadMontage.LoadSynchronous();
}

int32 ALuxRevolver::GetFireSequence() const
{
	return FireSequence;
}

int32 ALuxRevolver::GetDryFireSequence() const
{
	return DryFireSequence;
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

FString ALuxRevolver::DescribeLastFireResultForDevelopment() const
{
	return HasAuthority() ? LastFireResultForDevelopment.ToString() : TEXT("AuthorityOnly");
}

ALuxCharacter* ALuxRevolver::GetLastNonLethalHitForDevelopment() const
{
	return HasAuthority() ? LastNonLethalHitForDevelopment.Get() : nullptr;
}

FString ALuxRevolver::DescribeLastReloadResultForDevelopment() const
{
	return HasAuthority() ? LastReloadResultForDevelopment.ToString() : TEXT("AuthorityOnly");
}

void ALuxRevolver::ServerFire_Implementation()
{
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	const UWorld* World = GetWorld();
	const double ServerTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (!CanFire(EquippedCharacter, ServerTimeSeconds))
	{
		LastFireResultForDevelopment = TEXT("Rejected");
		return;
	}

	LastFireServerTimeSeconds = ServerTimeSeconds;
	LastNonLethalHitForDevelopment.Reset();
	const ELuxRevolverRoundType RoundType = ChamberRoundTypes[CurrentChamberIndex];

	if (RoundType == ELuxRevolverRoundType::Empty)
	{
		++DryFireSequence;
		LastFireResultForDevelopment = TEXT("DryFire");
		OnDryFire.Broadcast(this);
		ForceNetUpdate();
		return;
	}

	ALuxCharacter* HitCharacter = nullptr;
	if (RoundType == ELuxRevolverRoundType::Live || RoundType == ELuxRevolverRoundType::Rubber)
	{
		HitCharacter = TraceCharacter(EquippedCharacter);
	}

	if (RoundType == ELuxRevolverRoundType::Live)
	{
		LastFireResultForDevelopment = TEXT("Live");
		if (HitCharacter)
		{
			HitCharacter->Die();
		}
	}
	else if (RoundType == ELuxRevolverRoundType::Blank)
	{
		LastFireResultForDevelopment = TEXT("Blank");
	}
	else
	{
		LastFireResultForDevelopment = TEXT("Rubber");
		if (HitCharacter)
		{
			LastNonLethalHitForDevelopment = HitCharacter;
			OnNonLethalHit.Broadcast(this, HitCharacter);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("LuxRevolver: Rubber round registered non-lethal hit on [%s]."),
				*HitCharacter->GetName()
			);
		}
	}

	++FireSequence;
	ConsumeCurrentRoundAndAdvance();
	ForceNetUpdate();
}

void ALuxRevolver::ServerOpenCylinder_Implementation()
{
	TryOpenCylinder();
}

void ALuxRevolver::ServerCloseCylinder_Implementation()
{
	TryCloseCylinder();
}

void ALuxRevolver::ServerCancelReload_Implementation()
{
	TryCancelReload();
}

bool ALuxRevolver::CanFire(const ALuxCharacter* EquippedCharacter, double ServerTimeSeconds) const
{
	return HasAuthority()
		&& EquippedCharacter
		&& EquippedCharacter->GetEquippedRevolver() == this
		&& !EquippedCharacter->IsDead()
		&& !bCylinderOpen
		&& IsValidChamberIndex(CurrentChamberIndex)
		&& (
			LastFireServerTimeSeconds < 0.0
			|| ServerTimeSeconds - LastFireServerTimeSeconds >= MinimumFireIntervalSeconds
		);
}

bool ALuxRevolver::CanManipulateCylinder(const ALuxCharacter* EquippedCharacter) const
{
	return HasAuthority()
		&& EquippedCharacter
		&& EquippedCharacter->GetEquippedRevolver() == this
		&& !EquippedCharacter->IsDead();
}

void ALuxRevolver::ClearPendingRoundInsertion()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RoundInsertionTimer);
	}

	bRoundInsertionPending = false;
	PendingRoundType = ELuxRevolverRoundType::Empty;
	PendingRoundChamberIndex = ChamberCount;
}

void ALuxRevolver::CommitPendingRoundInsertion()
{
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	const ELuxRevolverRoundType RoundType = PendingRoundType;
	const int32 ChamberIndex = PendingRoundChamberIndex;
	ClearPendingRoundInsertion();

	if (
		!CanManipulateCylinder(EquippedCharacter)
		|| !bCylinderOpen
		|| RoundType <= ELuxRevolverRoundType::Empty
		|| RoundType > ELuxRevolverRoundType::Rubber
		|| !IsValidChamberIndex(ChamberIndex)
		|| ChamberRoundTypes[ChamberIndex] != ELuxRevolverRoundType::Empty
	)
	{
		LastReloadResultForDevelopment = TEXT("Rejected");
		ForceNetUpdate();
		return;
	}

	ChamberRoundTypes[ChamberIndex] = RoundType;
	ReloadChamberIndex = static_cast<uint8>((ChamberIndex + 1) % ChamberCount);
	++RoundInsertSequence;
	LastReloadResultForDevelopment = TEXT("Inserted");
	RefreshLoadedMask();
	ForceNetUpdate();
}

int32 ALuxRevolver::FindNextEmptyChamber(int32 StartIndex) const
{
	if (!IsValidChamberIndex(StartIndex))
	{
		return INDEX_NONE;
	}

	for (int32 Offset = 0; Offset < ChamberCount; ++Offset)
	{
		const int32 ChamberIndex = (StartIndex + Offset) % ChamberCount;
		if (ChamberRoundTypes[ChamberIndex] == ELuxRevolverRoundType::Empty)
		{
			return ChamberIndex;
		}
	}

	return INDEX_NONE;
}

bool ALuxRevolver::IsValidChamberIndex(int32 ChamberIndex) const
{
	return ChamberIndex >= 0 && ChamberIndex < ChamberCount;
}

bool ALuxRevolver::TryCancelReload()
{
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	if (!CanManipulateCylinder(EquippedCharacter) || !bCylinderOpen)
	{
		LastReloadResultForDevelopment = TEXT("Rejected");
		return false;
	}

	ClearPendingRoundInsertion();
	bCylinderOpen = false;
	LastReloadResultForDevelopment = TEXT("Cancelled");
	ForceNetUpdate();
	return true;
}

bool ALuxRevolver::TryCloseCylinder()
{
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	if (!CanManipulateCylinder(EquippedCharacter) || !bCylinderOpen || bRoundInsertionPending)
	{
		LastReloadResultForDevelopment = TEXT("Rejected");
		return false;
	}

	bCylinderOpen = false;
	LastReloadResultForDevelopment = TEXT("Closed");
	ForceNetUpdate();
	return true;
}

bool ALuxRevolver::TryOpenCylinder()
{
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	if (!CanManipulateCylinder(EquippedCharacter) || bCylinderOpen || bRoundInsertionPending)
	{
		LastReloadResultForDevelopment = TEXT("Rejected");
		return false;
	}

	bCylinderOpen = true;
	ReloadChamberIndex = CurrentChamberIndex;
	LastReloadResultForDevelopment = TEXT("Opened");
	ForceNetUpdate();
	return true;
}

ALuxCharacter* ALuxRevolver::TraceCharacter(const ALuxCharacter* EquippedCharacter) const
{
	if (!EquippedCharacter || !GetWorld())
	{
		return nullptr;
	}

	FVector TraceStart;
	FRotator ViewRotation;
	EquippedCharacter->GetActorEyesViewPoint(TraceStart, ViewRotation);
	const FVector TraceEnd = TraceStart + ViewRotation.Vector() * TraceRange;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LuxRevolverFire), true, EquippedCharacter);
	QueryParams.AddIgnoredActor(this);
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);
	return Cast<ALuxCharacter>(HitResult.GetActor());
}

void ALuxRevolver::ConsumeCurrentRoundAndAdvance()
{
	ChamberRoundTypes[CurrentChamberIndex] = ELuxRevolverRoundType::Empty;
	CurrentChamberIndex = static_cast<uint8>((CurrentChamberIndex + 1) % ChamberCount);
	RefreshLoadedMask();
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
