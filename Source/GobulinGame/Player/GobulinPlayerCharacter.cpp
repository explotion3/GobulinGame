#include "Player/GobulinPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Combat/BattleAttributeComponent.h"
#include "Combat/BattleAttributeSet.h"
#include "Combat/CombatantRegistrySubsystem.h"
#include "Combat/GobulinSwordCombatComponent.h"
#include "Combat/GobulinSwordFeedbackComponent.h"
#include "Combat/GobulinWeaponViewComponent.h"
#include "Core/BattleTags.h"
#include "Core/CombatTags.h"
#include "EnhancedInputComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Player/GobulinCameraFeedbackComponent.h"
#include "UObject/ConstructorHelpers.h"

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

	CameraFeedback = CreateDefaultSubobject<UGobulinCameraFeedbackComponent>(TEXT("CameraFeedback"));
	CameraFeedback->SetCamera(FirstPersonCameraComponent);

	FirstPersonWeaponView = CreateDefaultSubobject<UGobulinWeaponViewComponent>(TEXT("FirstPersonWeaponView"));
	FirstPersonWeaponView->SetupAttachment(FirstPersonCameraComponent);

	FirstPersonWeaponVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonWeaponVisual"));
	FirstPersonWeaponVisual->SetupAttachment(FirstPersonWeaponView);
	FirstPersonWeaponVisual->SetOnlyOwnerSee(true);
	FirstPersonWeaponVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonWeaponVisual->SetCastShadow(false);
	FirstPersonWeaponVisual->SetReceivesDecals(false);
	FirstPersonWeaponVisual->SetMobility(EComponentMobility::Movable);

	FirstPersonSwordTip = CreateDefaultSubobject<UArrowComponent>(TEXT("FirstPersonSwordTip"));
	FirstPersonSwordTip->SetupAttachment(FirstPersonWeaponVisual);
	FirstPersonSwordTip->SetArrowLength(24.0f);
	FirstPersonSwordTip->SetHiddenInGame(true);
	FirstPersonSwordTip->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FirstPersonSwordTip->SetOnlyOwnerSee(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh.Succeeded())
	{
		FirstPersonWeaponVisual->SetStaticMesh(PlaneMesh.Object);
	}

	FirstPersonWeaponView->SetVisualMesh(FirstPersonWeaponVisual);

	Attributes = CreateDefaultSubobject<UBattleAttributeComponent>(TEXT("BattleAttributes"));
	SwordCombat = CreateDefaultSubobject<UGobulinSwordCombatComponent>(TEXT("SwordCombat"));
	SwordFeedback = CreateDefaultSubobject<UGobulinSwordFeedbackComponent>(TEXT("SwordFeedback"));
	SwordFeedback->SetSwordCombat(SwordCombat);
}

void AGobulinPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>())
	{
		CombatantHandle = Registry->RegisterActor(this, CombatTeamId);
	}

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

	if (SwordCombat)
	{
		SwordCombat->SetWeaponView(FirstPersonWeaponView);
		SwordCombat->SetSwordTip(FirstPersonSwordTip);
		SwordCombat->OnAttackStateChanged.AddDynamic(this, &AGobulinPlayerCharacter::OnSwordAttackStateChanged);
	}

	if (SwordFeedback)
	{
		SwordFeedback->SetSwordCombat(SwordCombat);
	}

	if (!IsLocallyControlled())
	{
		if (FirstPersonWeaponView)
		{
			FirstPersonWeaponView->SetVisibility(false, false);
		}
		if (FirstPersonWeaponVisual)
		{
			FirstPersonWeaponVisual->SetVisibility(false, true);
		}
	}
}

void AGobulinPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UCombatantRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>() : nullptr)
	{
		Registry->UnregisterHandle(CombatantHandle);
	}
	CombatantHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void AGobulinPlayerCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FirstPersonWeaponView)
	{
		FirstPersonWeaponView->SetVisualMesh(FirstPersonWeaponVisual);
	}

	if (CameraFeedback)
	{
		CameraFeedback->SetCamera(FirstPersonCameraComponent);
	}

	if (SwordCombat && FirstPersonWeaponView)
	{
		FirstPersonWeaponView->SetSwordDefinition(SwordCombat->GetSwordDefinition());
		SwordCombat->SetSwordTip(FirstPersonSwordTip);
	}

	if (SwordFeedback)
	{
		SwordFeedback->SetSwordCombat(SwordCombat);
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
	if (SwordAttackAction)
	{
		EnhancedInputComponent->BindAction(SwordAttackAction, ETriggerEvent::Started, this, &AGobulinPlayerCharacter::SwordAttackStarted);
		EnhancedInputComponent->BindAction(SwordAttackAction, ETriggerEvent::Completed, this, &AGobulinPlayerCharacter::SwordAttackCompleted);
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
	if (!bDead && CanJumpDuringSwordState())
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

void AGobulinPlayerCharacter::SwordAttackStarted()
{
	if (bDead || !SwordCombat)
	{
		return;
	}

	SwordCombat->SetAttackInputHeld(true);
	SwordCombat->StartAttack();
}

void AGobulinPlayerCharacter::SwordAttackCompleted()
{
	if (SwordCombat)
	{
		SwordCombat->SetAttackInputHeld(false);
	}
}

void AGobulinPlayerCharacter::SetSprinting(bool bInSprinting)
{
	if (bDead)
	{
		bSprinting = false;
	}
	else if (bInSprinting && !CanSprintDuringSwordState())
	{
		bSprinting = false;
	}
	else
	{
		bSprinting = bInSprinting;
	}

	ApplyMovementSettings();
}

bool AGobulinPlayerCharacter::CanJumpDuringSwordState() const
{
	if (!SwordCombat)
	{
		return true;
	}

	switch (SwordCombat->GetAttackState())
	{
	case EGobulinSwordAttackState::Attacking:
		return bAllowJumpDuringSwordAttack;
	case EGobulinSwordAttackState::Recovery:
		return bAllowJumpDuringSwordRecovery;
	default:
		return true;
	}
}

bool AGobulinPlayerCharacter::CanSprintDuringSwordState() const
{
	if (!SwordCombat)
	{
		return true;
	}

	switch (SwordCombat->GetAttackState())
	{
	case EGobulinSwordAttackState::Attacking:
		return bAllowSprintDuringSwordAttack;
	case EGobulinSwordAttackState::Recovery:
		return bAllowSprintDuringSwordRecovery;
	default:
		return true;
	}
}

void AGobulinPlayerCharacter::OnSwordAttackStateChanged(
	EGobulinSwordAttackState PreviousState,
	EGobulinSwordAttackState NewState)
{
	(void)PreviousState;

	if (NewState == EGobulinSwordAttackState::Attacking && bCancelSprintOnSwordAttack)
	{
		bSprinting = false;
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

	const bool bSwordAttackActive = SwordCombat && SwordCombat->GetAttackState() == EGobulinSwordAttackState::Attacking;
	const bool bSwordRecoveryActive = SwordCombat && SwordCombat->GetAttackState() == EGobulinSwordAttackState::Recovery;
	const bool bSprintActive = bSprinting && CanSprintDuringSwordState();

	if (bSprintActive)
	{
		MovementSpeed *= SprintSpeedMultiplier;
	}

	if (bSwordAttackActive)
	{
		MovementSpeed *= SwordAttackMoveSpeedMultiplier;
	}
	else if (bSwordRecoveryActive)
	{
		MovementSpeed *= SwordRecoveryMoveSpeedMultiplier;
	}

	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
}

void AGobulinPlayerCharacter::OnAttributeChanged(FGameplayTag AttributeTag, float NewValue)
{
	if (AttributeTag.MatchesTagExact(BattleTag_Movement_Speed))
	{
		ApplyMovementSettings();
	}
}

FCombatDamageResult AGobulinPlayerCharacter::ResolveCombatDamage_Implementation(const FCombatDamageRequest& Request)
{
	FCombatDamageResult Result;
	Result.CommandId = Request.CommandId;
	Result.Source = Request.Source;
	Result.Target = Request.Target;
	Result.RequestedAmount = FMath::IsFinite(Request.BaseAmount) ? FMath::Max(0.0f, Request.BaseAmount) : 0.0f;

	if (Attributes)
	{
		Result.RemainingHealth = Attributes->GetAttributeValue(BattleTag_Health_Current);
	}

	if (Request.Target != CombatantHandle)
	{
		Result.Result = ECombatDamageResult::InvalidTarget;
		return Result;
	}

	if (bDead)
	{
		Result.Result = ECombatDamageResult::AlreadyDead;
		return Result;
	}

	if (!Attributes || !Request.IsValid())
	{
		Result.Result = ECombatDamageResult::InvalidRequest;
		return Result;
	}

	const float CurrentHealth = Attributes->GetAttributeValue(BattleTag_Health_Current);
	if (CurrentHealth <= 0.0f)
	{
		Result.Result = ECombatDamageResult::AlreadyDead;
		return Result;
	}

	const float MaximumAppliedAmount = Request.HasFlag(ECombatDamageFlags::CannotKill)
		? FMath::Max(0.0f, CurrentHealth - 1.0f)
		: CurrentHealth;
	const float AppliedAmount = FMath::Min(MaximumAppliedAmount, Request.BaseAmount);
	if (AppliedAmount <= KINDA_SMALL_NUMBER)
	{
		Result.Result = ECombatDamageResult::Blocked;
		return Result;
	}

	const float NewHealth = FMath::Max(0.0f, CurrentHealth - AppliedAmount);
	Attributes->SetBaseAttribute(BattleTag_Health_Current, NewHealth, 0.0f, 99999.0f);

	Result.Result = ECombatDamageResult::Applied;
	Result.AppliedAmount = AppliedAmount;
	Result.RemainingHealth = NewHealth;
	Result.ReactionTag = CombatTag_Reaction_Hit;
	Result.bKilled = NewHealth <= 0.0f;

	if (Result.bKilled)
	{
		Die();
	}

	return Result;
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

	if (UCombatantRegistrySubsystem* Registry = GetWorld()
		? GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>()
		: nullptr)
	{
		Registry->SetCombatantActive(CombatantHandle, false);
	}

	if (SwordCombat)
	{
		SwordCombat->CancelAttack();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
