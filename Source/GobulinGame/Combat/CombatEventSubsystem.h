#pragma once

#include "CoreMinimal.h"
#include "Combat/DamageProtocol.h"
#include "Containers/Queue.h"
#include "Enemy/EnemyEventTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "CombatEventSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCombatDamageResolvedDelegate, const FCombatDamageResolvedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemySpawnResolvedDelegate, const FEnemySpawnResolvedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemySpawnedDelegate, const FEnemySpawnedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyStateChangedDelegate, const FEnemyStateChangedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyTargetChangedDelegate, const FEnemyTargetChangedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyMoveStatusChangedDelegate, const FEnemyMoveStatusChangedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyAttackDelegate, const FEnemyAttackEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyDiedDelegate, const FEnemyDiedEvent&);
DECLARE_MULTICAST_DELEGATE_OneParam(FEnemyRetiredDelegate, const FEnemyRetiredEvent&);

/**
 * World-level typed event stream. Producers may enqueue from multiple threads; listeners
 * receive confirmed facts once per frame on the game thread in deterministic category order.
 */
UCLASS()
class GOBULINGAME_API UCombatEventSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

	void EnqueueDamageResolved(const FCombatDamageResolvedEvent& Event);
	void EnqueueEnemySpawnResolved(const FEnemySpawnResolvedEvent& Event);
	void EnqueueEnemySpawned(const FEnemySpawnedEvent& Event);
	void EnqueueEnemyStateChanged(const FEnemyStateChangedEvent& Event);
	void EnqueueEnemyTargetChanged(const FEnemyTargetChangedEvent& Event);
	void EnqueueEnemyMoveStatusChanged(const FEnemyMoveStatusChangedEvent& Event);
	void EnqueueEnemyAttack(const FEnemyAttackEvent& Event);
	void EnqueueEnemyDied(const FEnemyDiedEvent& Event);
	void EnqueueEnemyRetired(const FEnemyRetiredEvent& Event);

	/** Public for deterministic tests and explicit frame-end integration. */
	void FlushEvents();

	FCombatDamageResolvedDelegate& OnDamageResolved() { return DamageResolvedDelegate; }
	FEnemySpawnResolvedDelegate& OnEnemySpawnResolved() { return EnemySpawnResolvedDelegate; }
	FEnemySpawnedDelegate& OnEnemySpawned() { return EnemySpawnedDelegate; }
	FEnemyStateChangedDelegate& OnEnemyStateChanged() { return EnemyStateChangedDelegate; }
	FEnemyTargetChangedDelegate& OnEnemyTargetChanged() { return EnemyTargetChangedDelegate; }
	FEnemyMoveStatusChangedDelegate& OnEnemyMoveStatusChanged() { return EnemyMoveStatusChangedDelegate; }
	FEnemyAttackDelegate& OnEnemyAttack() { return EnemyAttackDelegate; }
	FEnemyDiedDelegate& OnEnemyDied() { return EnemyDiedDelegate; }
	FEnemyRetiredDelegate& OnEnemyRetired() { return EnemyRetiredDelegate; }

private:
	TQueue<FEnemySpawnResolvedEvent, EQueueMode::Mpsc> EnemySpawnResolvedQueue;
	TQueue<FEnemySpawnedEvent, EQueueMode::Mpsc> EnemySpawnedQueue;
	TQueue<FEnemyTargetChangedEvent, EQueueMode::Mpsc> EnemyTargetChangedQueue;
	TQueue<FEnemyMoveStatusChangedEvent, EQueueMode::Mpsc> EnemyMoveStatusChangedQueue;
	TQueue<FEnemyAttackEvent, EQueueMode::Mpsc> EnemyAttackQueue;
	TQueue<FCombatDamageResolvedEvent, EQueueMode::Mpsc> DamageResolvedQueue;
	TQueue<FEnemyStateChangedEvent, EQueueMode::Mpsc> EnemyStateChangedQueue;
	TQueue<FEnemyDiedEvent, EQueueMode::Mpsc> EnemyDiedQueue;
	TQueue<FEnemyRetiredEvent, EQueueMode::Mpsc> EnemyRetiredQueue;

	FCombatDamageResolvedDelegate DamageResolvedDelegate;
	FEnemySpawnResolvedDelegate EnemySpawnResolvedDelegate;
	FEnemySpawnedDelegate EnemySpawnedDelegate;
	FEnemyStateChangedDelegate EnemyStateChangedDelegate;
	FEnemyTargetChangedDelegate EnemyTargetChangedDelegate;
	FEnemyMoveStatusChangedDelegate EnemyMoveStatusChangedDelegate;
	FEnemyAttackDelegate EnemyAttackDelegate;
	FEnemyDiedDelegate EnemyDiedDelegate;
	FEnemyRetiredDelegate EnemyRetiredDelegate;

	bool bIsFlushing = false;
};
