#pragma once

#include "CoreMinimal.h"
#include "Combat/DamageProtocol.h"
#include "Containers/Queue.h"
#include "Enemy/EnemyEventTypes.h"
#include "Enemy/EnemySpawnProtocol.h"
#include "Enemy/GobulinEnemyRuntimeData.h"
#include "Subsystems/WorldSubsystem.h"
#include "GobulinEnemySubsystem.generated.h"

class AGobulinEnemyActor;
struct FCombatantSnapshot;
struct FStreamableHandle;
class UGobulinEnemyArchetype;

/**
 * World-authoritative enemy lifecycle service.
 * The current storage adapter uses Actors; protocol callers never depend on that representation.
 */
UCLASS()
class GOBULINGAME_API UGobulinEnemySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	/** Asynchronously resolves the archetype and submits one backend-neutral spawn request. */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	FCombatCommandId SpawnEnemy(FEnemySpawnRequest Request);

	/** 按输入顺序提交一批生成请求；当前 Actor 后端逐个执行，未来 Mass 后端可在内部批处理。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	TArray<FCombatCommandId> SpawnEnemies(const TArray<FEnemySpawnRequest>& Requests);

	/** 分配当前 World 内的生成组 ID，供一次批量生成共享。 */
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	int32 AllocateSpawnGroupId();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool RetireEnemy(FCombatantHandle Enemy, EEnemyRetireReason Reason = EEnemyRetireReason::WaveEnded);

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool GetEnemyRuntimeData(FCombatantHandle Enemy, FGobulinEnemyRuntimeData& OutRuntimeData) const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetActiveEnemyCount() const { return ActiveEnemies.Num(); }

	FCombatDamageResult ResolveEnemyDamage(FCombatantHandle Enemy, const FCombatDamageRequest& Request);
	void NotifyEnemyActorEndPlay(
		const AGobulinEnemyActor* Actor,
		FCombatantHandle Enemy,
		EEndPlayReason::Type EndPlayReason);
	void NotifyEnemyMoveCompleted(
		FCombatantHandle Enemy,
		int32 IntentSequence,
		EEnemyMoveStatus Status);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	struct FActorEnemyRecord
	{
		FGobulinEnemyRuntimeData RuntimeData;
		TWeakObjectPtr<AGobulinEnemyActor> Actor;
	};

	void HandleArchetypeLoaded(FCombatCommandId CommandId);
	void CompleteSpawn(const FEnemySpawnRequest& Request, UGobulinEnemyArchetype& Archetype);
	bool TransitionEnemy(
		FCombatantHandle Enemy,
		EEnemyState NewState,
		float Duration,
		FGameplayTag Reason);
	void UpdateEnemyBehavior(
		FCombatantHandle Enemy,
		float WorldTime,
		const TArray<FCombatantSnapshot>& Combatants);
	const FCombatantSnapshot* FindBestTarget(
		const FGobulinEnemyRuntimeData& RuntimeData,
		const FVector& EnemyLocation,
		const TArray<FCombatantSnapshot>& Combatants) const;
	void AssignEnemyTarget(
		FActorEnemyRecord& Record,
		const FCombatantSnapshot& Target,
		float WorldTime);
	void ClearEnemyTarget(
		FCombatantHandle Enemy,
		EEnemyTargetChangeReason Reason,
		FGameplayTag StateReason,
		float WorldTime);
	void SetEnemyMoveStatus(
		FActorEnemyRecord& Record,
		EEnemyMoveStatus NewStatus,
		float WorldTime);
	void EnterAttackReady(FCombatantHandle Enemy, FActorEnemyRecord& Record, float WorldTime);
	void PublishTargetChanged(
		FActorEnemyRecord& Record,
		FCombatantHandle PreviousTarget,
		const FVector& LastKnownLocation,
		EEnemyTargetChangeReason Reason) const;
	void PublishMoveStatusChanged(
		FActorEnemyRecord& Record,
		EEnemyMoveStatus PreviousStatus) const;

	bool RememberSpawnCommand(FCombatCommandId CommandId);
	void PublishSpawnResolved(
		const FEnemySpawnRequest& Request,
		EEnemySpawnResult Result,
		FCombatantHandle Enemy = FCombatantHandle()) const;
	void PublishEnemySpawned(
		const FEnemySpawnRequest& Request,
		FCombatantHandle Enemy,
		const FTransform& ActualTransform) const;
	void PublishEnemyRetired(FActorEnemyRecord& Record, EEnemyRetireReason Reason, const FVector& LastLocation) const;

	TMap<FCombatantHandle, FActorEnemyRecord> ActiveEnemies;
	TMap<FCombatCommandId, FEnemySpawnRequest> PendingSpawnRequests;
	TMap<FCombatCommandId, TSharedPtr<FStreamableHandle>> PendingLoadHandles;

	UPROPERTY(Transient)
	TMap<FPrimaryAssetId, TObjectPtr<UGobulinEnemyArchetype>> LoadedArchetypes;

	TSet<FCombatCommandId> RememberedSpawnCommands;
	TQueue<FCombatCommandId, EQueueMode::Spsc> SpawnCommandEvictionQueue;
	int32 NextSpawnGroupId = 1;
	bool bIsDeinitializing = false;

	static constexpr int32 MaxRememberedSpawnCommandCount = 32768;
};
