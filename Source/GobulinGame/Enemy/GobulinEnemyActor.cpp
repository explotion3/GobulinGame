#include "Enemy/GobulinEnemyActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Core/GobulinCollisionChannels.h"
#include "Enemy/GobulinEnemyAIController.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Enemy/GobulinEnemyPresentationComponent.h"
#include "Enemy/GobulinEnemySubsystem.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/CharacterMovementComponent.h"

AGobulinEnemyActor::AGobulinEnemyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	UCapsuleComponent* CollisionCapsule = GetCapsuleComponent();
	CollisionCapsule->InitCapsuleSize(34.0f, 88.0f);
	CollisionCapsule->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
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
	Movement->bCanWalkOffLedges = false;
	Movement->bUseRVOAvoidance = true;
	Movement->AvoidanceConsiderationRadius = 250.0f;

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
	GetCharacterMovement()->SetAvoidanceEnabled(Archetype.bUseRVOAvoidance);
	GetCharacterMovement()->AvoidanceConsiderationRadius = Archetype.AvoidanceConsiderationRadius;
	FeetAnchor->SetRelativeLocation(FVector(0.0f, 0.0f, -Archetype.Body.CapsuleHalfHeight));
	PresentationComponent->ApplyDefinition(Archetype.Presentation);
	ApplyEnemyState(EEnemyState::Spawning);
}

void AGobulinEnemyActor::ApplyEnemyState(EEnemyState NewState)
{
	if (!PresentationComponent)
	{
		return;
	}

	PresentationComponent->ApplyEnemyState(NewState);
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

void AGobulinEnemyActor::StopEnemyMovement()
{
	if (AGobulinEnemyAIController* EnemyController = Cast<AGobulinEnemyAIController>(GetController()))
	{
		EnemyController->StopEnemyMove();
	}
	GetCharacterMovement()->StopMovementImmediately();
}

void AGobulinEnemyActor::SetEnemyCollisionEnabled(bool bEnabled)
{
	GetCapsuleComponent()->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (!bEnabled)
	{
		StopEnemyMovement();
		GetCharacterMovement()->DisableMovement();
	}
}

void AGobulinEnemyActor::ReleaseEnemyHandle()
{
	CombatantHandle.Reset();
	ArchetypeId = FPrimaryAssetId();
}
