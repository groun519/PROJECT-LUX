#include "Player/LuxCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapons/LuxRevolver.h"

ALuxCharacter::ALuxCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
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
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> ThirdPersonMeshFinder(
		TEXT("/Game/SCK_Casual01/Models/Premade_Characters/MESH_PC_00.MESH_PC_00"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> ThirdPersonAnimClassFinder(
		TEXT("/Game/LUX/Animation/Locomotion/ABP_LuxCharacter"));

	PlayerMappingContext = MappingContextFinder.Object;
	MoveAction = MoveActionFinder.Object;
	LookAction = LookActionFinder.Object;
	FireAction = FireActionFinder.Object;
	if (ThirdPersonMeshFinder.Succeeded())
	{
		ThirdPersonMesh->SetSkeletalMeshAsset(ThirdPersonMeshFinder.Object);
	}
	if (ThirdPersonAnimClassFinder.Succeeded())
	{
		ThirdPersonMesh->SetAnimInstanceClass(ThirdPersonAnimClassFinder.Class);
	}
}

void ALuxCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		SpawnDefaultRevolver();
	}
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
}

ALuxRevolver* ALuxCharacter::GetEquippedRevolver() const
{
	return EquippedRevolver;
}

bool ALuxCharacter::IsDead() const
{
	return bIsDead;
}

bool ALuxCharacter::Die()
{
	if (!HasAuthority() || bIsDead)
	{
		return false;
	}

	bIsDead = true;
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
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALuxCharacter::AttachEquippedRevolver()
{
	if (EquippedRevolver && RevolverAttachPoint)
	{
		EquippedRevolver->AttachToComponent(
			RevolverAttachPoint,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale
		);
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
}

void ALuxCharacter::Fire(const FInputActionValue& Value)
{
	if (!bIsDead && Value.Get<bool>() && EquippedRevolver)
	{
		EquippedRevolver->RequestFire();
	}
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
