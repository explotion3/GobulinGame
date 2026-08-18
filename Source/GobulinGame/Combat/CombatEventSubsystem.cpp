#include "Combat/CombatEventSubsystem.h"

void UCombatEventSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;
	FlushEvents();
}

TStatId UCombatEventSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCombatEventSubsystem, STATGROUP_Tickables);
}

void UCombatEventSubsystem::Deinitialize()
{
	FlushEvents();
	DamageResolvedDelegate.Clear();
	EnemySpawnResolvedDelegate.Clear();
	EnemySpawnedDelegate.Clear();
	EnemyTargetChangedDelegate.Clear();
	EnemyMoveStatusChangedDelegate.Clear();
	EnemyStateChangedDelegate.Clear();
	EnemyAttackDelegate.Clear();
	EnemyDiedDelegate.Clear();
	EnemyRetiredDelegate.Clear();
	Super::Deinitialize();
}

void UCombatEventSubsystem::EnqueueDamageResolved(const FCombatDamageResolvedEvent& Event)
{
	DamageResolvedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemySpawnResolved(const FEnemySpawnResolvedEvent& Event)
{
	EnemySpawnResolvedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemySpawned(const FEnemySpawnedEvent& Event)
{
	EnemySpawnedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemyStateChanged(const FEnemyStateChangedEvent& Event)
{
	EnemyStateChangedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemyTargetChanged(const FEnemyTargetChangedEvent& Event)
{
	EnemyTargetChangedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemyMoveStatusChanged(const FEnemyMoveStatusChangedEvent& Event)
{
	EnemyMoveStatusChangedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemyAttack(const FEnemyAttackEvent& Event)
{
	EnemyAttackQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemyDied(const FEnemyDiedEvent& Event)
{
	EnemyDiedQueue.Enqueue(Event);
}

void UCombatEventSubsystem::EnqueueEnemyRetired(const FEnemyRetiredEvent& Event)
{
	EnemyRetiredQueue.Enqueue(Event);
}

void UCombatEventSubsystem::FlushEvents()
{
	check(IsInGameThread());

	if (bIsFlushing)
	{
		return;
	}

	TGuardValue<bool> FlushingGuard(bIsFlushing, true);

	FEnemySpawnResolvedEvent SpawnResolvedEvent;
	while (EnemySpawnResolvedQueue.Dequeue(SpawnResolvedEvent))
	{
		EnemySpawnResolvedDelegate.Broadcast(SpawnResolvedEvent);
	}

	FEnemySpawnedEvent SpawnedEvent;
	while (EnemySpawnedQueue.Dequeue(SpawnedEvent))
	{
		EnemySpawnedDelegate.Broadcast(SpawnedEvent);
	}

	FEnemyTargetChangedEvent TargetChangedEvent;
	while (EnemyTargetChangedQueue.Dequeue(TargetChangedEvent))
	{
		EnemyTargetChangedDelegate.Broadcast(TargetChangedEvent);
	}

	FEnemyMoveStatusChangedEvent MoveStatusChangedEvent;
	while (EnemyMoveStatusChangedQueue.Dequeue(MoveStatusChangedEvent))
	{
		EnemyMoveStatusChangedDelegate.Broadcast(MoveStatusChangedEvent);
	}

	FEnemyAttackEvent AttackEvent;
	while (EnemyAttackQueue.Dequeue(AttackEvent))
	{
		EnemyAttackDelegate.Broadcast(AttackEvent);
	}

	FCombatDamageResolvedEvent DamageEvent;
	while (DamageResolvedQueue.Dequeue(DamageEvent))
	{
		DamageResolvedDelegate.Broadcast(DamageEvent);
	}

	FEnemyStateChangedEvent StateEvent;
	while (EnemyStateChangedQueue.Dequeue(StateEvent))
	{
		EnemyStateChangedDelegate.Broadcast(StateEvent);
	}

	FEnemyDiedEvent DiedEvent;
	while (EnemyDiedQueue.Dequeue(DiedEvent))
	{
		EnemyDiedDelegate.Broadcast(DiedEvent);
	}

	FEnemyRetiredEvent RetiredEvent;
	while (EnemyRetiredQueue.Dequeue(RetiredEvent))
	{
		EnemyRetiredDelegate.Broadcast(RetiredEvent);
	}
}
