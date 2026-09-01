#include "Weapons/LuxRevolver.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
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
	const FName ThirdPersonHandBone(TEXT("hand_r"));
	const FVector R21WeaponGripLocation(-8.018609, 3.509527, -1.799098);
	const FRotator R21WeaponGripRotation(17.229283, 72.287198, 1.505405);
	const FVector R21WeaponMuzzleOffset(45.0, 0.0, 25.0);

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

	ThirdPersonWeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ThirdPersonWeaponMesh"));
	ThirdPersonWeaponMesh->SetupAttachment(RootComponent);
	ThirdPersonWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThirdPersonWeaponMesh->SetGenerateOverlapEvents(false);
	ThirdPersonWeaponMesh->SetOwnerNoSee(true);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMeshFinder(
		TEXT("/Game/RevolverFPGM/System/FPWeapon/Mesh/Revolver/SKM_Revolver_NoClip.SKM_Revolver_NoClip"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> WeaponAnimClassFinder(
		TEXT("/Game/RevolverFPGM/System/FPWeapon/Mesh/Revolver/ABP_Revolver"));
	if (WeaponMeshFinder.Succeeded())
	{
		FirstPersonWeaponMesh->SetSkeletalMeshAsset(WeaponMeshFinder.Object);
		ThirdPersonWeaponMesh->SetSkeletalMeshAsset(WeaponMeshFinder.Object);
	}
	if (WeaponAnimClassFinder.Succeeded())
	{
		FirstPersonWeaponMesh->SetAnimInstanceClass(WeaponAnimClassFinder.Class);
		ThirdPersonWeaponMesh->SetAnimInstanceClass(WeaponAnimClassFinder.Class);
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
	ThirdPersonHipFireAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(
		TEXT("/Game/LUX/Animation/Revolver/ThirdPerson/A_Lux_TP_Revolver_FireHip.A_Lux_TP_Revolver_FireHip")
	));
	ThirdPersonAimFireAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(
		TEXT("/Game/LUX/Animation/Revolver/ThirdPerson/A_Lux_TP_Revolver_FireAim.A_Lux_TP_Revolver_FireAim")
	));
	ThirdPersonReloadAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(
		TEXT("/Game/LUX/Animation/Revolver/ThirdPerson/A_Lux_TP_Revolver_ReloadSingle.A_Lux_TP_Revolver_ReloadSingle")
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

void ALuxRevolver::BeginPlay()
{
	Super::BeginPlay();
	ResolvePresentationAssets();
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

void ALuxRevolver::PostNetInit()
{
	Super::PostNetInit();
	// Initial JIP state is not a new cosmetic event.
	bPresentationReplicationReady = true;
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
	uint16 RequestId = 0;
	if (IsLocallyPresented())
	{
		do
		{
			++NextLocalFireRequestId;
		} while (NextLocalFireRequestId == 0);
		RequestId = NextLocalFireRequestId;

		const UWorld* World = GetWorld();
		const double LocalTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
		const ELocalFirePrediction Prediction = PredictLocalFire(LocalTimeSeconds);
		LocalFirePredictions.Add(RequestId, Prediction);
		if (Prediction != ELocalFirePrediction::None)
		{
			PlayFirePresentation(Prediction == ELocalFirePrediction::Dry);
		}
	}

	if (HasAuthority())
	{
		bool bDryFire = false;
		const bool bAccepted = ResolveServerFire(bDryFire);
		if (IsLocallyPresented())
		{
			ConfirmLocalFire(RequestId, bAccepted, bDryFire);
		}
		else
		{
			ClientConfirmFire(RequestId, bAccepted, bDryFire);
		}
		return;
	}

	ServerFire(RequestId);
}

void ALuxRevolver::AttachPresentationVisualsTo(
	USkeletalMeshComponent* FirstPersonArms,
	USkeletalMeshComponent* ThirdPersonBody
)
{
	if (FirstPersonWeaponMesh && FirstPersonArms)
	{
		ensureMsgf(
			FirstPersonArms->DoesSocketExist(R21WeaponSocket),
			TEXT("R21 First Person Arms is missing required weapon socket '%s'."),
			*R21WeaponSocket.ToString()
		);
		ensureMsgf(
			FirstPersonArms->DoesSocketExist(R21MuzzleSocket),
			TEXT("R21 First Person Arms is missing required muzzle socket '%s'."),
			*R21MuzzleSocket.ToString()
		);
		FirstPersonWeaponMesh->AttachToComponent(
			FirstPersonArms,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			R21WeaponSocket
		);
	}

	if (ThirdPersonWeaponMesh && ThirdPersonBody)
	{
		ThirdPersonWeaponMesh->AttachToComponent(
			ThirdPersonBody,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			ThirdPersonHandBone
		);
		// Reuse the R21 hand socket offset without modifying the Marketplace skeleton.
		ThirdPersonWeaponMesh->SetRelativeLocationAndRotation(
			R21WeaponGripLocation,
			R21WeaponGripRotation
		);
	}
}

void ALuxRevolver::StopOwnerPresentation()
{
	StopReloadPresentation();
	StopThirdPersonReloadPresentation();
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
	if (!bPresentationReplicationReady || bWasCylinderOpen == bCylinderOpen)
	{
		return;
	}

	if (IsLocallyPresented())
	{
		PlayCylinderPresentation(bCylinderOpen);
	}
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonCylinderPresentation(bCylinderOpen);
		if (!bCylinderOpen)
		{
			StopThirdPersonReloadPresentation();
		}
	}
}

void ALuxRevolver::OnRep_FireSequence()
{
	// The owner reconciles by request id; non-owners consume the generic replicated event.
	if (bPresentationReplicationReady && IsRemotelyPresented())
	{
		PlayThirdPersonFirePresentation(false);
	}
}

void ALuxRevolver::OnRep_DryFireSequence()
{
	// The owner reconciles by request id; non-owners consume the generic replicated event.
	if (bPresentationReplicationReady && IsRemotelyPresented())
	{
		PlayThirdPersonFirePresentation(true);
	}
}

void ALuxRevolver::OnRep_ReloadSequence()
{
	if (!bPresentationReplicationReady)
	{
		return;
	}

	if (IsLocallyPresented())
	{
		PlayReloadPresentation();
	}
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonReloadPresentation();
	}
}

void ALuxRevolver::OnRep_RoundInsertSequence()
{
	if (!bPresentationReplicationReady)
	{
		return;
	}

	if (IsLocallyPresented())
	{
		PlayRoundInsertPresentation();
	}
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonRoundInsertPresentation();
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
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonReloadPresentation();
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
	return ResolvedSingleRoundReloadMontage;
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

int32 ALuxRevolver::GetOwnerFirePresentationCountForDevelopment() const
{
	return OwnerFirePresentationCount;
}

int32 ALuxRevolver::GetOwnerDryFirePresentationCountForDevelopment() const
{
	return OwnerDryFirePresentationCount;
}

void ALuxRevolver::ServerFire_Implementation(uint16 RequestId)
{
	bool bDryFire = false;
	const bool bAccepted = ResolveServerFire(bDryFire);
	ClientConfirmFire(RequestId, bAccepted, bDryFire);
}

void ALuxRevolver::ClientConfirmFire_Implementation(uint16 RequestId, bool bAccepted, bool bDryFire)
{
	ConfirmLocalFire(RequestId, bAccepted, bDryFire);
}

void ALuxRevolver::ConfirmLocalFire(uint16 RequestId, bool bAccepted, bool bDryFire)
{
	const ELocalFirePrediction* Prediction = LocalFirePredictions.Find(RequestId);
	if (bAccepted)
	{
		const ELocalFirePrediction AuthoritativeResult =
			bDryFire ? ELocalFirePrediction::Dry : ELocalFirePrediction::Loaded;
		if (!Prediction || *Prediction == ELocalFirePrediction::None)
		{
			PlayFirePresentation(bDryFire);
		}
		else if (*Prediction != AuthoritativeResult)
		{
			// Public chamber replication will converge the next prediction. Do not double-play this shot.
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("LuxRevolver: local fire prediction disagreed with authoritative public result.")
			);
		}
	}

	LocalFirePredictions.Remove(RequestId);
}

bool ALuxRevolver::ResolveServerFire(bool& bOutDryFire)
{
	bOutDryFire = false;
	ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	const UWorld* World = GetWorld();
	const double ServerTimeSeconds = World ? World->GetTimeSeconds() : 0.0;
	if (!CanFire(EquippedCharacter, ServerTimeSeconds))
	{
		LastFireResultForDevelopment = TEXT("Rejected");
		return false;
	}

	LastFireServerTimeSeconds = ServerTimeSeconds;
	LastNonLethalHitForDevelopment.Reset();
	const ELuxRevolverRoundType RoundType = ChamberRoundTypes[CurrentChamberIndex];

	if (RoundType == ELuxRevolverRoundType::Empty)
	{
		bOutDryFire = true;
		++DryFireSequence;
		LastFireResultForDevelopment = TEXT("DryFire");
		OnDryFire.Broadcast(this);
		ForceNetUpdate();
		if (IsRemotelyPresented())
		{
			PlayThirdPersonFirePresentation(true);
		}
		return true;
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
	if (IsRemotelyPresented())
	{
		PlayThirdPersonFirePresentation(false);
	}
	return true;
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

ALuxRevolver::ELocalFirePrediction ALuxRevolver::PredictLocalFire(double LocalTimeSeconds)
{
	const ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	if (
		!IsLocallyPresented()
		|| !EquippedCharacter
		|| EquippedCharacter->GetEquippedRevolver() != this
		|| EquippedCharacter->IsDead()
		|| bCylinderOpen
		|| !IsValidChamberIndex(CurrentChamberIndex)
		|| (
			LastLocalFirePresentationTimeSeconds >= 0.0
			&& LocalTimeSeconds - LastLocalFirePresentationTimeSeconds < MinimumFireIntervalSeconds
		)
	)
	{
		return ELocalFirePrediction::None;
	}

	LastLocalFirePresentationTimeSeconds = LocalTimeSeconds;
	// Occupancy is public; exact Live / Blank / Rubber identity remains server-only.
	return IsChamberLoaded(CurrentChamberIndex)
		? ELocalFirePrediction::Loaded
		: ELocalFirePrediction::Dry;
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
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonRoundInsertPresentation();
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

void ALuxRevolver::ResolvePresentationAssets()
{
	ResolvedSingleRoundReloadMontage = SingleRoundReloadMontage.LoadSynchronous();
	ResolvedWeaponSingleRoundReloadMontage = WeaponSingleRoundReloadMontage.LoadSynchronous();
	ResolvedAimFireMontage = AimFireMontage.LoadSynchronous();
	ResolvedWeaponAimFireMontage = WeaponAimFireMontage.LoadSynchronous();
	ResolvedHipFireMontage = HipFireMontage.LoadSynchronous();
	ResolvedWeaponHipFireMontage = WeaponHipFireMontage.LoadSynchronous();
	ResolvedThirdPersonHipFireAnimation = ThirdPersonHipFireAnimation.LoadSynchronous();
	ResolvedThirdPersonAimFireAnimation = ThirdPersonAimFireAnimation.LoadSynchronous();
	ResolvedThirdPersonReloadAnimation = ThirdPersonReloadAnimation.LoadSynchronous();
	ResolvedMuzzleFlashSystem = MuzzleFlashSystem.LoadSynchronous();
	ResolvedFireSound = FireSound.LoadSynchronous();
	ResolvedDryFireSound = DryFireSound.LoadSynchronous();
	ResolvedCylinderOpenSound = CylinderOpenSound.LoadSynchronous();
	ResolvedCylinderCloseSound = CylinderCloseSound.LoadSynchronous();
	ResolvedRoundInsertSound = RoundInsertSound.LoadSynchronous();

	ensureMsgf(ResolvedSingleRoundReloadMontage, TEXT("Missing required R21 arms reload montage."));
	ensureMsgf(ResolvedWeaponSingleRoundReloadMontage, TEXT("Missing required R21 weapon reload montage."));
	ensureMsgf(ResolvedAimFireMontage, TEXT("Missing required R21 arms aim-fire montage."));
	ensureMsgf(ResolvedWeaponAimFireMontage, TEXT("Missing required R21 weapon aim-fire montage."));
	ensureMsgf(ResolvedHipFireMontage, TEXT("Missing required R21 arms hip-fire montage."));
	ensureMsgf(ResolvedWeaponHipFireMontage, TEXT("Missing required R21 weapon hip-fire montage."));
	ensureMsgf(ResolvedThirdPersonHipFireAnimation, TEXT("Missing retargeted third-person hip-fire animation."));
	ensureMsgf(ResolvedThirdPersonAimFireAnimation, TEXT("Missing retargeted third-person aim-fire animation."));
	ensureMsgf(ResolvedThirdPersonReloadAnimation, TEXT("Missing retargeted third-person reload animation."));
	ensureMsgf(ResolvedMuzzleFlashSystem, TEXT("Missing required muzzle flash system."));
	ensureMsgf(ResolvedFireSound, TEXT("Missing required R21 fire sound."));
	ensureMsgf(ResolvedDryFireSound, TEXT("Missing required R21 dry-fire sound."));
	ensureMsgf(ResolvedCylinderOpenSound, TEXT("Missing required R21 cylinder-open sound."));
	ensureMsgf(ResolvedCylinderCloseSound, TEXT("Missing required R21 cylinder-close sound."));
	ensureMsgf(ResolvedRoundInsertSound, TEXT("Missing required R21 round-insert sound."));
}

bool ALuxRevolver::IsRemotelyPresented() const
{
	const ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	return GetNetMode() != NM_DedicatedServer
		&& EquippedCharacter
		&& !EquippedCharacter->IsLocallyControlled();
}

void ALuxRevolver::PlayCylinderPresentation(bool bNowOpen)
{
	PlaySoundForOwner(bNowOpen ? ResolvedCylinderOpenSound : ResolvedCylinderCloseSound);
	if (bNowOpen)
	{
		PlayReloadPresentation();
		ScheduleCylinderOpenPosePause();
	}
	else
	{
		StopReloadPresentation();
	}
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
		++OwnerDryFirePresentationCount;
		PlaySoundForOwner(ResolvedDryFireSound, R21WeaponSocket);
		return;
	}
	++OwnerFirePresentationCount;

	const bool bAiming = EquippedCharacter && EquippedCharacter->IsAiming();
	if (EquippedCharacter)
	{
		EquippedCharacter->PlayFirstPersonMontage(
			bAiming ? ResolvedAimFireMontage : ResolvedHipFireMontage
		);
	}
	if (FirstPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonWeaponMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(
				bAiming ? ResolvedWeaponAimFireMontage : ResolvedWeaponHipFireMontage
			);
		}
	}

	PlaySoundForOwner(ResolvedFireSound, R21MuzzleSocket);
	if (EquippedCharacter)
	{
		USkeletalMeshComponent* FirstPersonArms = EquippedCharacter->GetFirstPersonArms();
		UNiagaraSystem* MuzzleFlash = ResolvedMuzzleFlashSystem;
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

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CylinderOpenPoseTimer);
	}

	if (ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		EquippedCharacter->PlayFirstPersonMontage(ResolvedSingleRoundReloadMontage, 1.3f);
	}
	if (FirstPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonWeaponMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(ResolvedWeaponSingleRoundReloadMontage, 1.3f);
		}
	}
}

void ALuxRevolver::PlayRoundInsertPresentation()
{
	PauseReloadPresentation();
	PlaySoundForOwner(ResolvedRoundInsertSound, R21WeaponSocket);
}

void ALuxRevolver::ScheduleCylinderOpenPosePause()
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		CylinderOpenPoseTimer,
		this,
		&ALuxRevolver::PauseReloadPresentation,
		CylinderOpenPoseDelaySeconds,
		false
	);
}

void ALuxRevolver::PauseReloadPresentation()
{
	if (ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Arms = EquippedCharacter->GetFirstPersonArms())
		{
			if (UAnimInstance* AnimInstance = Arms->GetAnimInstance())
			{
				AnimInstance->Montage_Pause(ResolvedSingleRoundReloadMontage);
			}
		}
	}
	if (FirstPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = FirstPersonWeaponMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Pause(ResolvedWeaponSingleRoundReloadMontage);
		}
	}
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

void ALuxRevolver::PlayThirdPersonCylinderPresentation(bool bNowOpen)
{
	PlaySoundForRemote(bNowOpen ? ResolvedCylinderOpenSound : ResolvedCylinderCloseSound);
}

void ALuxRevolver::PlayThirdPersonFirePresentation(bool bDryFire)
{
	if (!IsRemotelyPresented() || !ThirdPersonWeaponMesh)
	{
		return;
	}

	if (bDryFire)
	{
		PlaySoundForRemote(ResolvedDryFireSound);
		return;
	}

	const ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner());
	const bool bAiming = EquippedCharacter && EquippedCharacter->IsAiming();
	if (ALuxCharacter* MutableCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		MutableCharacter->PlayThirdPersonUpperBodyAnimation(
			bAiming ? ResolvedThirdPersonAimFireAnimation : ResolvedThirdPersonHipFireAnimation
		);
	}
	if (UAnimInstance* AnimInstance = ThirdPersonWeaponMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Play(
			bAiming ? ResolvedWeaponAimFireMontage : ResolvedWeaponHipFireMontage
		);
	}

	PlaySoundForRemote(ResolvedFireSound);
	if (UNiagaraSystem* MuzzleFlash = ResolvedMuzzleFlashSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlash,
			ThirdPersonWeaponMesh,
			NAME_None,
			R21WeaponMuzzleOffset,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
}

void ALuxRevolver::PlayThirdPersonReloadPresentation()
{
	if (!IsRemotelyPresented() || !ThirdPersonWeaponMesh)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = ThirdPersonWeaponMesh->GetAnimInstance())
	{
		AnimInstance->Montage_Play(ResolvedWeaponSingleRoundReloadMontage, 1.3f);
	}
	if (ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		EquippedCharacter->PlayThirdPersonUpperBodyAnimation(ResolvedThirdPersonReloadAnimation, 1.3f);
	}
}

void ALuxRevolver::PlayThirdPersonRoundInsertPresentation()
{
	PlaySoundForRemote(ResolvedRoundInsertSound);
}

void ALuxRevolver::PlaySoundForRemote(USoundBase* Sound) const
{
	if (IsRemotelyPresented() && Sound && ThirdPersonWeaponMesh)
	{
		UGameplayStatics::SpawnSoundAttached(Sound, ThirdPersonWeaponMesh);
	}
}

void ALuxRevolver::StopReloadPresentation()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CylinderOpenPoseTimer);
	}
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

void ALuxRevolver::StopThirdPersonReloadPresentation()
{
	if (ALuxCharacter* EquippedCharacter = Cast<ALuxCharacter>(GetOwner()))
	{
		EquippedCharacter->StopThirdPersonUpperBodyAnimation();
	}
	if (ThirdPersonWeaponMesh)
	{
		if (UAnimInstance* AnimInstance = ThirdPersonWeaponMesh->GetAnimInstance())
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
	}
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonCylinderPresentation(false);
		StopThirdPersonReloadPresentation();
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
	}
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonCylinderPresentation(false);
		StopThirdPersonReloadPresentation();
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
	else if (IsRemotelyPresented())
	{
		PlayThirdPersonCylinderPresentation(true);
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
