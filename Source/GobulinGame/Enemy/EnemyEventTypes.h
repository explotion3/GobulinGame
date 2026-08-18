#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "Enemy/EnemySpawnProtocol.h"
#include "Enemy/EnemyNavigationTypes.h"
#include "Enemy/EnemyStateTypes.h"
#include "GameplayTagContainer.h"
#include "EnemyEventTypes.generated.h"

UENUM(BlueprintType)
enum class EEnemyAttackEventPhase : uint8
{
	WindupStarted,
	Committed,
	Finished,
	Interrupted
};

UENUM(BlueprintType)
enum class EEnemyRetireReason : uint8
{
	DeathCompleted,
	OutOfRange,
	WaveEnded,
	ExternalRemoval,
	WorldCleanup
};

/** Completion event for every spawn request, including failures. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemySpawnResolvedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FEnemySpawnResultData Result;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemySpawnedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatCommandId CommandId;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FPrimaryAssetId EnemyDefinitionId;

	/** 实际采用的地面锚点，而不是 Actor 胶囊体中心。 */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	int32 SpawnGroupId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyStateChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FEnemyStateTransition Transition;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyTargetChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle PreviousTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle NewTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	EEnemyTargetChangeReason Reason = EEnemyTargetChangeReason::Acquired;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	int32 EventSequence = 0;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyMoveStatusChangedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	EEnemyMoveStatus PreviousStatus = EEnemyMoveStatus::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	EEnemyMoveStatus NewStatus = EEnemyMoveStatus::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FEnemyMoveIntent Intent;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	int32 EventSequence = 0;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyAttackEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatCommandId CommandId;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Target;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events", meta = (Categories = "Combat.Attack"))
	FGameplayTag AttackTag;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	EEnemyAttackEventPhase Phase = EEnemyAttackEventPhase::WindupStarted;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	int32 EventSequence = 0;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyDiedEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatCommandId CommandId;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Killer;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events", meta = (Categories = "Combat.Attack"))
	FGameplayTag AttackTag;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events", meta = (Categories = "Combat.Damage"))
	FGameplayTag DamageType;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FVector DeathLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	int32 EventSequence = 0;
};

USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyRetiredEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FCombatantHandle Enemy;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	EEnemyRetireReason Reason = EEnemyRetireReason::WorldCleanup;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	FVector LastLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|Events")
	int32 EventSequence = 0;
};
