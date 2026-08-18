#include "Enemy/EnemyStateTypes.h"

bool FEnemyStateData::CanTransitionTo(EEnemyState NewState) const
{
	if (NewState == CurrentState)
	{
		return false;
	}

	// Inactive is the common terminal state used by every backend when an enemy is retired.
	if (NewState == EEnemyState::Inactive)
	{
		return true;
	}

	switch (CurrentState)
	{
	case EEnemyState::Inactive:
		return NewState == EEnemyState::Spawning;

	case EEnemyState::Spawning:
		return NewState == EEnemyState::SeekingTarget
			|| NewState == EEnemyState::Moving
			|| NewState == EEnemyState::Dying;

	case EEnemyState::SeekingTarget:
		return NewState == EEnemyState::Moving
			|| NewState == EEnemyState::ReadyToAttack
			|| NewState == EEnemyState::AttackWindup
			|| NewState == EEnemyState::Staggered
			|| NewState == EEnemyState::Dying;

	case EEnemyState::Moving:
		return NewState == EEnemyState::SeekingTarget
			|| NewState == EEnemyState::ReadyToAttack
			|| NewState == EEnemyState::AttackWindup
			|| NewState == EEnemyState::Staggered
			|| NewState == EEnemyState::Dying;

	case EEnemyState::ReadyToAttack:
		return NewState == EEnemyState::SeekingTarget
			|| NewState == EEnemyState::Moving
			|| NewState == EEnemyState::AttackWindup
			|| NewState == EEnemyState::Staggered
			|| NewState == EEnemyState::Dying;

	case EEnemyState::AttackWindup:
		return NewState == EEnemyState::AttackActive
			|| NewState == EEnemyState::Staggered
			|| NewState == EEnemyState::Dying;

	case EEnemyState::AttackActive:
		return NewState == EEnemyState::AttackRecovery
			|| NewState == EEnemyState::Staggered
			|| NewState == EEnemyState::Dying;

	case EEnemyState::AttackRecovery:
		return NewState == EEnemyState::SeekingTarget
			|| NewState == EEnemyState::Moving
			|| NewState == EEnemyState::ReadyToAttack
			|| NewState == EEnemyState::AttackWindup
			|| NewState == EEnemyState::Staggered
			|| NewState == EEnemyState::Dying;

	case EEnemyState::Staggered:
		return NewState == EEnemyState::SeekingTarget
			|| NewState == EEnemyState::Moving
			|| NewState == EEnemyState::ReadyToAttack
			|| NewState == EEnemyState::Dying;

	case EEnemyState::Dying:
		return NewState == EEnemyState::Inactive;

	default:
		return false;
	}
}

bool FEnemyStateData::TryTransition(
	EEnemyState NewState,
	float WorldTime,
	float Duration,
	FGameplayTag Reason,
	FEnemyStateTransition* OutTransition)
{
	if (!CanTransitionTo(NewState) || !FMath::IsFinite(WorldTime) || !FMath::IsFinite(Duration))
	{
		return false;
	}

	const EEnemyState OldState = CurrentState;
	PreviousState = OldState;
	CurrentState = NewState;
	StateStartTime = WorldTime;
	StateEndTime = Duration > 0.0f ? WorldTime + Duration : 0.0f;
	++StateSequence;
	LastTransitionReason = Reason;

	if (OutTransition)
	{
		OutTransition->PreviousState = OldState;
		OutTransition->NewState = NewState;
		OutTransition->StateStartTime = StateStartTime;
		OutTransition->StateEndTime = StateEndTime;
		OutTransition->StateSequence = StateSequence;
		OutTransition->Reason = Reason;
	}

	return true;
}
