#pragma once

#include "Containers/StaticArray.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LuxRevolver.generated.h"

class ALuxCharacter;
class ALuxRevolver;
class UAnimMontage;
class UAnimSequenceBase;
class UNiagaraSystem;
class USkeletalMeshComponent;
class USoundBase;

UENUM(BlueprintType)
enum class ELuxRevolverRoundType : uint8
{
	Empty,
	Live,
	Blank,
	Rubber
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FLuxNonLethalHitSignature,
	ALuxRevolver*, Revolver,
	ALuxCharacter*, Target
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLuxDryFireSignature, ALuxRevolver*, Revolver);

UCLASS()
class PROJECT_LUX_API ALuxRevolver : public AActor
{
	GENERATED_BODY()

public:
	static constexpr uint8 ChamberCount = 6;

	ALuxRevolver();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostNetInit() override;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetChamberCount() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	uint8 GetLoadedMask() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	uint8 GetCurrentChamberIndex() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsCylinderOpen() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsChamberLoaded(int32 ChamberIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Revolver")
	void RequestFire();

	void AttachPresentationVisualsTo(
		USkeletalMeshComponent* FirstPersonArms,
		USkeletalMeshComponent* ThirdPersonBody
	);
	void StopOwnerPresentation();

	UFUNCTION(BlueprintCallable, Category = "Revolver|Reload")
	void RequestOpenCylinder();

	UFUNCTION(BlueprintCallable, Category = "Revolver|Reload")
	void RequestCloseCylinder();

	UFUNCTION(BlueprintCallable, Category = "Revolver|Reload")
	void RequestCancelReload();

	// Server-side production entry point. Future ammo inventory code must use this path.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Revolver|Reload")
	bool BeginRoundInsertion(ELuxRevolverRoundType RoundType);

	UFUNCTION(BlueprintPure, Category = "Revolver|Reload")
	bool IsRoundInsertionPending() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Reload")
	int32 GetReloadSequence() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Reload")
	int32 GetRoundInsertSequence() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Reload")
	UAnimMontage* GetSingleRoundReloadMontage() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetFireSequence() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	int32 GetDryFireSequence() const;

	ELuxRevolverRoundType GetRoundTypeForAuthority(int32 ChamberIndex) const;

	UPROPERTY(BlueprintAssignable, Category = "Revolver")
	FLuxNonLethalHitSignature OnNonLethalHit;

	UPROPERTY(BlueprintAssignable, Category = "Revolver")
	FLuxDryFireSignature OnDryFire;

	UFUNCTION(BlueprintCallable, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool SetChamberRoundTypeForDevelopment(int32 ChamberIndex, ELuxRevolverRoundType RoundType);

	UFUNCTION(BlueprintCallable, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool SetPublicStateForDevelopment(int32 ChamberIndex, bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	FString DescribeChambersForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	FString DescribeLastFireResultForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	ALuxCharacter* GetLastNonLethalHitForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	FString DescribeLastReloadResultForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	int32 GetOwnerFirePresentationCountForDevelopment() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	int32 GetOwnerDryFirePresentationCountForDevelopment() const;

private:
	enum class ELocalFirePrediction : int8
	{
		None = -1,
		Loaded = 0,
		Dry = 1
	};

	UFUNCTION()
	void OnRep_LoadedMask();

	UFUNCTION()
	void OnRep_CylinderOpen(bool bWasCylinderOpen);

	UFUNCTION()
	void OnRep_FireSequence();

	UFUNCTION()
	void OnRep_DryFireSequence();

	UFUNCTION()
	void OnRep_ReloadSequence();

	UFUNCTION()
	void OnRep_RoundInsertSequence();

	UFUNCTION(Server, Reliable)
	void ServerFire(uint16 RequestId);

	UFUNCTION(Client, Reliable)
	void ClientConfirmFire(uint16 RequestId, bool bAccepted, bool bDryFire);

	UFUNCTION(Server, Reliable)
	void ServerOpenCylinder();

	UFUNCTION(Server, Reliable)
	void ServerCloseCylinder();

	UFUNCTION(Server, Reliable)
	void ServerCancelReload();

	bool CanFire(const ALuxCharacter* EquippedCharacter, double ServerTimeSeconds) const;
	bool CanManipulateCylinder(const ALuxCharacter* EquippedCharacter) const;
	void ConfirmLocalFire(uint16 RequestId, bool bAccepted, bool bDryFire);
	void ClearPendingRoundInsertion();
	void CommitPendingRoundInsertion();
	int32 FindNextEmptyChamber(int32 StartIndex) const;
	bool IsLocallyPresented() const;
	bool IsRemotelyPresented() const;
	bool IsValidChamberIndex(int32 ChamberIndex) const;
	void HoldCylinderOpenPresentation();
	void PlayCylinderPresentation(bool bNowOpen);
	void PlayFirePresentation(bool bDryFire);
	void PlayReloadPresentation(float StartPositionSeconds = 0.0f);
	void PlayRoundInsertPresentation();
	void PlaySoundForOwner(USoundBase* Sound, FName SocketName = NAME_None) const;
	ELocalFirePrediction PredictLocalFire(double LocalTimeSeconds);
	void ResolvePresentationAssets();
	bool ResolveServerFire(bool& bOutDryFire);
	void ScheduleCylinderOpenPoseHold(float DelaySeconds);
	void SpawnMuzzleFlashFor(USkeletalMeshComponent* WeaponMesh, float Scale) const;
	void PlayThirdPersonCylinderPresentation(bool bNowOpen);
	void PlayThirdPersonFirePresentation(bool bDryFire);
	void PlayThirdPersonReloadPresentation(float StartPositionSeconds = 0.0f);
	void PlayThirdPersonRoundInsertPresentation();
	void PlaySoundForRemote(USoundBase* Sound) const;
	void StopReloadPresentation();
	void StopThirdPersonReloadPresentation();
	ALuxCharacter* TraceCharacter(const ALuxCharacter* EquippedCharacter) const;
	bool TryCancelReload();
	bool TryCloseCylinder();
	bool TryOpenCylinder();
	void ConsumeCurrentRoundAndAdvance();
	void RefreshLoadedMask();
	void RefreshBulletVisuals();

	// Exact round types intentionally remain server-only and are never registered for replication.
	TStaticArray<ELuxRevolverRoundType, ChamberCount> ChamberRoundTypes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolver|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonWeaponMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolver|Presentation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> ThirdPersonWeaponMesh;

	UPROPERTY(ReplicatedUsing = OnRep_LoadedMask, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	uint8 LoadedMask = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	uint8 CurrentChamberIndex = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CylinderOpen, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	bool bCylinderOpen = false;

	// Generic sequences support later presentation without revealing Live, Blank, or Rubber.
	UPROPERTY(ReplicatedUsing = OnRep_FireSequence, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	int32 FireSequence = 0;

	UPROPERTY(ReplicatedUsing = OnRep_DryFireSequence, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	int32 DryFireSequence = 0;

	// Reload sequences are generic physical events and never identify the inserted round type.
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver|Reload", meta = (AllowPrivateAccess = "true"))
	bool bRoundInsertionPending = false;

	UPROPERTY(ReplicatedUsing = OnRep_ReloadSequence, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver|Reload", meta = (AllowPrivateAccess = "true"))
	int32 ReloadSequence = 0;

	UPROPERTY(ReplicatedUsing = OnRep_RoundInsertSequence, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver|Reload", meta = (AllowPrivateAccess = "true"))
	int32 RoundInsertSequence = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Reload")
	TSoftObjectPtr<UAnimMontage> SingleRoundReloadMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimMontage> WeaponSingleRoundReloadMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimMontage> AimFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimMontage> WeaponAimFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimMontage> HipFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimMontage> WeaponHipFireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimSequenceBase> ThirdPersonHipFireAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimSequenceBase> ThirdPersonAimFireAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UAnimSequenceBase> ThirdPersonReloadAnimation;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<UNiagaraSystem> MuzzleFlashSystem;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<USoundBase> CylinderOpenSound;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation")
	TSoftObjectPtr<USoundBase> CylinderCloseSound;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ResolvedSingleRoundReloadMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ResolvedWeaponSingleRoundReloadMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ResolvedAimFireMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ResolvedWeaponAimFireMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ResolvedHipFireMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ResolvedWeaponHipFireMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ResolvedThirdPersonHipFireAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ResolvedThirdPersonAimFireAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ResolvedThirdPersonReloadAnimation;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ResolvedMuzzleFlashSystem;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ResolvedDryFireSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ResolvedCylinderOpenSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ResolvedCylinderCloseSound;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Reload", meta = (ClampMin = "0.1"))
	float ReloadPresentationPlayRate = 1.0f;

	// R21's first visible single-round insertion starts here and lands at 2.88 seconds.
	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Reload", meta = (ClampMin = "0.0"))
	float RoundInsertMontageStartSeconds = 2.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Reload", meta = (ClampMin = "0.05"))
	float RoundInsertMontageCommitSeconds = 2.88f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Reload", meta = (ClampMin = "0.0"))
	float RoundInsertSettleDelaySeconds = 0.18f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Fire", meta = (ClampMin = "0.05"))
	float MinimumFireIntervalSeconds = 0.25f;

	// Keep the cylinder open without leaving the hands frozen mid-insertion.
	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation", meta = (ClampMin = "0.0"))
	float CylinderOpenPosePositionSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation", meta = (ClampMin = "0.01"))
	float MuzzleFlashDurationSeconds = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation", meta = (ClampMin = "0.01"))
	float FirstPersonMuzzleFlashScale = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Presentation", meta = (ClampMin = "0.01"))
	float ThirdPersonMuzzleFlashScale = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Revolver|Fire", meta = (ClampMin = "100.0"))
	float TraceRange = 10000.0f;

	double LastFireServerTimeSeconds = -1.0;
	double LastLocalFirePresentationTimeSeconds = -1.0;
	FName LastFireResultForDevelopment = TEXT("None");
	TWeakObjectPtr<ALuxCharacter> LastNonLethalHitForDevelopment;
	TMap<uint16, ELocalFirePrediction> LocalFirePredictions;
	uint16 NextLocalFireRequestId = 0;
	int32 OwnerFirePresentationCount = 0;
	int32 OwnerDryFirePresentationCount = 0;

	uint8 ReloadChamberIndex = 0;
	uint8 PendingRoundChamberIndex = ChamberCount;
	ELuxRevolverRoundType PendingRoundType = ELuxRevolverRoundType::Empty;
	FTimerHandle RoundInsertionTimer;
	FTimerHandle CylinderOpenPoseTimer;
	FName LastReloadResultForDevelopment = TEXT("None");
	bool bPresentationReplicationReady = false;
};
