#include "Player/GobulinPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/BattleAttributeComponent.h"
#include "Combat/BattleAttributeSet.h"
#include "Combat/FPSWeaponOverlayComponent.h"
#include "Combat/PlayerCombatComponent.h"
#include "Core/BattleTags.h"
#include "EnhancedInputComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"

AGobulinPlayerCharacter::AGobulinPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 96.0f);

	// 第一人称角色由控制器朝向驱动，不能同时开启“朝移动方向旋转”。
	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = 500.0f;
	GetCharacterMovement()->JumpZVelocity = 420.0f;
	GetCharacterMovement()->AirControl = 0.5f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// 世界中的角色表现交给后续的 2D 纸片/远端表现配置，本地不显示身体网格。
	GetMesh()->SetOwnerNoSee(true);

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 90.0f;
	FirstPersonCameraComponent->FirstPersonScale = 1.0f;

	FirstPersonWeaponOverlay = CreateDefaultSubobject<UFPSWeaponOverlayComponent>(TEXT("FirstPersonWeaponOverlay"));
	FirstPersonWeaponOverlay->SetupAttachment(FirstPersonCameraComponent);
	FirstPersonWeaponOverlay->SetOnlyOwnerSee(true);
	FirstPersonWeaponOverlay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonWeaponOverlay->SetCastShadow(false);

	Attributes = CreateDefaultSubobject<UBattleAttributeComponent>(TEXT("BattleAttributes"));
	Combat = CreateDefaultSubobject<UPlayerCombatComponent>(TEXT("PlayerCombat"));
}

void AGobulinPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (Attributes)
	{
		Attributes->SetBaseAttribute(BattleTag_Health_Max, 100.0f, 1.0f, 99999.0f);
		Attributes->SetBaseAttribute(BattleTag_Health_Current, 100.0f, 0.0f, 99999.0f);
		Attributes->SetBaseAttribute(BattleTag_Movement_Speed, 500.0f, 100.0f, 2000.0f);

		if (UBattleAttributeSet* AttributeSet = Attributes->GetAttributeSet())
		{
			AttributeSet->OnAttributeChanged.AddDynamic(this, &AGobulinPlayerCharacter::OnAttributeChanged);
		}
	}

	ApplyMovementSettings();

	if (Combat)
	{
		Combat->SetWeaponOverlay(FirstPersonWeaponOverlay);
		Combat->SetComponentTickEnabled(IsLocallyControlled() || HasAuthority());
	}

	if (FirstPersonWeaponOverlay && !IsLocallyControlled())
	{
		FirstPersonWeaponOverlay->SetVisibility(false, true);
		FirstPersonWeaponOverlay->SetComponentTickEnabled(false);
	}
}

void AGobulinPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AGobulinPlayerCharacter::MoveInput);
	}
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AGobulinPlayerCharacter::LookInput);
	}
	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::JumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AGobulinPlayerCharacter::JumpCompleted);
	}
	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::SprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AGobulinPlayerCharacter::SprintCompleted);
	}
	if (FireAction)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::FireStarted);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AGobulinPlayerCharacter::FireCompleted);
	}
	if (AimAction)
	{
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::AimStarted);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AGobulinPlayerCharacter::AimCompleted);
	}
	if (ReloadAction)
	{
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::ReloadStarted);
	}
	if (MeleeAction)
	{
		EnhancedInputComponent->BindAction(MeleeAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::MeleeStarted);
		EnhancedInputComponent->BindAction(MeleeAction, ETriggerEvent::Completed, this, &AGobulinPlayerCharacter::MeleeCompleted);
	}
	if (SwitchWeaponAction)
	{
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::SwitchWeaponStarted);
	}
}

void AGobulinPlayerCharacter::MoveInput(const FInputActionValue& Value)
{
	if (bDead || !Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, MovementVector.Y);
	AddMovementInput(Right, MovementVector.X);
}

void AGobulinPlayerCharacter::LookInput(const FInputActionValue& Value)
{
	if (bDead)
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>();
	AddControllerYawInput(LookAxis.X);
	AddControllerPitchInput(LookAxis.Y);
}

void AGobulinPlayerCharacter::JumpStarted()
{
	if (!bDead)
	{
		Jump();
	}
}

void AGobulinPlayerCharacter::JumpCompleted()
{
	StopJumping();
}

void AGobulinPlayerCharacter::SprintStarted()
{
	SetSprinting(true);
}

void AGobulinPlayerCharacter::SprintCompleted()
{
	SetSprinting(false);
}

void AGobulinPlayerCharacter::FireStarted()
{
	if (!bDead && Combat)
	{
		Combat->StartFire();
	}
}

void AGobulinPlayerCharacter::FireCompleted()
{
	if (Combat)
	{
		Combat->StopFire();
	}
}

void AGobulinPlayerCharacter::AimStarted()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AGobulinPlayerCharacter::AimCompleted()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AGobulinPlayerCharacter::ReloadStarted()
{
	if (!bDead && Combat)
	{
		Combat->Reload();
	}
}

void AGobulinPlayerCharacter::MeleeStarted()
{
	if (!bDead && Combat)
	{
		Combat->StartMelee();
	}
}

void AGobulinPlayerCharacter::MeleeCompleted()
{
	if (Combat)
	{
		Combat->StopFire();
	}
}

void AGobulinPlayerCharacter::SwitchWeaponStarted()
{
	if (!bDead && Combat)
	{
		Combat->SwitchWeapon(1);
	}
}

void AGobulinPlayerCharacter::SetSprinting(bool bInSprinting)
{
	if (bDead)
	{
		bSprinting = false;
	}
	else
	{
		bSprinting = bInSprinting;
	}

	ApplyMovementSettings();
}

void AGobulinPlayerCharacter::ApplyMovementSettings()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	float MovementSpeed = 500.0f;
	if (Attributes)
	{
		MovementSpeed = Attributes->GetAttributeValue(BattleTag_Movement_Speed);
	}

	GetCharacterMovement()->MaxWalkSpeed = bSprinting
		? MovementSpeed * SprintSpeedMultiplier
		: MovementSpeed;
}

void AGobulinPlayerCharacter::OnAttributeChanged(FGameplayTag AttributeTag, float NewValue)
{
	if (AttributeTag.MatchesTagExact(BattleTag_Movement_Speed))
	{
		ApplyMovementSettings();
	}
}

void AGobulinPlayerCharacter::TakeDamage_Implementation(const FDamageInfo& DamageInfo)
{
	if (bDead || !Attributes || DamageInfo.Amount <= 0.0f)
	{
		return;
	}

	const float CurrentHealth = Attributes->GetAttributeValue(BattleTag_Health_Current);
	const float NewHealth = FMath::Max(0.0f, CurrentHealth - DamageInfo.Amount);
	Attributes->SetBaseAttribute(BattleTag_Health_Current, NewHealth, 0.0f, 99999.0f);

	if (NewHealth <= 0.0f)
	{
		Die();
	}
}

float AGobulinPlayerCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	FDamageInfo DamageInfo;
	DamageInfo.Amount = DamageAmount;
	DamageInfo.Instigator = DamageCauser ? DamageCauser : (EventInstigator ? EventInstigator->GetPawn() : nullptr);
	DamageInfo.DamageSourceId = TEXT("ClassicDamage");
	IDamageable::Execute_TakeDamage(this, DamageInfo);
	return DamageAmount;
}

void AGobulinPlayerCharacter::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	bSprinting = false;
	ApplyMovementSettings();

	if (Combat)
	{
		Combat->StopFire();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
