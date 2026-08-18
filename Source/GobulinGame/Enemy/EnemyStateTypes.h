#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EnemyStateTypes.generated.h"

UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Inactive,
	Spawning,
	SeekingTarget,
	Moving,
	ReadyToAttack,
	AttackWindup,
	AttackActive,
	AttackRecovery,
	HitReacting,
	Staggered,
	Dying
};

/** Resulting facts from one accepted enemy state transition. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyStateTransition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState PreviousState = EEnemyState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState NewState = EEnemyState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|State")
	float StateStartTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|State")
	float StateEndTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|State")
	int32 StateSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy|State", meta = (Categories = "Combat.Enemy.StateReason"))
	FGameplayTag Reason;
};

/** Compact mutable state shared semantically by Actor and Mass enemy backends. */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyStateData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState CurrentState = EEnemyState::Inactive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	EEnemyState PreviousState = EEnemyState::Inactive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	float StateStartTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	float StateEndTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	int32 StateSequence = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State", meta = (Categories = "Combat.Enemy.StateReason"))
	FGameplayTag LastTransitionReason;

	bool CanTransitionTo(EEnemyState NewState) const;

	bool TryTransition(
		EEnemyState NewState,
		float WorldTime,
		float Duration,
		FGameplayTag Reason,
		FEnemyStateTransition* OutTransition = nullptr);
};
