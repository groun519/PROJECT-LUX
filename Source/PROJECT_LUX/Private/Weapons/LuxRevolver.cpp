#include "Weapons/LuxRevolver.h"

#include "Animation/AnimMontage.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Player/LuxCharacter.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName R21WeaponSocket(TEXT("38"));
	const FName R21MuzzleSocket(TEXT("weapon_r_muzzle"));

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

	FirstPersonWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonWeaponMesh"));
	FirstPersonWeaponMesh->SetupAttachment(RootComponent);
	FirstPersonWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonWeaponMesh->SetGenerateOverlapEvents(false);
	FirstPersonWeaponMesh->SetOnlyOwnerSee(true);
	FirstPersonWeaponMesh->SetCastShadow(false);
	FirstPersonWeaponMesh->bCastDynamicShadow = false;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshFinder(
		TEXT("/Game/RevolverFPGM/System/FPWeapon/Mesh/Revolver/SKM_Revolver_NoClip.SKM_Revolver_NoClip"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> WeaponAnimClassFinder(
		TEXT("/Game/RevolverFPGM/System/FPWeapon/Mesh/Revolver/ABP_Revolver"));
	if (WeaponMeshFinder.Succeeded())
	{
		FirstPersonWeaponMesh->SetSkeletalMeshAsset(WeaponMeshFinder.Object);
	}
	if (WeaponAnimClassFinder.Succeeded())
	{
		FirstPersonWeaponMesh->SetAnimInstanceClass(WeaponAnimClassFinder.Class);
	}

	SingleRoundReloadMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Reload/SingleBullet/AM_Reload_SingleBullet_Revolver.AM_Reload_SingleBullet_Revolver")
	));
	WeaponSingleRoundReloadMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Reload/SingleBullet/AM_Wpn_Reload_SingleBullet_Revolver.AM_Wpn_Reload_SingleBullet_Revolver")
	));
	AimFireMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Fire/Aim/Montages/AM_AimFire_Standard_Revolver.AM_AimFire_Standard_Revolver")
	));
	WeaponAimFireMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Fire/Aim/Montages/A_Wpn_AimFire_Standard_Revolver_Montage.A_Wpn_AimFire_Standard_Revolver_Montage")
	));
	HipFireMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Fire/NoAim/Montages/AM_Fire_NoAim_Standard_Revolver.AM_Fire_NoAim_Standard_Revolver")
	));
	WeaponHipFireMontage = TSoftObjectPtr<UAnimMontage>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Animation/Fire/NoAim/Montages/A_Wpn_Fire_NoAim_Standard_Revolver_Montage.A_Wpn_Fire_NoAim_Standard_Revolver_Montage")
	));
	MuzzleFlashSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/MuzzleFlash/MuzzleFlash/Niagara/NS_MuzzleFlash.NS_MuzzleFlash")
	));
	FireSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Sounds/Revolver/Cue/SC_Fire_Revolver.SC_Fire_Revolver")
	));
	DryFireSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Sounds/Revolver/Cue/SC_Trigger_Revolver.SC_Trigger_Revolver")
	));
	CylinderOpenSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Sounds/Revolver/Cue/SC_Door_Revolver.SC_Door_Revolver")
	));
	CylinderCloseSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Sounds/Revolver/Cue/SC_Close_Drum_Revolver.SC_Close_Drum_Revolver")
	));
	RoundInsertSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(
		TEXT("/Game/RevolverFPGM/System/Sounds/Revolver/Cue/SC_PutBullet_Revolver.SC_PutBullet_Revolver")
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
	const ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	if (
		IsLocallyPresented()
		&& EquippedCharacter
		&& !EquippedCharacter->IsDead()
		&& !bCylinderOpen
		&& IsValidChamberIndex(CurrentChamberIndex)
	)
	{
		// Occupancy is public, but the exact Live / Blank / Rubber type remains server-only.
		PlayFirePresentation(!IsChamberLoaded(CurrentChamberIndex));
	}

	if (HasAuthority())
	{
		ServerFire_Implementation();
		return;
	}

	ServerFire();
}

void ALuxRevolver::AttachFirstPersonVisualTo(USkeletalMeshComponent* FirstPersonArms)
{
	if (!FirstPersonWeaponMesh || !FirstPersonArms)
	{
		return;
	}

	FirstPersonWeaponMesh->AttachToComponent(
		FirstPersonArms,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		R21WeaponSocket
	);
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

void ALuxRevolver::OnRep_CylinderOpen(bool bWasCylinderOpen)
{
	if (bWasCylinderOpen != bCylinderOpen && IsLocallyPresented())
	{
		PlayCylinderPresentation(bCylinderOpen);
		if (!bCylinderOpen)
		{
			StopReloadPresentation();
		}
	}
}

void ALuxRevolver::OnRep_FireSequence()
{
	// The owning client already predicts its owner-only presentation in RequestFire.
}

void ALuxRevolver::OnRep_DryFireSequence()
{
	// The owning client already predicts its owner-only presentation in RequestFire.
}

void ALuxRevolver::OnRep_ReloadSequence()
{
	if (IsLocallyPresented())
	{
		PlayReloadPresentation();
	}
}

void ALuxRevolver::OnRep_RoundInsertSequence()
{
	if (IsLocallyPresented())
	{
		PlayRoundInsertPresentation();
	}
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
	if (IsLocallyPresented())
	{
		PlayReloadPresentation();
	}

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
	if (IsLocallyPresented())
	{
		PlayRoundInsertPresentation();
	}
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

bool ALuxRevolver::IsLocallyPresented() const
{
	const ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	return EquippedCharacter && EquippedCharacter->IsLocallyControlled();
}

void ALuxRevolver::PlayCylinderPresentation(bool bNowOpen)
{
	PlaySoundForOwner((bNowOpen ? CylinderOpenSound : CylinderCloseSound).LoadSynchronous());
}

void ALuxRevolver::PlayFirePresentation(bool bDryFire)
{
	if (!IsLocallyPresented())
	{
		return;
	}

	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	if (bDryFire)
	{
		PlaySoundForOwner(DryFireSound.LoadSynchronous(), R21WeaponSocket);
		return;
	}

	const bool bAiming = EquippedCharacter && EquippedCharacter->IsAiming();
	if (EquippedCharacter)
	{
		EquippedCharacter->PlayFirstPersonMontage(
			(bAiming ? AimFireMontage : HipFireMontage).LoadSynchronous()
		);
	}
	if (FirstPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonWeaponMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(
				(bAiming ? WeaponAimFireMontage : WeaponHipFireMontage).LoadSynchronous()
			);
		}
	}

	PlaySoundForOwner(FireSound.LoadSynchronous(), R21MuzzleSocket);
	if (EquippedCharacter)
	{
		USkeletalMeshComponent* FirstPersonArms = EquippedCharacter->GetFirstPersonArms();
		UNiagaraSystem* MuzzleFlash = MuzzleFlashSystem.LoadSynchronous();
		if (FirstPersonArms && MuzzleFlash)
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				MuzzleFlash,
				FirstPersonArms,
				R21MuzzleSocket,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
			);
		}
	}
}

void ALuxRevolver::PlayReloadPresentation()
{
	if (!IsLocallyPresented())
	{
		return;
	}

	if (ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		EquippedCharacter->PlayFirstPersonMontage(SingleRoundReloadMontage.LoadSynchronous(), 1.3f);
	}
	if (FirstPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonWeaponMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(WeaponSingleRoundReloadMontage.LoadSynchronous(), 1.3f);
		}
	}
}

void ALuxRevolver::PlayRoundInsertPresentation()
{
	PlaySoundForOwner(RoundInsertSound.LoadSynchronous(), R21WeaponSocket);
}

void ALuxRevolver::PlaySoundForOwner(USoundBase* Sound, FName SocketName) const
{
	const ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	USkeletalMeshComponent* FirstPersonArms = EquippedCharacter ? EquippedCharacter->GetFirstPersonArms() : nullptr;
	if (IsLocallyPresented() && Sound && FirstPersonArms)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, FirstPersonArms, SocketName);
	}
}

void ALuxRevolver::StopReloadPresentation()
{
	if (ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		EquippedCharacter->StopFirstPersonMontages();
	}
	if (FirstPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonWeaponMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.15f);
		}
	}
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
	if (IsLocallyPresented())
	{
		PlayCylinderPresentation(false);
		StopReloadPresentation();
	}
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
	if (IsLocallyPresented())
	{
		PlayCylinderPresentation(false);
		StopReloadPresentation();
	}
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
	if (IsLocallyPresented())
	{
		PlayCylinderPresentation(true);
	}
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
