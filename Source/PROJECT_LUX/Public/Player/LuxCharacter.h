#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LuxCharacter.generated.h"

class UCameraComponent;
class UAnimMontage;
class UAnimSequenceBase;
class UInputAction;
class UInputMappingContext;
class USceneComponent;
class USkeletalMeshComponent;
class ALuxRevolver;
struct FInputActionValue;

UCLASS()
class PROJECT_LUX_API ALuxCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALuxCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	ALuxRevolver* GetEquippedRevolver() const;

	UFUNCTION(BlueprintPure, Category = "State")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	bool IsAiming() const;

	UFUNCTION(BlueprintPure, Category = "Revolver|Development", meta = (DevelopmentOnly))
	bool GetLocalAimIntentForDevelopment() const;

	USkeletalMeshComponent* GetFirstPersonArms() const;
	void PlayFirstPersonMontage(
		UAnimMontage* Montage,
		float PlayRate = 1.0f,
		float StartPositionSeconds = 0.0f
	);
	void PlayThirdPersonUpperBodyAnimation(
		UAnimSequenceBase* Animation,
		float PlayRate = 1.0f,
		float StartPositionSeconds = 0.0f
	);
	void StopFirstPersonMontages(float BlendOutSeconds = 0.15f);
	void StopThirdPersonUpperBodyAnimation(float BlendOutSeconds = 0.15f);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "State")
	bool Die();

private:
	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bNewAiming);

	UFUNCTION()
	void OnRep_EquippedRevolver();

	UFUNCTION()
	void OnRep_IsDead();

	void ApplyDeathState();
	void AttachEquippedRevolver();
	void AimStarted(const FInputActionValue& Value);
	void AimStopped(const FInputActionValue& Value);
	void SetAiming(bool bNewAiming);
	void UpdateFirstPersonAimAnimation();
	void UpdateFirstPersonPresentation(float DeltaSeconds);
	void Fire(const FInputActionValue& Value);
	void Reload(const FInputActionValue& Value);
	void SpawnDefaultRevolver();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "View", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "View", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonArms;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RevolverAttachPoint;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedRevolver, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ALuxRevolver> EquippedRevolver;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleInstanceOnly, BlueprintReadOnly, Category = "State", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	// Server-authoritative state for remote presentation. The owner uses bLocalAimIntent immediately.
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	bool bIsAiming = false;

	bool bLocalAimIntent = false;

	UPROPERTY(EditDefaultsOnly, Category = "View", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float DefaultFieldOfView = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category = "View", meta = (ClampMin = "30.0", ClampMax = "120.0"))
	float AimFieldOfView = 65.0f;

	UPROPERTY(EditDefaultsOnly, Category = "View", meta = (ClampMin = "0.1"))
	float AimInterpolationSpeed = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> AimAction;
};
