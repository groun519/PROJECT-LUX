#include "Player/LuxCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "Player/LuxFirstPersonAnimInstance.h"
#include "Player/LuxPlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/LuxRevolver.h"

ALuxCharacter::ALuxCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCapsuleComponent()->InitCapsuleSize(34.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	USkeletalMeshComponent* ThirdPersonMesh = GetMesh();
	ThirdPersonMesh->SetRelativeLocationAndRotation(FVector(0.0, 0.0, -96.0), FRotator(0.0, -90.0, 0.0));
	ThirdPersonMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThirdPersonMesh->SetGenerateOverlapEvents(false);
	ThirdPersonMesh->SetOwnerNoSee(true);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0, 0.0, 64.0));
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);

	FirstPersonArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
	FirstPersonArms->SetupAttachment(FirstPersonCamera);
	FirstPersonArms->SetRelativeLocationAndRotation(
		// Exact R21 mesh-to-camera offset measured from its reference character.
		FVector(-1.5, 0.0, -162.7),
		FRotator(0.0, -90.0, 0.0)
	);
	FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonArms->SetGenerateOverlapEvents(false);
	FirstPersonArms->SetOnlyOwnerSee(true);
	FirstPersonArms->SetCastShadow(false);
	FirstPersonArms->bCastDynamicShadow = false;

	RevolverAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RevolverAttachPoint"));
	RevolverAttachPoint->SetupAttachment(GetCapsuleComponent());

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextFinder(
		TEXT("/Game/LUX/Input/IMC_Player.IMC_Player"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionFinder(
		TEXT("/Game/LUX/Input/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionFinder(
		TEXT("/Game/LUX/Input/IA_Look.IA_Look"));
	static ConstructorHelpers::FObjectFinder<UInputAction> FireActionFinder(
		TEXT("/Game/LUX/Input/IA_Fire.IA_Fire"));
	static ConstructorHelpers::FObjectFinder<UInputAction> ReloadActionFinder(
		TEXT("/Game/LUX/Input/IA_Reload.IA_Reload"));
	static ConstructorHelpers::FObjectFinder<UInputAction> AimActionFinder(
		TEXT("/Game/LUX/Input/IA_Aim.IA_Aim"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ThirdPersonMeshFinder(
		TEXT("/Game/SCK_Casual01/Models/Premade_Characters/MESH_PC_00.MESH_PC_00"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> ThirdPersonAnimClassFinder(
		TEXT("/Game/LUX/Animation/Locomotion/ABP_LuxCharacter"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FirstPersonArmsFinder(
		TEXT("/Game/RevolverFPGM/Demo/FirstPersonArms/Character/Mesh/SKM_Mannequin_Arms.SKM_Mannequin_Arms"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> FirstPersonAnimClassFinder(
		TEXT("/Game/LUX/Animation/Revolver/ABP_LuxFirstPerson"));

	PlayerMappingContext = MappingContextFinder.Object;
	MoveAction = MoveActionFinder.Object;
	LookAction = LookActionFinder.Object;
	FireAction = FireActionFinder.Object;
	ReloadAction = ReloadActionFinder.Object;
	AimAction = AimActionFinder.Object;
	if (ThirdPersonMeshFinder.Succeeded())
	{
		ThirdPersonMesh->SetSkeletalMeshAsset(ThirdPersonMeshFinder.Object);
	}
	if (ThirdPersonAnimClassFinder.Succeeded())
	{
		ThirdPersonMesh->SetAnimInstanceClass(ThirdPersonAnimClassFinder.Class);
	}
	if (FirstPersonArmsFinder.Succeeded())
	{
		FirstPersonArms->SetSkeletalMeshAsset(FirstPersonArmsFinder.Object);
	}
	if (FirstPersonAnimClassFinder.Succeeded())
	{
		FirstPersonArms->SetAnimInstanceClass(FirstPersonAnimClassFinder.Class);
	}
}

void ALuxCharacter::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(IsLocallyControlled());
	UpdateFirstPersonAimAnimation();

	if (HasAuthority())
	{
		SpawnDefaultRevolver();
	}
}

void ALuxCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFirstPersonPresentation(DeltaSeconds);
}

void ALuxCharacter::Destroyed()
{
	if (HasAuthority() && IsValid(EquippedRevolver))
	{
		EquippedRevolver->Destroy();
		EquippedRevolver = nullptr;
	}

	Super::Destroyed();
}

void ALuxCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALuxCharacter, EquippedRevolver);
	DOREPLIFETIME(ALuxCharacter, bIsDead);
	DOREPLIFETIME_CONDITION(ALuxCharacter, bIsAiming, COND_SkipOwner);
}

ALuxRevolver* ALuxCharacter::GetEquippedRevolver() const
{
	return EquippedRevolver;
}

bool ALuxCharacter::IsDead() const
{
	return bIsDead;
}

bool ALuxCharacter::IsAiming() const
{
	return IsLocallyControlled() ? bLocalAimIntent : bIsAiming;
}

bool ALuxCharacter::GetLocalAimIntentForDevelopment() const
{
	return bLocalAimIntent;
}

bool ALuxCharacter::SetAimingForDevelopment(bool bNewAiming)
{
	if (!HasAuthority())
	{
		return false;
	}

	bIsAiming = bNewAiming && !bIsDead;
	ForceNetUpdate();
	return true;
}

USkeletalMeshComponent* ALuxCharacter::GetFirstPersonArms() const
{
	return FirstPersonArms;
}

void ALuxCharacter::PlayFirstPersonMontage(
	UAnimMontage* Montage,
	float PlayRate,
	float StartPositionSeconds
)
{
	if (!IsLocallyControlled() || !Montage || !FirstPersonArms)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = FirstPersonArms->GetAnimInstance())
	{
		AnimInstance->Montage_Play(
			Montage,
			PlayRate,
			EMontagePlayReturnType::MontageLength,
			StartPositionSeconds
		);
	}
}

void ALuxCharacter::PlayThirdPersonUpperBodyAnimation(
	UAnimSequenceBase* Animation,
	float PlayRate,
	float StartPositionSeconds
)
{
	if (IsLocallyControlled() || !Animation)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->PlaySlotAnimationAsDynamicMontage(
			Animation,
			TEXT("DefaultSlot"),
			0.1f,
			0.15f,
			PlayRate,
			1,
			-1.0f,
			StartPositionSeconds
		);
	}
}

void ALuxCharacter::StopFirstPersonMontages(float BlendOutSeconds)
{
	if (FirstPersonArms)
	{
		if (UAnimInstance* AnimInstance = FirstPersonArms->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(BlendOutSeconds);
		}
	}
}

void ALuxCharacter::StopThirdPersonUpperBodyAnimation(float BlendOutSeconds)
{
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->StopAllMontages(BlendOutSeconds);
	}
}

bool ALuxCharacter::Die()
{
	if (!HasAuthority() || bIsDead)
	{
		return false;
	}

	bIsDead = true;
	bIsAiming = false;
	bLocalAimIntent = false;
	ApplyDeathState();
	ForceNetUpdate();
	return true;
}

void ALuxCharacter::OnRep_EquippedRevolver()
{
	AttachEquippedRevolver();
}

void ALuxCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		ApplyDeathState();
	}
}

void ALuxCharacter::ApplyDeathState()
{
	bIsAiming = false;
	bLocalAimIntent = false;
	UpdateFirstPersonAimAnimation();
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(DefaultFieldOfView);
	}
	SetActorTickEnabled(false);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (EquippedRevolver)
	{
		EquippedRevolver->StopOwnerPresentation();
	}
	else
	{
		StopFirstPersonMontages();
	}
	if (FirstPersonArms)
	{
		FirstPersonArms->SetVisibility(false, true);
	}
}

void ALuxCharacter::AttachEquippedRevolver()
{
	if (EquippedRevolver && RevolverAttachPoint)
	{
		EquippedRevolver->AttachToComponent(
			RevolverAttachPoint,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
		EquippedRevolver->AttachPresentationVisualsTo(FirstPersonArms, GetMesh());
	}
}

void ALuxCharacter::SpawnDefaultRevolver()
{
	if (EquippedRevolver || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedRevolver = GetWorld()->SpawnActor<ALuxRevolver>(
		ALuxRevolver::StaticClass(),
		GetActorTransform(),
		SpawnParameters
	);
	AttachEquippedRevolver();
	ForceNetUpdate();
}

void ALuxCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	SetActorTickEnabled(true);
	UpdateFirstPersonAimAnimation();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

	if (InputSubsystem && PlayerMappingContext)
	{
		// Removing first keeps repeated possession/restart paths from stacking this context.
		InputSubsystem->RemoveMappingContext(PlayerMappingContext);
		InputSubsystem->AddMappingContext(PlayerMappingContext, 0);
	}
}

void ALuxCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALuxCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALuxCharacter::Look);
	}
	if (FireAction)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ALuxCharacter::Fire);
	}
	if (ReloadAction)
	{
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ALuxCharacter::Reload);
	}
	if (AimAction)
	{
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ALuxCharacter::AimStarted);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ALuxCharacter::AimStopped);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Canceled, this, &ALuxCharacter::AimStopped);
	}

	// Until the inventory phase supplies an actual carried round, number keys are a
	// non-Shipping Live-round driver for validating the final manual chamber flow.
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ALuxCharacter::ReloadPosition1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ALuxCharacter::ReloadPosition2);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ALuxCharacter::ReloadPosition3);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ALuxCharacter::ReloadPosition4);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ALuxCharacter::ReloadPosition5);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ALuxCharacter::ReloadPosition6);
}

void ALuxCharacter::AimStarted(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		SetAiming(true);
	}
}

void ALuxCharacter::AimStopped(const FInputActionValue& Value)
{
	(void)Value;
	SetAiming(false);
}

void ALuxCharacter::SetAiming(bool bNewAiming)
{
	bNewAiming = bNewAiming && !bIsDead;
	if (bLocalAimIntent == bNewAiming)
	{
		return;
	}

	bLocalAimIntent = bNewAiming;
	UpdateFirstPersonAimAnimation();
	if (HasAuthority())
	{
		bIsAiming = bNewAiming;
		ForceNetUpdate();
	}
	else
	{
		ServerSetAiming(bNewAiming);
	}
}

void ALuxCharacter::ServerSetAiming_Implementation(bool bNewAiming)
{
	bIsAiming = bNewAiming && !bIsDead;
	ForceNetUpdate();
}

void ALuxCharacter::UpdateFirstPersonAimAnimation()
{
	ULuxFirstPersonAnimInstance* AnimInstance =
		FirstPersonArms ? Cast<ULuxFirstPersonAnimInstance>(FirstPersonArms->GetAnimInstance()) : nullptr;
	if (AnimInstance)
	{
		AnimInstance->SetAiming(IsAiming() && !bIsDead);
	}
}

void ALuxCharacter::UpdateFirstPersonPresentation(float DeltaSeconds)
{
	if (!IsLocallyControlled() || !FirstPersonCamera)
	{
		return;
	}

	const float TargetFieldOfView = IsAiming() && !bIsDead ? AimFieldOfView : DefaultFieldOfView;
	FirstPersonCamera->SetFieldOfView(FMath::FInterpTo(
		FirstPersonCamera->FieldOfView,
		TargetFieldOfView,
		DeltaSeconds,
		AimInterpolationSpeed
	));
}

void ALuxCharacter::Fire(const FInputActionValue& Value)
{
	if (!bIsDead && Value.Get<bool>() && EquippedRevolver)
	{
		EquippedRevolver->RequestFire();
	}
}

void ALuxCharacter::Reload(const FInputActionValue& Value)
{
	if (bIsDead || !Value.Get<bool>() || !EquippedRevolver)
	{
		return;
	}

	if (!EquippedRevolver->IsCylinderOpen())
	{
		EquippedRevolver->RequestOpenCylinder();
	}
	else if (EquippedRevolver->IsRoundInsertionPending())
	{
		EquippedRevolver->RequestCancelReload();
	}
	else
	{
		EquippedRevolver->RequestCloseCylinder();
	}
}

void ALuxCharacter::ReloadPosition1()
{
	ReloadAtPosition(1);
}

void ALuxCharacter::ReloadPosition2()
{
	ReloadAtPosition(2);
}

void ALuxCharacter::ReloadPosition3()
{
	ReloadAtPosition(3);
}

void ALuxCharacter::ReloadPosition4()
{
	ReloadAtPosition(4);
}

void ALuxCharacter::ReloadPosition5()
{
	ReloadAtPosition(5);
}

void ALuxCharacter::ReloadPosition6()
{
	ReloadAtPosition(6);
}

void ALuxCharacter::ReloadAtPosition(int32 ReloadPosition)
{
#if UE_BUILD_SHIPPING
	(void)ReloadPosition;
#else
	if (
		bIsDead
		|| !EquippedRevolver
		|| !EquippedRevolver->IsCylinderOpen()
		|| EquippedRevolver->IsRoundInsertionPending()
		|| EquippedRevolver->IsReloadPositionLoaded(ReloadPosition)
	)
	{
		return;
	}

	if (ALuxPlayerController* PlayerController = Cast<ALuxPlayerController>(GetController()))
	{
		PlayerController->LuxLoadRound(TEXT("Live"), ReloadPosition);
	}
#endif
}

void ALuxCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D Movement = Value.Get<FVector2D>();
	const FRotator YawRotation(0.0, Controller->GetControlRotation().Yaw, 0.0);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Movement.Y);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Movement.X);
}

void ALuxCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}
