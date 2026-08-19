#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "Combat/CombatantSnapshot.h"
#include "Enemy/EnemyContactDamageTypes.h"
#include "Enemy/EnemyCrowdTypes.h"
#include "Enemy/EnemyNavigationTypes.h"
#include "Enemy/EnemyReactionTypes.h"
#include "Enemy/EnemyStateTypes.h"
#include "UObject/PrimaryAssetId.h"
#include "GobulinEnemyRuntimeData.generated.h"

/**
 * Immutable values derived from one archetype and one spawn request.
 * These fields are deliberately flat so they can later move into a Mass const-shared fragment.
 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyRuntimeStats
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float MaxHealth = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float MoveSpeed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float TargetAcquisitionRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float TargetLoseRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FGobulinEnemyContactDamageDefinition ContactDamage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float DecisionInterval = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float NavigationRetryDelay = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float SpawnDuration = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FGobulinEnemyReactionDefinition Reaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FGobulinEnemyCrowdDefinition Crowd;
};

/** Backend-neutral mutable data for one enemy instance. Contains no Actor or Mass handle. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FCombatantHandle Handle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FPrimaryAssetId ArchetypeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FCombatantHandle Owner;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FEnemyTargetData Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	uint8 TeamId = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	int32 EnemyLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float PowerScale = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FGobulinEnemyRuntimeStats Stats;

	/** 本实例缩放后的逻辑胶囊；接触判定与注册快照使用同一数值。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FCombatantBodyShape BodyShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float CurrentHealth = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FEnemyStateData State;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FEnemyMovementData Movement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FEnemyContactDamageData ContactDamage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FGobulinEnemyReactionData Reaction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	FGobulinEnemyCrowdData Crowd;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	float NextBehaviorUpdateTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Runtime")
	int32 EventSequence = 0;

	bool IsAlive() const
	{
		return CurrentHealth > 0.0f
			&& State.CurrentState != EEnemyState::Dying
			&& State.CurrentState != EEnemyState::Inactive;
	}
};
