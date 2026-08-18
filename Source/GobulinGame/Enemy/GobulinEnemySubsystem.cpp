#include "Enemy/GobulinEnemySubsystem.h"

#include "Combat/CombatantRegistrySubsystem.h"
#include "Combat/CombatantSnapshot.h"
#include "Combat/CombatEventSubsystem.h"
#include "Combat/CombatSubsystem.h"
#include "Core/CombatTags.h"
#include "Enemy/GobulinEnemyActor.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Engine/AssetManager.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "Subsystems/SubsystemCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogGobulinEnemy, Log, All);

namespace
{
	bool IsBehaviorState(EEnemyState State)
	{
		return State == EEnemyState::SeekingTarget
			|| State == EEnemyState::Moving
			|| State == EEnemyState::ReadyToAttack;
	}

	bool IsHandleBefore(const FCombatantHandle& Left, const FCombatantHandle& Right)
	{
		return Left.GetIndex() != Right.GetIndex()
			? Left.GetIndex() < Right.GetIndex()
			: Left.GetGeneration() < Right.GetGeneration();
	}
}

void UGobulinEnemySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UCombatantRegistrySubsystem>();
	Collection.InitializeDependency<UCombatEventSubsystem>();
	Collection.InitializeDependency<UCombatSubsystem>();
	NextSpawnGroupId = 1;
	bIsDeinitializing = false;
}

void UGobulinEnemySubsystem::Deinitialize()
{
	bIsDeinitializing = true;

	for (TPair<FCombatCommandId, TSharedPtr<FStreamableHandle>>& Pair : PendingLoadHandles)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CancelHandle();
		}
	}
	PendingLoadHandles.Reset();
	PendingSpawnRequests.Reset();

	UCombatantRegistrySubsystem* Registry = GetWorld()
		? GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>()
		: nullptr;
	for (TPair<FCombatantHandle, FActorEnemyRecord>& Pair : ActiveEnemies)
	{
		if (Registry)
		{
			Registry->UnregisterHandle(Pair.Key);
		}
		if (AGobulinEnemyActor* Actor = Pair.Value.Actor.Get())
		{
			Actor->ReleaseEnemyHandle();
		}
	}
	ActiveEnemies.Reset();
	LoadedArchetypes.Reset();
	RememberedSpawnCommands.Reset();

	FCombatCommandId IgnoredCommand;
	while (SpawnCommandEvictionQueue.Dequeue(IgnoredCommand))
	{
	}

	Super::Deinitialize();
}

void UGobulinEnemySubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float WorldTime = World->GetTimeSeconds();
	TArray<FCombatantHandle> SpawnCompletions;
	TArray<FCombatantHandle> DeathCompletions;
	TArray<FCombatantHandle> MissingActors;
	TArray<FCombatantHandle> BehaviorUpdates;

	for (const TPair<FCombatantHandle, FActorEnemyRecord>& Pair : ActiveEnemies)
	{
		const FGobulinEnemyRuntimeData& RuntimeData = Pair.Value.RuntimeData;
		if (!Pair.Value.Actor.IsValid())
		{
			MissingActors.Add(Pair.Key);
			continue;
		}

		if (RuntimeData.State.StateEndTime <= 0.0f || WorldTime < RuntimeData.State.StateEndTime)
		{
			if (RuntimeData.IsAlive()
				&& IsBehaviorState(RuntimeData.State.CurrentState)
				&& WorldTime >= RuntimeData.NextBehaviorUpdateTime)
			{
				BehaviorUpdates.Add(Pair.Key);
			}
			continue;
		}

		if (RuntimeData.State.CurrentState == EEnemyState::Spawning)
		{
			SpawnCompletions.Add(Pair.Key);
		}
		else if (RuntimeData.State.CurrentState == EEnemyState::Dying)
		{
			DeathCompletions.Add(Pair.Key);
		}
	}

	for (FCombatantHandle Enemy : SpawnCompletions)
	{
		TransitionEnemy(
			Enemy,
			EEnemyState::SeekingTarget,
			0.0f,
			CombatTag_EnemyStateReason_SpawnCompleted);
	}

	for (FCombatantHandle Enemy : DeathCompletions)
	{
		RetireEnemy(Enemy, EEnemyRetireReason::DeathCompleted);
	}

	for (FCombatantHandle Enemy : MissingActors)
	{
		RetireEnemy(Enemy, EEnemyRetireReason::ExternalRemoval);
	}

	if (BehaviorUpdates.Num() > 0)
	{
		BehaviorUpdates.Sort(IsHandleBefore);
		TArray<FCombatantSnapshot> Combatants;
		if (const UCombatantRegistrySubsystem* Registry = World->GetSubsystem<UCombatantRegistrySubsystem>())
		{
			Registry->GetCombatantSnapshots(Combatants);
		}

		for (FCombatantHandle Enemy : BehaviorUpdates)
		{
			UpdateEnemyBehavior(Enemy, WorldTime, Combatants);
		}
	}
}

TStatId UGobulinEnemySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGobulinEnemySubsystem, STATGROUP_Tickables);
}

bool UGobulinEnemySubsystem::IsTickable() const
{
	return !bIsDeinitializing && ActiveEnemies.Num() > 0;
}

FCombatCommandId UGobulinEnemySubsystem::SpawnEnemy(FEnemySpawnRequest Request)
{
	check(IsInGameThread());

	UCombatSubsystem* Combat = GetWorld() ? GetWorld()->GetSubsystem<UCombatSubsystem>() : nullptr;
	if (!Combat)
	{
		return FCombatCommandId();
	}

	if (!Request.CommandId.IsSet())
	{
		Request.CommandId = Combat->AllocateCommandId();
	}

	if (!RememberSpawnCommand(Request.CommandId))
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::Duplicate);
		return Request.CommandId;
	}

	if (!Request.IsValid() || Request.EnemyDefinitionId.PrimaryAssetType != UGobulinEnemyArchetype::PrimaryAssetType)
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::InvalidRequest);
		return Request.CommandId;
	}

	if (TObjectPtr<UGobulinEnemyArchetype>* CachedArchetype = LoadedArchetypes.Find(Request.EnemyDefinitionId))
	{
		CompleteSpawn(Request, **CachedArchetype);
		return Request.CommandId;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	if (!AssetManager.GetPrimaryAssetPath(Request.EnemyDefinitionId).IsValid())
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::DefinitionUnavailable);
		return Request.CommandId;
	}

	PendingSpawnRequests.Add(Request.CommandId, Request);
	const TArray<FName> LoadBundles { UGobulinEnemyArchetype::PresentationBundle };
	TSharedPtr<FStreamableHandle> LoadHandle = AssetManager.LoadPrimaryAsset(
		Request.EnemyDefinitionId,
		LoadBundles,
		FStreamableDelegate::CreateUObject(
			this,
			&UGobulinEnemySubsystem::HandleArchetypeLoaded,
			Request.CommandId));

	if (LoadHandle.IsValid() && PendingSpawnRequests.Contains(Request.CommandId))
	{
		PendingLoadHandles.Add(Request.CommandId, MoveTemp(LoadHandle));
	}

	return Request.CommandId;
}

TArray<FCombatCommandId> UGobulinEnemySubsystem::SpawnEnemies(
	const TArray<FEnemySpawnRequest>& Requests)
{
	check(IsInGameThread());

	TArray<FCombatCommandId> CommandIds;
	CommandIds.Reserve(Requests.Num());
	for (const FEnemySpawnRequest& Request : Requests)
	{
		CommandIds.Add(SpawnEnemy(Request));
	}
	return CommandIds;
}

int32 UGobulinEnemySubsystem::AllocateSpawnGroupId()
{
	check(IsInGameThread());

	const int32 AllocatedId = NextSpawnGroupId;
	if (NextSpawnGroupId == MAX_int32)
	{
		NextSpawnGroupId = 1;
	}
	else
	{
		++NextSpawnGroupId;
	}
	return AllocatedId;
}

bool UGobulinEnemySubsystem::RetireEnemy(FCombatantHandle Enemy, EEnemyRetireReason Reason)
{
	check(IsInGameThread());

	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	if (!Record)
	{
		return false;
	}

	const FVector LastLocation = Record->Actor.IsValid()
		? Record->Actor->GetActorLocation()
		: FVector::ZeroVector;
	TransitionEnemy(Enemy, EEnemyState::Inactive, 0.0f, CombatTag_EnemyStateReason_Retired);

	FActorEnemyRecord RetiredRecord;
	if (!ActiveEnemies.RemoveAndCopyValue(Enemy, RetiredRecord))
	{
		return false;
	}

	if (UCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>())
	{
		Registry->UnregisterHandle(Enemy);
	}

	PublishEnemyRetired(RetiredRecord, Reason, LastLocation);
	if (AGobulinEnemyActor* Actor = RetiredRecord.Actor.Get())
	{
		Actor->ReleaseEnemyHandle();
		Actor->Destroy();
	}
	return true;
}

bool UGobulinEnemySubsystem::GetEnemyRuntimeData(
	FCombatantHandle Enemy,
	FGobulinEnemyRuntimeData& OutRuntimeData) const
{
	if (const FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy))
	{
		OutRuntimeData = Record->RuntimeData;
		return true;
	}
	return false;
}

FCombatDamageResult UGobulinEnemySubsystem::ResolveEnemyDamage(
	FCombatantHandle Enemy,
	const FCombatDamageRequest& Request)
{
	check(IsInGameThread());

	FCombatDamageResult Result;
	Result.CommandId = Request.CommandId;
	Result.Source = Request.Source;
	Result.Target = Request.Target;
	Result.RequestedAmount = FMath::IsFinite(Request.BaseAmount) ? FMath::Max(0.0f, Request.BaseAmount) : 0.0f;

	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	if (!Record || Request.Target != Enemy || !Request.IsValid() || !Record->Actor.IsValid())
	{
		Result.Result = Record ? ECombatDamageResult::InvalidRequest : ECombatDamageResult::InvalidTarget;
		return Result;
	}

	FGobulinEnemyRuntimeData& RuntimeData = Record->RuntimeData;
	Result.RemainingHealth = RuntimeData.CurrentHealth;
	if (!RuntimeData.IsAlive())
	{
		Result.Result = ECombatDamageResult::AlreadyDead;
		return Result;
	}

	const float MaximumAppliedAmount = Request.HasFlag(ECombatDamageFlags::CannotKill)
		? FMath::Max(0.0f, RuntimeData.CurrentHealth - 1.0f)
		: RuntimeData.CurrentHealth;
	const float AppliedAmount = FMath::Min(MaximumAppliedAmount, Request.BaseAmount);
	if (AppliedAmount <= KINDA_SMALL_NUMBER)
	{
		Result.Result = ECombatDamageResult::Blocked;
		return Result;
	}

	RuntimeData.CurrentHealth = FMath::Max(0.0f, RuntimeData.CurrentHealth - AppliedAmount);
	Result.Result = ECombatDamageResult::Applied;
	Result.AppliedAmount = AppliedAmount;
	Result.RemainingHealth = RuntimeData.CurrentHealth;
	Result.ReactionTag = CombatTag_Reaction_Hit;
	Result.bKilled = RuntimeData.CurrentHealth <= 0.0f;

	if (!Result.bKilled)
	{
		return Result;
	}

	if (UCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>())
	{
		Registry->SetCombatantActive(Enemy, false);
	}
	Record->RuntimeData.Target.Clear(GetWorld()->GetTimeSeconds());
	Record->RuntimeData.Movement.Status = EEnemyMoveStatus::Idle;
	Record->Actor->SetEnemyCollisionEnabled(false);
	TransitionEnemy(
		Enemy,
		EEnemyState::Dying,
		RuntimeData.Stats.DeathDuration,
		CombatTag_EnemyStateReason_Killed);

	if (UCombatEventSubsystem* Events = GetWorld()->GetSubsystem<UCombatEventSubsystem>())
	{
		FEnemyDiedEvent Event;
		Event.CommandId = Request.CommandId;
		Event.Enemy = Enemy;
		Event.Killer = Request.Source;
		Event.AttackTag = Request.AttackTag;
		Event.DamageType = Request.DamageType;
		Event.DeathLocation = Record->Actor->GetActorLocation();
		Event.EventSequence = ++RuntimeData.EventSequence;
		Events->EnqueueEnemyDied(Event);
	}

	return Result;
}

void UGobulinEnemySubsystem::NotifyEnemyActorEndPlay(
	const AGobulinEnemyActor* Actor,
	FCombatantHandle Enemy,
	EEndPlayReason::Type EndPlayReason)
{
	check(IsInGameThread());

	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	if (!Record || Record->Actor.Get() != Actor)
	{
		return;
	}

	const FVector LastLocation = Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
	if (!bIsDeinitializing)
	{
		TransitionEnemy(Enemy, EEnemyState::Inactive, 0.0f, CombatTag_EnemyStateReason_Retired);
	}

	FActorEnemyRecord RemovedRecord;
	ActiveEnemies.RemoveAndCopyValue(Enemy, RemovedRecord);
	if (UCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>())
	{
		Registry->UnregisterHandle(Enemy);
	}

	if (!bIsDeinitializing)
	{
		const EEnemyRetireReason Reason = EndPlayReason == EEndPlayReason::Destroyed
			? EEnemyRetireReason::ExternalRemoval
			: EEnemyRetireReason::WorldCleanup;
		PublishEnemyRetired(RemovedRecord, Reason, LastLocation);
	}
}

void UGobulinEnemySubsystem::NotifyEnemyMoveCompleted(
	FCombatantHandle Enemy,
	int32 IntentSequence,
	EEnemyMoveStatus Status)
{
	check(IsInGameThread());

	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	if (!Record
		|| !Record->RuntimeData.IsAlive()
		|| Record->RuntimeData.Movement.Intent.IntentSequence != IntentSequence
		|| Record->RuntimeData.Movement.Status != EEnemyMoveStatus::Moving)
	{
		return;
	}

	const float WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SetEnemyMoveStatus(*Record, Status, WorldTime);
	if (Status == EEnemyMoveStatus::Reached)
	{
		EnterAttackReady(Enemy, *Record, WorldTime);
	}
	else if (Status == EEnemyMoveStatus::Blocked || Status == EEnemyMoveStatus::Failed)
	{
		ClearEnemyTarget(
			Enemy,
			EEnemyTargetChangeReason::NavigationFailed,
			CombatTag_EnemyStateReason_NavigationFailed,
			WorldTime);
	}
}

bool UGobulinEnemySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game
		|| WorldType == EWorldType::PIE
		|| WorldType == EWorldType::GamePreview;
}

void UGobulinEnemySubsystem::HandleArchetypeLoaded(FCombatCommandId CommandId)
{
	PendingLoadHandles.Remove(CommandId);

	FEnemySpawnRequest Request;
	if (!PendingSpawnRequests.RemoveAndCopyValue(CommandId, Request) || bIsDeinitializing)
	{
		return;
	}

	UGobulinEnemyArchetype* Archetype = UAssetManager::Get()
		.GetPrimaryAssetObject<UGobulinEnemyArchetype>(Request.EnemyDefinitionId);
	if (!Archetype)
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::DefinitionUnavailable);
		return;
	}

	LoadedArchetypes.Add(Request.EnemyDefinitionId, Archetype);
	CompleteSpawn(Request, *Archetype);
}

void UGobulinEnemySubsystem::CompleteSpawn(
	const FEnemySpawnRequest& Request,
	UGobulinEnemyArchetype& Archetype)
{
	if (!Archetype.IsDefinitionValid())
	{
		UE_LOG(LogGobulinEnemy, Warning, TEXT("Enemy archetype %s contains invalid runtime values."), *Request.EnemyDefinitionId.ToString());
		PublishSpawnResolved(Request, EEnemySpawnResult::InvalidRequest);
		return;
	}

	UWorld* World = GetWorld();
	UCombatantRegistrySubsystem* Registry = World
		? World->GetSubsystem<UCombatantRegistrySubsystem>()
		: nullptr;
	if (!World || !Registry)
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::BackendUnavailable);
		return;
	}

	constexpr float GroundClearance = 1.0f;
	FTransform ActorSpawnTransform = Request.SpawnTransform;
	const FVector AbsoluteScale = ActorSpawnTransform.GetScale3D().GetAbs();
	const float RadialScale = FMath::Max(FMath::Max(AbsoluteScale.X, AbsoluteScale.Y), KINDA_SMALL_NUMBER);
	const float VerticalScale = FMath::Max(AbsoluteScale.Z, KINDA_SMALL_NUMBER);
	ActorSpawnTransform.AddToTranslation(
		FVector::UpVector * ((Archetype.Body.CapsuleHalfHeight * VerticalScale) + GroundClearance));

	FCollisionQueryParams CollisionQueryParams(SCENE_QUERY_STAT(GobulinEnemySpawn), false);
	const FCollisionShape SpawnShape = FCollisionShape::MakeCapsule(
		Archetype.Body.CapsuleRadius * RadialScale,
		Archetype.Body.CapsuleHalfHeight * VerticalScale);
	if (World->OverlapBlockingTestByChannel(
		ActorSpawnTransform.GetLocation(),
		ActorSpawnTransform.GetRotation(),
		ECC_Pawn,
		SpawnShape,
		CollisionQueryParams))
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::SpawnBlocked);
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Registry->ResolveActor(Request.Owner);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGobulinEnemyActor* Actor = World->SpawnActor<AGobulinEnemyActor>(
		AGobulinEnemyActor::StaticClass(),
		ActorSpawnTransform,
		SpawnParameters);
	if (!Actor)
	{
		PublishSpawnResolved(Request, EEnemySpawnResult::SpawnBlocked);
		return;
	}

	const FCombatantHandle Enemy = Registry->RegisterActor(Actor, Request.TeamId);
	if (!Enemy.IsSet())
	{
		Actor->Destroy();
		PublishSpawnResolved(Request, EEnemySpawnResult::BackendUnavailable);
		return;
	}

	Actor->InitializeEnemy(Enemy, Archetype);

	FActorEnemyRecord Record;
	Record.Actor = Actor;
	Record.RuntimeData.Handle = Enemy;
	Record.RuntimeData.ArchetypeId = Request.EnemyDefinitionId;
	Record.RuntimeData.Owner = Request.Owner;
	Record.RuntimeData.TeamId = Request.TeamId;
	Record.RuntimeData.EnemyLevel = Request.EnemyLevel;
	Record.RuntimeData.PowerScale = Request.PowerScale;
	Record.RuntimeData.Stats = Archetype.BuildRuntimeStats(Request.PowerScale);
	Record.RuntimeData.CurrentHealth = Record.RuntimeData.Stats.MaxHealth;
	const float BehaviorStagger = Record.RuntimeData.Stats.DecisionInterval
		* (static_cast<float>(Enemy.GetIndex() % 16) / 16.0f);
	Record.RuntimeData.NextBehaviorUpdateTime = World->GetTimeSeconds() + BehaviorStagger;
	ActiveEnemies.Add(Enemy, MoveTemp(Record));

	if (!TransitionEnemy(
		Enemy,
		EEnemyState::Spawning,
		Archetype.SpawnDuration,
		FGameplayTag()))
	{
		RetireEnemy(Enemy, EEnemyRetireReason::ExternalRemoval);
		PublishSpawnResolved(Request, EEnemySpawnResult::BackendUnavailable);
		return;
	}

	PublishSpawnResolved(Request, EEnemySpawnResult::Success, Enemy);
	FTransform ActualGroundTransform = Actor->GetActorTransform();
	ActualGroundTransform.AddToTranslation(
		-FVector::UpVector * ((Archetype.Body.CapsuleHalfHeight * VerticalScale) + GroundClearance));
	PublishEnemySpawned(Request, Enemy, ActualGroundTransform);
}

bool UGobulinEnemySubsystem::TransitionEnemy(
	FCombatantHandle Enemy,
	EEnemyState NewState,
	float Duration,
	FGameplayTag Reason)
{
	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	if (!Record || !GetWorld())
	{
		return false;
	}

	const bool bTimedLifecycleState = NewState == EEnemyState::Spawning || NewState == EEnemyState::Dying;
	const float EffectiveDuration = bTimedLifecycleState ? FMath::Max(Duration, KINDA_SMALL_NUMBER) : 0.0f;
	FEnemyStateTransition Transition;
	if (!Record->RuntimeData.State.TryTransition(
		NewState,
		GetWorld()->GetTimeSeconds(),
		EffectiveDuration,
		Reason,
		&Transition))
	{
		return false;
	}

	if (AGobulinEnemyActor* Actor = Record->Actor.Get())
	{
		Actor->ApplyEnemyState(NewState);
	}

	if (UCombatEventSubsystem* Events = GetWorld()->GetSubsystem<UCombatEventSubsystem>())
	{
		FEnemyStateChangedEvent Event;
		Event.Enemy = Enemy;
		Event.Transition = MoveTemp(Transition);
		Events->EnqueueEnemyStateChanged(Event);
	}
	return true;
}

void UGobulinEnemySubsystem::UpdateEnemyBehavior(
	FCombatantHandle Enemy,
	float WorldTime,
	const TArray<FCombatantSnapshot>& Combatants)
{
	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	AGobulinEnemyActor* Actor = Record ? Record->Actor.Get() : nullptr;
	UCombatantRegistrySubsystem* Registry = GetWorld()
		? GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>()
		: nullptr;
	if (!Record || !Actor || !Registry || !Record->RuntimeData.IsAlive())
	{
		return;
	}

	FGobulinEnemyRuntimeData& RuntimeData = Record->RuntimeData;
	RuntimeData.NextBehaviorUpdateTime = WorldTime + RuntimeData.Stats.DecisionInterval;

	const FCombatantSnapshot* TargetSnapshot = nullptr;
	bool bAcquiredThisUpdate = false;
	if (RuntimeData.Target.IsSet())
	{
		for (const FCombatantSnapshot& Snapshot : Combatants)
		{
			if (Snapshot.Handle == RuntimeData.Target.Handle)
			{
				TargetSnapshot = &Snapshot;
				break;
			}
		}

		if (!TargetSnapshot
			|| !TargetSnapshot->bActive
			|| TargetSnapshot->TeamId == RuntimeData.TeamId)
		{
			ClearEnemyTarget(
				Enemy,
				EEnemyTargetChangeReason::TargetInactive,
				CombatTag_EnemyStateReason_TargetLost,
				WorldTime);
			return;
		}

		const double LoseRadiusSquared = FMath::Square(
			static_cast<double>(RuntimeData.Stats.TargetLoseRadius));
		if (FVector::DistSquared(Actor->GetActorLocation(), TargetSnapshot->Location) > LoseRadiusSquared)
		{
			ClearEnemyTarget(
				Enemy,
				EEnemyTargetChangeReason::OutOfRange,
				CombatTag_EnemyStateReason_TargetLost,
				WorldTime);
			return;
		}

		RuntimeData.Target.Refresh(TargetSnapshot->Location, WorldTime);
	}
	else
	{
		TargetSnapshot = FindBestTarget(RuntimeData, Actor->GetActorLocation(), Combatants);
		if (!TargetSnapshot)
		{
			return;
		}

		AssignEnemyTarget(*Record, *TargetSnapshot, WorldTime);
		bAcquiredThisUpdate = true;
	}

	const float HorizontalDistance = FVector::Dist2D(
		Actor->GetActorLocation(),
		TargetSnapshot->Location);
	const float ReadyThreshold = RuntimeData.State.CurrentState == EEnemyState::ReadyToAttack
		? RuntimeData.Stats.ResumeMoveDistance
		: RuntimeData.Stats.AttackReadyDistance;
	if (HorizontalDistance <= ReadyThreshold)
	{
		EnterAttackReady(Enemy, *Record, WorldTime);
		return;
	}

	if (RuntimeData.Movement.Status == EEnemyMoveStatus::Moving)
	{
		return;
	}

	AActor* TargetActor = Registry->ResolveActor(RuntimeData.Target.Handle);
	if (!TargetActor)
	{
		ClearEnemyTarget(
			Enemy,
			EEnemyTargetChangeReason::TargetInactive,
			CombatTag_EnemyStateReason_TargetLost,
			WorldTime);
		return;
	}

	FEnemyMoveIntent Intent;
	Intent.Target = RuntimeData.Target.Handle;
	Intent.Destination = TargetSnapshot->Location;
	Intent.DesiredSpeed = RuntimeData.Stats.MoveSpeed;
	Intent.AcceptanceRadius = RuntimeData.Stats.AttackReadyDistance;
	Intent.RequestedTime = WorldTime;
	Intent.IntentSequence = ++RuntimeData.Movement.LastIntentSequence;
	RuntimeData.Movement.Intent = Intent;

	const EEnemyMoveStatus SubmissionStatus = Actor->RequestMoveToTarget(TargetActor, Intent);
	SetEnemyMoveStatus(*Record, SubmissionStatus, WorldTime);
	if (SubmissionStatus == EEnemyMoveStatus::Moving)
	{
		if (RuntimeData.State.CurrentState != EEnemyState::Moving)
		{
			TransitionEnemy(
				Enemy,
				EEnemyState::Moving,
				0.0f,
				bAcquiredThisUpdate
					? CombatTag_EnemyStateReason_TargetAcquired
					: CombatTag_EnemyStateReason_PursuitResumed);
		}
	}
	else if (SubmissionStatus == EEnemyMoveStatus::Reached)
	{
		EnterAttackReady(Enemy, *Record, WorldTime);
	}
	else
	{
		ClearEnemyTarget(
			Enemy,
			EEnemyTargetChangeReason::NavigationFailed,
			CombatTag_EnemyStateReason_NavigationFailed,
			WorldTime);
	}
}

const FCombatantSnapshot* UGobulinEnemySubsystem::FindBestTarget(
	const FGobulinEnemyRuntimeData& RuntimeData,
	const FVector& EnemyLocation,
	const TArray<FCombatantSnapshot>& Combatants) const
{
	const double AcquisitionRadiusSquared = FMath::Square(
		static_cast<double>(RuntimeData.Stats.TargetAcquisitionRadius));
	const FCombatantSnapshot* BestTarget = nullptr;
	double BestDistanceSquared = TNumericLimits<double>::Max();

	for (const FCombatantSnapshot& Candidate : Combatants)
	{
		if (!Candidate.IsValid()
			|| !Candidate.bActive
			|| Candidate.Handle == RuntimeData.Handle
			|| Candidate.TeamId == RuntimeData.TeamId)
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared(EnemyLocation, Candidate.Location);
		if (DistanceSquared > AcquisitionRadiusSquared)
		{
			continue;
		}

		if (!BestTarget
			|| DistanceSquared < BestDistanceSquared
			|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
				&& IsHandleBefore(Candidate.Handle, BestTarget->Handle)))
		{
			BestTarget = &Candidate;
			BestDistanceSquared = DistanceSquared;
		}
	}

	return BestTarget;
}

void UGobulinEnemySubsystem::AssignEnemyTarget(
	FActorEnemyRecord& Record,
	const FCombatantSnapshot& Target,
	float WorldTime)
{
	const FCombatantHandle PreviousTarget = Record.RuntimeData.Target.Handle;
	Record.RuntimeData.Target.Assign(Target.Handle, Target.Location, WorldTime);
	PublishTargetChanged(
		Record,
		PreviousTarget,
		Target.Location,
		EEnemyTargetChangeReason::Acquired);
}

void UGobulinEnemySubsystem::ClearEnemyTarget(
	FCombatantHandle Enemy,
	EEnemyTargetChangeReason Reason,
	FGameplayTag StateReason,
	float WorldTime)
{
	FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	if (!Record || !Record->RuntimeData.Target.IsSet())
	{
		return;
	}

	const FCombatantHandle PreviousTarget = Record->RuntimeData.Target.Handle;
	const FVector LastKnownLocation = Record->RuntimeData.Target.LastKnownLocation;
	if (AGobulinEnemyActor* Actor = Record->Actor.Get())
	{
		Actor->StopEnemyMovement();
	}

	Record->RuntimeData.Target.Clear(WorldTime);
	if (Reason != EEnemyTargetChangeReason::NavigationFailed)
	{
		Record->RuntimeData.Movement.Status = EEnemyMoveStatus::Idle;
		Record->RuntimeData.Movement.StatusChangeTime = WorldTime;
	}

	PublishTargetChanged(*Record, PreviousTarget, LastKnownLocation, Reason);
	if (Record->RuntimeData.State.CurrentState == EEnemyState::Moving
		|| Record->RuntimeData.State.CurrentState == EEnemyState::ReadyToAttack)
	{
		TransitionEnemy(Enemy, EEnemyState::SeekingTarget, 0.0f, StateReason);
	}

	if (Reason == EEnemyTargetChangeReason::NavigationFailed)
	{
		Record->RuntimeData.NextBehaviorUpdateTime = WorldTime
			+ Record->RuntimeData.Stats.NavigationRetryDelay;
	}
}

void UGobulinEnemySubsystem::SetEnemyMoveStatus(
	FActorEnemyRecord& Record,
	EEnemyMoveStatus NewStatus,
	float WorldTime)
{
	if (Record.RuntimeData.Movement.Status == NewStatus)
	{
		return;
	}

	const EEnemyMoveStatus PreviousStatus = Record.RuntimeData.Movement.Status;
	Record.RuntimeData.Movement.Status = NewStatus;
	Record.RuntimeData.Movement.StatusChangeTime = WorldTime;
	PublishMoveStatusChanged(Record, PreviousStatus);
}

void UGobulinEnemySubsystem::EnterAttackReady(
	FCombatantHandle Enemy,
	FActorEnemyRecord& Record,
	float WorldTime)
{
	if (AGobulinEnemyActor* Actor = Record.Actor.Get())
	{
		Actor->StopEnemyMovement();
	}

	SetEnemyMoveStatus(Record, EEnemyMoveStatus::Reached, WorldTime);
	if (Record.RuntimeData.State.CurrentState != EEnemyState::ReadyToAttack)
	{
		TransitionEnemy(
			Enemy,
			EEnemyState::ReadyToAttack,
			0.0f,
			CombatTag_EnemyStateReason_AttackReady);
	}
}

void UGobulinEnemySubsystem::PublishTargetChanged(
	FActorEnemyRecord& Record,
	FCombatantHandle PreviousTarget,
	const FVector& LastKnownLocation,
	EEnemyTargetChangeReason Reason) const
{
	if (UCombatEventSubsystem* Events = GetWorld()
		? GetWorld()->GetSubsystem<UCombatEventSubsystem>()
		: nullptr)
	{
		FEnemyTargetChangedEvent Event;
		Event.Enemy = Record.RuntimeData.Handle;
		Event.PreviousTarget = PreviousTarget;
		Event.NewTarget = Record.RuntimeData.Target.Handle;
		Event.LastKnownLocation = LastKnownLocation;
		Event.Reason = Reason;
		Event.EventSequence = ++Record.RuntimeData.EventSequence;
		Events->EnqueueEnemyTargetChanged(Event);
	}
}

void UGobulinEnemySubsystem::PublishMoveStatusChanged(
	FActorEnemyRecord& Record,
	EEnemyMoveStatus PreviousStatus) const
{
	if (UCombatEventSubsystem* Events = GetWorld()
		? GetWorld()->GetSubsystem<UCombatEventSubsystem>()
		: nullptr)
	{
		FEnemyMoveStatusChangedEvent Event;
		Event.Enemy = Record.RuntimeData.Handle;
		Event.PreviousStatus = PreviousStatus;
		Event.NewStatus = Record.RuntimeData.Movement.Status;
		Event.Intent = Record.RuntimeData.Movement.Intent;
		Event.EventSequence = ++Record.RuntimeData.EventSequence;
		Events->EnqueueEnemyMoveStatusChanged(Event);
	}
}

bool UGobulinEnemySubsystem::RememberSpawnCommand(FCombatCommandId CommandId)
{
	if (RememberedSpawnCommands.Contains(CommandId))
	{
		return false;
	}

	RememberedSpawnCommands.Add(CommandId);
	SpawnCommandEvictionQueue.Enqueue(CommandId);
	while (RememberedSpawnCommands.Num() > MaxRememberedSpawnCommandCount)
	{
		FCombatCommandId ExpiredCommandId;
		if (!SpawnCommandEvictionQueue.Dequeue(ExpiredCommandId))
		{
			break;
		}
		RememberedSpawnCommands.Remove(ExpiredCommandId);
	}
	return true;
}

void UGobulinEnemySubsystem::PublishSpawnResolved(
	const FEnemySpawnRequest& Request,
	EEnemySpawnResult Result,
	FCombatantHandle Enemy) const
{
	if (UCombatEventSubsystem* Events = GetWorld()
		? GetWorld()->GetSubsystem<UCombatEventSubsystem>()
		: nullptr)
	{
		FEnemySpawnResolvedEvent Event;
		Event.Result.CommandId = Request.CommandId;
		Event.Result.Result = Result;
		Event.Result.Enemy = Enemy;
		Event.Result.EnemyDefinitionId = Request.EnemyDefinitionId;
		Events->EnqueueEnemySpawnResolved(Event);
	}
}

void UGobulinEnemySubsystem::PublishEnemySpawned(
	const FEnemySpawnRequest& Request,
	FCombatantHandle Enemy,
	const FTransform& ActualTransform) const
{
	if (UCombatEventSubsystem* Events = GetWorld()->GetSubsystem<UCombatEventSubsystem>())
	{
		FEnemySpawnedEvent Event;
		Event.CommandId = Request.CommandId;
		Event.Enemy = Enemy;
		Event.EnemyDefinitionId = Request.EnemyDefinitionId;
		Event.SpawnTransform = ActualTransform;
		Event.SpawnGroupId = Request.SpawnGroupId;
		Events->EnqueueEnemySpawned(Event);
	}
}

void UGobulinEnemySubsystem::PublishEnemyRetired(
	FActorEnemyRecord& Record,
	EEnemyRetireReason Reason,
	const FVector& LastLocation) const
{
	if (UCombatEventSubsystem* Events = GetWorld()
		? GetWorld()->GetSubsystem<UCombatEventSubsystem>()
		: nullptr)
	{
		FEnemyRetiredEvent Event;
		Event.Enemy = Record.RuntimeData.Handle;
		Event.Reason = Reason;
		Event.LastLocation = LastLocation;
		Event.EventSequence = ++Record.RuntimeData.EventSequence;
		Events->EnqueueEnemyRetired(Event);
	}
}
