#include "Enemy/GobulinEnemyActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Core/GobulinCollisionChannels.h"
#include "Enemy/GobulinEnemyAIController.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Enemy/GobulinEnemyMovementComponent.h"
#include "Enemy/GobulinEnemyPresentationComponent.h"
#include "Enemy/GobulinEnemySubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"


AGobulinEnemyActor::AGobulinEnemyActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UGobulinEnemyMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	UCapsuleComponent* CollisionCapsule = GetCapsuleComponent();
	CollisionCapsule->InitCapsuleSize(34.0f, 88.0f);
	CollisionCapsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CollisionCapsule->SetCollisionObjectType(GobulinCollision::EnemyBody);
	CollisionCapsule->SetCollisionResponseToChannel(GobulinCollision::EnemyBody, ECR_Ignore);
	CollisionCapsule->SetCollisionResponseToChannel(GobulinCollision::EnemySwarmBoundary, ECR_Block);
	CollisionCapsule->SetCollisionResponseToChannel(GobulinCollision::EnemyCorpse, ECR_Block);
	CollisionCapsule->SetCollisionResponseToChannel(GobulinCollision::CombatTrace, ECR_Overlap);
	CollisionCapsule->SetGenerateOverlapEvents(false);
	CollisionCapsule->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;

	GetMesh()->SetVisibility(false, true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCastShadow(false);
	GetMesh()->SetComponentTickEnabled(false);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->bUseControllerDesiredRotation = false;
	Movement->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	Movement->MaxWalkSpeed = 300.0f;
	Movement->MaxAcceleration = 1200.0f;
	Movement->BrakingDecelerationWalking = 1200.0f;
	Movement->bCanWalkOffLedges = true;
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 250.0f;
	GetEnemyMovementComponent()->ConfigureGroundSupport(3.0f, 8.0f);

	AIControllerClass = AGobulinEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Spawned;

	FeetAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FeetAnchor"));
	FeetAnchor->SetupAttachment(GetCapsuleComponent());
	FeetAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));

	PresentationComponent = CreateDefaultSubobject<UGobulinEnemyPresentationComponent>(TEXT("Presentation"));
	PresentationComponent->SetupAttachment(FeetAnchor);

	bReplicates = false;
}

void AGobulinEnemyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CombatantHandle.IsSet())
	{
		if (UGobulinEnemySubsystem* EnemySubsystem = GetWorld()
			? GetWorld()->GetSubsystem<UGobulinEnemySubsystem>()
			: nullptr)
		{
			EnemySubsystem->NotifyEnemyActorEndPlay(this, CombatantHandle, EndPlayReason);
		}
	}

	CombatantHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void AGobulinEnemyActor::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (PresentationComponent
		&& PresentationComponent->GetCurrentVisualState() == EGobulinEnemyVisualState::Death)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		PresentationComponent->NotifyDeathLanded();
	}
}

FCombatDamageResult AGobulinEnemyActor::ResolveCombatDamage_Implementation(const FCombatDamageRequest& Request)
{
	if (UGobulinEnemySubsystem* EnemySubsystem = GetWorld()
		? GetWorld()->GetSubsystem<UGobulinEnemySubsystem>()
		: nullptr)
	{
		return EnemySubsystem->ResolveEnemyDamage(CombatantHandle, Request);
	}

	FCombatDamageResult Result;
	Result.CommandId = Request.CommandId;
	Result.Source = Request.Source;
	Result.Target = Request.Target;
	Result.RequestedAmount = FMath::IsFinite(Request.BaseAmount) ? FMath::Max(0.0f, Request.BaseAmount) : 0.0f;
	Result.Result = ECombatDamageResult::InvalidTarget;
	return Result;
}

void AGobulinEnemyActor::InitializeEnemy(FCombatantHandle InHandle, const UGobulinEnemyArchetype& Archetype)
{
	CombatantHandle = InHandle;
	ArchetypeId = Archetype.GetPrimaryAssetId();

	GetCapsuleComponent()->SetCapsuleSize(
		Archetype.Body.CapsuleRadius,
		Archetype.Body.CapsuleHalfHeight,
		true);
	GetCharacterMovement()->MaxWalkSpeed = Archetype.MoveSpeed;
	GetCharacterMovement()->SetAvoidanceEnabled(
		Archetype.bUseRVOAvoidance && !Archetype.Crowd.bEnableContinuousPiling);
	GetCharacterMovement()->AvoidanceConsiderationRadius = Archetype.AvoidanceConsiderationRadius;
	GetEnemyMovementComponent()->ConfigureGroundSupport(
		Archetype.Crowd.GroundSupportRadius,
		Archetype.Crowd.GroundSnapDownHeight);
	GetCapsuleComponent()->SetCollisionResponseToChannel(
		GobulinCollision::EnemyBody,
		Archetype.Crowd.bEnableContinuousPiling ? ECR_Ignore : ECR_Block);
	ReactionDefinition = Archetype.Reaction;
	FeetAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, -Archetype.Body.CapsuleHalfHeight));
	PresentationComponent->ApplyDefinition(Archetype.Presentation);
	ApplyEnemyState(EEnemyState::Spawning);
}

UGobulinEnemyMovementComponent* AGobulinEnemyActor::GetEnemyMovementComponent() const
{
	return CastChecked<UGobulinEnemyMovementComponent>(GetCharacterMovement());
}

void AGobulinEnemyActor::ApplyEnemyState(EEnemyState NewState)
{
	if (!PresentationComponent)
	{
		return;
	}

	PresentationComponent->ApplyEnemyState(NewState);
	PresentationComponent->SetLocomotionSuspended(
		NewState == EEnemyState::HitReacting || NewState == EEnemyState::Staggered);
	if (NewState == EEnemyState::Dying)
	{
		PresentationComponent->BeginDeathEffects(ReactionDefinition);
	}
}

EEnemyMoveStatus AGobulinEnemyActor::RequestMoveToTarget(
	AActor* TargetActor,
	const FEnemyMoveIntent& Intent)
{
	GetCharacterMovement()->MaxWalkSpeed = Intent.DesiredSpeed;
	AGobulinEnemyAIController* EnemyController = Cast<AGobulinEnemyAIController>(GetController());
	if (!EnemyController)
	{
		SpawnDefaultController();
		EnemyController = Cast<AGobulinEnemyAIController>(GetController());
	}

	return EnemyController
		? EnemyController->RequestMoveToTarget(TargetActor, Intent)
		: EEnemyMoveStatus::Failed;
}

bool AGobulinEnemyActor::HasCompleteNavigationPathToTarget(
	AActor* TargetActor,
	const FEnemyMoveIntent& Intent) const
{
	const AGobulinEnemyAIController* EnemyController =
		Cast<AGobulinEnemyAIController>(GetController());
	return EnemyController && EnemyController->HasCompletePathToTarget(TargetActor, Intent);
}

void AGobulinEnemyActor::StopEnemyMovement()
{
	if (AGobulinEnemyAIController* EnemyController = Cast<AGobulinEnemyAIController>(GetController()))
	{
		EnemyController->StopEnemyMove();
	}
	GetCharacterMovement()->StopMovementImmediately();
}

FVector AGobulinEnemyActor::ApplyCrowdVelocityChange(
	const FVector& VelocityChange,
	float MaximumLiftSpeed,
	float DeltaTime)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement
		|| Movement->MovementMode == MOVE_None
		|| VelocityChange.ContainsNaN()
		|| !FMath::IsFinite(DeltaTime)
		|| DeltaTime <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	FVector AppliedVelocityChange = FVector::ZeroVector;
	const FVector HorizontalChange(VelocityChange.X, VelocityChange.Y, 0.0f);
	const float MaximumAcceleration = FMath::Max(0.0f, Movement->GetMaxAcceleration());
	const float MaximumInputVelocityChange = MaximumAcceleration * DeltaTime;
	if (!HorizontalChange.IsNearlyZero() && MaximumInputVelocityChange > KINDA_SMALL_NUMBER)
	{
		const float AppliedMagnitude = FMath::Min(
			HorizontalChange.Size(),
			MaximumInputVelocityChange);
		const float InputScale = AppliedMagnitude / MaximumInputVelocityChange;
		AddMovementInput(HorizontalChange.GetSafeNormal(), InputScale, true);
		AppliedVelocityChange = HorizontalChange.GetSafeNormal() * AppliedMagnitude;
	}

	if (VelocityChange.Z > 0.0f)
	{
		const float RemainingLiftSpeed = FMath::Max(
			0.0f,
			MaximumLiftSpeed - Movement->Velocity.Z);
		AppliedVelocityChange.Z = FMath::Min(VelocityChange.Z, RemainingLiftSpeed);
		if (AppliedVelocityChange.Z > KINDA_SMALL_NUMBER)
		{
			Movement->AddImpulse(FVector(0.0f, 0.0f, AppliedVelocityChange.Z), true);
		}
	}
	return AppliedVelocityChange;
}

FVector AGobulinEnemyActor::ApplyCrowdFallbackDrive(
	const FVector& WorldDirection,
	float DesiredSpeed,
	const FGobulinEnemyCrowdDefinition& CrowdDefinition,
	float DeltaTime)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const FVector Direction = WorldDirection.GetSafeNormal2D();
	if (!Movement
		|| Movement->MovementMode == MOVE_None
		|| Direction.IsNearlyZero()
		|| !FMath::IsFinite(DesiredSpeed)
		|| !FMath::IsFinite(CrowdDefinition.FallbackDriveAcceleration)
		|| !FMath::IsFinite(DeltaTime)
		|| DeltaTime <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	Movement->MaxWalkSpeed = FMath::Max(0.0f, DesiredSpeed);
	const float MaximumAcceleration = FMath::Max(0.0f, Movement->GetMaxAcceleration());
	const float RequestedAcceleration = FMath::Min(
		FMath::Max(0.0f, CrowdDefinition.FallbackDriveAcceleration),
		MaximumAcceleration);
	if (RequestedAcceleration <= KINDA_SMALL_NUMBER || MaximumAcceleration <= KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}

	AddMovementInput(Direction, RequestedAcceleration / MaximumAcceleration, true);
	return Direction * RequestedAcceleration * DeltaTime;
}

void AGobulinEnemyActor::ApplyEnemyImpact(const FVector& LaunchVelocity, bool bLethal)
{
	StopEnemyMovement();
	if (PresentationComponent)
	{
		PresentationComponent->BeginHitFlash(bLethal, ReactionDefinition);
	}

	if (!LaunchVelocity.IsNearlyZero())
	{
		LaunchCharacter(LaunchVelocity, true, true);
	}
}

void AGobulinEnemyActor::BeginDeathPhysics(const FVector& LaunchVelocity)
{
	UCapsuleComponent* CollisionCapsule = GetCapsuleComponent();
	CollisionCapsule->SetCapsuleSize(
		CollisionCapsule->GetUnscaledCapsuleRadius() * ReactionDefinition.CorpseCapsuleRadiusScale,
		CollisionCapsule->GetUnscaledCapsuleHalfHeight(),
		true);
	CollisionCapsule->SetCollisionObjectType(GobulinCollision::EnemyCorpse);
	CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionCapsule->SetCollisionResponseToChannel(GobulinCollision::EnemyCorpse, ECR_Block);
	CollisionCapsule->SetCollisionResponseToChannel(GobulinCollision::EnemySwarmBoundary, ECR_Block);
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionCapsule->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	CollisionCapsule->SetWalkableSlopeOverride(
		FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.0f));

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->SetAvoidanceEnabled(false);
	Movement->GravityScale = ReactionDefinition.DeathGravityScale;
	ApplyEnemyImpact(LaunchVelocity, true);
}

bool AGobulinEnemyActor::IsEnemyGrounded() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	return Movement && !Movement->IsFalling();
}

bool AGobulinEnemyActor::IsDeathPresentationComplete() const
{
	return PresentationComponent && PresentationComponent->IsDeathPresentationComplete();
}

void AGobulinEnemyActor::ReleaseEnemyHandle()
{
	CombatantHandle.Reset();
	ArchetypeId = FPrimaryAssetId();
}
