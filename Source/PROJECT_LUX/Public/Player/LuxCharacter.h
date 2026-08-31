#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LuxCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USceneComponent;
class ALuxRevolver;
struct FInputActionValue;

UCLASS()
class PROJECT_LUX_API ALuxCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALuxCharacter();

	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintPure, Category = "Revolver")
	ALuxRevolver* GetEquippedRevolver() const;

private:
	UFUNCTION()
	void OnRep_EquippedRevolver();

	void AttachEquippedRevolver();
	void SpawnDefaultRevolver();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "View", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RevolverAttachPoint;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedRevolver, VisibleInstanceOnly, BlueprintReadOnly, Category = "Revolver", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ALuxRevolver> EquippedRevolver;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> PlayerMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
};
