#include "Enemy/GobulinEnemySubsystem.h"

#include "Combat/CombatantRegistrySubsystem.h"
#include "Combat/CombatantSnapshot.h"
#include "Combat/CombatEventSubsystem.h"
#include "Combat/CombatSubsystem.h"
#include "Core/CombatTags.h"
#include "Core/GobulinCollisionChannels.h"
#include "Enemy/GobulinEnemyActor.h"
#include "Enemy/GobulinEnemyAIController.h"
#include "Enemy/GobulinEnemyArchetype.h"
#include "Enemy/GobulinEnemyMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/AssetManager.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Subsystems/SubsystemCollection.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY_STATIC(LogGobulinEnemy, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarGobulinEnemyDebug(
		TEXT("gobulin.Enemy.Debug"),
		0,
		TEXT("Enemy debug visualization: 0=off, 1=all bodies plus stalled labels, 2=all labels."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarGobulinEnemyDebugHandle(
		TEXT("gobulin.Enemy.DebugHandle"),
		INDEX_NONE,
		TEXT("Combatant handle index to draw with full path, neighbor range and force vectors."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarGobulinEnemyDebugMaxLabels(
		TEXT("gobulin.Enemy.DebugMaxLabels"),
		64,
		TEXT("Maximum number of enemy world labels drawn each frame."),
		ECVF_Cheat);

	TAutoConsoleVariable<int32> CVarGobulinEnemyDebugDump(
		TEXT("gobulin.Enemy.DebugDump"),
		0,
		TEXT("Set to 1 to dump one snapshot of every active enemy to the Output Log."),
		ECVF_Cheat);

	FString GetEnemyStateDebugName(EEnemyState State)
	{
		const UEnum* Enum = StaticEnum<EEnemyState>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(State)) : TEXT("Unknown");
	}

	FString GetEnemyMoveDebugName(EEnemyMoveStatus Status)
	{
		const UEnum* Enum = StaticEnum<EEnemyMoveStatus>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Status)) : TEXT("Unknown");
	}

	FString GetTargetChangeReasonDebugName(EEnemyTargetChangeReason Reason)
	{
		const UEnum* Enum = StaticEnum<EEnemyTargetChangeReason>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Reason)) : TEXT("Unknown");
	}

	struct FEnemyCrowdParticle
	{
		FCombatantHandle Handle;
		TObjectPtr<AGobulinEnemyActor> Actor;
		FGobulinEnemyRuntimeData* RuntimeData = nullptr;
		FVector Location = FVector::ZeroVector;
		FVector DriveDirection = FVector::ZeroVector;
		float Radius = 1.0f;
		bool bCanReceiveForce = false;
	};

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

	float GetCrowdStableUnit(const FCombatantHandle& Handle)
	{
		const uint32 HandleHash = GetTypeHash(Handle);
		return static_cast<float>(HandleHash & 0xffffu) / 65535.0f;
	}

	float GetCrowdNavigationRetryDelay(const FGobulinEnemyRuntimeData& RuntimeData)
	{
		const float BaseInterval = FMath::Max(
			0.05f,
			RuntimeData.Stats.Crowd.FallbackNavigationRetryInterval);
		return BaseInterval * FMath::Lerp(
			0.75f,
			1.25f,
			GetCrowdStableUnit(RuntimeData.Handle));
	}

	bool AreUprightCapsulesWithinTolerance(
		const FVector& LeftLocation,
		const FCombatantBodyShape& LeftBody,
		const FVector& RightLocation,
		const FCombatantBodyShape& RightBody,
		float Tolerance)
	{
		if (!LeftBody.IsValid()
			|| !RightBody.IsValid()
			|| !FMath::IsFinite(Tolerance)
			|| Tolerance < 0.0f)
		{
			return false;
		}

		const double HorizontalDistanceSquared = FVector::DistSquaredXY(LeftLocation, RightLocation);
		const double LeftSegmentHalfLength = LeftBody.CapsuleHalfHeight - LeftBody.CapsuleRadius;
		const double RightSegmentHalfLength = RightBody.CapsuleHalfHeight - RightBody.CapsuleRadius;
		const double VerticalAxisGap = FMath::Max(
			0.0,
			FMath::Abs(static_cast<double>(LeftLocation.Z - RightLocation.Z))
				- LeftSegmentHalfLength
				- RightSegmentHalfLength);
		const double ContactDistance = LeftBody.CapsuleRadius
			+ RightBody.CapsuleRadius
			+ static_cast<double>(Tolerance);
		return HorizontalDistanceSquared + FMath::Square(VerticalAxisGap)
			<= FMath::Square(ContactDistance);
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
	TArray<FCombatantHandle> ReactionCompletions;
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

		if (RuntimeData.State.CurrentState == EEnemyState::Dying)
		{
			if (Pair.Value.Actor->IsDeathPresentationComplete()
				|| (RuntimeData.State.StateEndTime > 0.0f
					&& WorldTime >= RuntimeData.State.StateEndTime))
			{
				DeathCompletions.Add(Pair.Key);
			}
			continue;
		}

		if (RuntimeData.State.CurrentState == EEnemyState::Spawning
			&& RuntimeData.State.StateEndTime > 0.0f
			&& WorldTime >= RuntimeData.State.StateEndTime)
		{
			SpawnCompletions.Add(Pair.Key);
		}
		else if ((RuntimeData.State.CurrentState == EEnemyState::HitReacting
				|| RuntimeData.State.CurrentState == EEnemyState::Staggered)
			&& RuntimeData.State.StateEndTime > 0.0f
			&& WorldTime >= RuntimeData.State.StateEndTime
			&& (Pair.Value.Actor->IsEnemyGrounded()
				|| WorldTime >= RuntimeData.State.StateStartTime
					+ RuntimeData.Stats.Reaction.MaximumAirborneReactionDuration))
		{
			ReactionCompletions.Add(Pair.Key);
		}
		else if (RuntimeData.IsAlive()
			&& IsBehaviorState(RuntimeData.State.CurrentState)
			&& WorldTime >= RuntimeData.NextBehaviorUpdateTime)
		{
			BehaviorUpdates.Add(Pair.Key);
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

	for (FCombatantHandle Enemy : ReactionCompletions)
	{
		if (FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy))
		{
			Record->RuntimeData.Movement.Status = EEnemyMoveStatus::Idle;
			Record->RuntimeData.NextBehaviorUpdateTime = WorldTime;
		}
		TransitionEnemy(
			Enemy,
			EEnemyState::SeekingTarget,
			0.0f,
			CombatTag_EnemyStateReason_Recovered);
	}

	for (FCombatantHandle Enemy : MissingActors)
	{
		RetireEnemy(Enemy, EEnemyRetireReason::ExternalRemoval);
	}

	UpdateEnemyCrowd(DeltaTime);

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

	UpdateEnemyDebugTracking(WorldTime);
	if (CVarGobulinEnemyDebugDump.GetValueOnGameThread() != 0)
	{
		DumpEnemyDebug();
		// 使用与控制台输入相同的优先级，否则 SetByConsole 会拒绝较低优先级的自动复位。
		CVarGobulinEnemyDebugDump.AsVariable()->Set(0, ECVF_SetByConsole);
	}
	DrawEnemyDebug();
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

bool UGobulinEnemySubsystem::GetEnemyDebugSnapshot(
	FCombatantHandle Enemy,
	FGobulinEnemyDebugSnapshot& OutSnapshot) const
{
	const FActorEnemyRecord* Record = ActiveEnemies.Find(Enemy);
	const AGobulinEnemyActor* Actor = Record ? Record->Actor.Get() : nullptr;
	if (!Record || !Actor)
	{
		return false;
	}

	const FGobulinEnemyRuntimeData& RuntimeData = Record->RuntimeData;
	const FEnemyDebugRecord& Debug = Record->Debug;
	const UWorld* World = GetWorld();
	const float WorldTime = World ? World->GetTimeSeconds() : 0.0f;

	OutSnapshot = FGobulinEnemyDebugSnapshot();
	OutSnapshot.Handle = Enemy;
	OutSnapshot.ActorName = Actor->GetName();
	OutSnapshot.EnemyState = GetEnemyStateDebugName(RuntimeData.State.CurrentState);
	OutSnapshot.MoveStatus = GetEnemyMoveDebugName(RuntimeData.Movement.Status);
	OutSnapshot.ActorLocation = Actor->GetActorLocation();
	OutSnapshot.ActualVelocity = Actor->GetVelocity();
	OutSnapshot.TargetLocation = RuntimeData.Target.LastKnownLocation;
	OutSnapshot.IntentDestination = RuntimeData.Movement.Intent.Destination;
	OutSnapshot.DriveVelocityChange = Debug.DriveVelocityChange;
	OutSnapshot.SeparationVelocityChange = Debug.SeparationVelocityChange;
	OutSnapshot.LiftVelocityChange = Debug.LiftVelocityChange;
	OutSnapshot.LocalNeighborCount = RuntimeData.Crowd.LocalNeighborCount;
	OutSnapshot.LocalPressure = RuntimeData.Crowd.LocalPressure;
	OutSnapshot.CombatCapsuleRadius = RuntimeData.BodyShape.CapsuleRadius;
	OutSnapshot.CombatCapsuleHalfHeight = RuntimeData.BodyShape.CapsuleHalfHeight;
	OutSnapshot.CrowdRadius = RuntimeData.BodyShape.CapsuleRadius
		* RuntimeData.Stats.Crowd.ParticleRadiusScale;
	OutSnapshot.NeighborRange = OutSnapshot.CrowdRadius
		* 2.0f
		* RuntimeData.Stats.Crowd.NeighborRangeScale;
	OutSnapshot.CrowdNavigationRetryRemaining = FMath::Max(
		0.0f,
		RuntimeData.Crowd.NextNavigationRetryTime - WorldTime);
	OutSnapshot.bNavigationPathRejected = RuntimeData.Crowd.bNavigationPathRejected;
	if (OutSnapshot.bNavigationPathRejected)
	{
		OutSnapshot.NavigationRecoveryHeightDelta = FMath::Abs(
			OutSnapshot.ActorLocation.Z
				- RuntimeData.Crowd.NavigationRejectionAgentLocation.Z);
		OutSnapshot.NavigationRecoveryTargetMovement = FVector::Dist(
			OutSnapshot.TargetLocation,
			RuntimeData.Crowd.NavigationRejectionTargetLocation);
		OutSnapshot.NavigationRecoveryTargetProgress =
			RuntimeData.Crowd.NavigationRejectionTargetDistance
			- FVector::Dist(OutSnapshot.ActorLocation, OutSnapshot.TargetLocation);
	}
	OutSnapshot.StateAge = FMath::Max(0.0f, WorldTime - RuntimeData.State.StateStartTime);
	OutSnapshot.MoveStatusAge = FMath::Max(
		0.0f,
		WorldTime - RuntimeData.Movement.StatusChangeTime);
	OutSnapshot.DistanceMovedLastSample = Debug.DistanceMovedLastSample;
	OutSnapshot.TargetDistanceDeltaLastSample = Debug.TargetDistanceDeltaLastSample;
	OutSnapshot.NoTargetProgressTime = Debug.NoTargetProgressTime;
	OutSnapshot.LastMovementEvent = Debug.LastMovementEvent;
	OutSnapshot.bHasTarget = RuntimeData.Target.IsSet();
	OutSnapshot.bInContact = RuntimeData.ContactDamage.bInContact;
	OutSnapshot.bTargetProgressStalled = Debug.bTargetProgressStalled;

	if (OutSnapshot.bHasTarget && RuntimeData.Movement.Intent.Target.IsSet())
	{
		OutSnapshot.TargetToIntentDistance = FVector::Dist(
			OutSnapshot.TargetLocation,
			OutSnapshot.IntentDestination);
	}

	if (const UCharacterMovementComponent* Movement = Actor->GetCharacterMovement())
	{
		if (const UEnum* Enum = StaticEnum<EMovementMode>())
		{
			OutSnapshot.MovementMode = Enum->GetNameStringByValue(
				static_cast<int64>(Movement->MovementMode));
		}
		OutSnapshot.bGrounded = Movement->IsMovingOnGround();
		OutSnapshot.bCanWalkOffLedges = Movement->CanWalkOffLedges();
		if (const UGobulinEnemyMovementComponent* EnemyMovement =
			Cast<UGobulinEnemyMovementComponent>(Movement))
		{
			OutSnapshot.bHasGroundSupportSample = EnemyMovement->HasGroundSupportSample();
			OutSnapshot.bHasCenterGroundSupport = EnemyMovement->HasCenterGroundSupport();
			OutSnapshot.GroundSupportRadius = EnemyMovement->GetGroundSupportRadius();
			OutSnapshot.GroundSnapDownHeight = EnemyMovement->GetGroundSnapDownHeight();
			OutSnapshot.GroundSupportLocation = EnemyMovement->GetGroundSupportLocation();
		}
	}

	if (const AGobulinEnemyAIController* Controller =
		Cast<AGobulinEnemyAIController>(Actor->GetController()))
	{
		Controller->GetEnemyNavigationDebugData(
			OutSnapshot.PathStatus,
			OutSnapshot.bPathIntentActive,
			OutSnapshot.PathPoints);
	}
	else
	{
		OutSnapshot.PathStatus = TEXT("NoController");
	}

	return true;
}

void UGobulinEnemySubsystem::GetAllEnemyDebugSnapshots(
	TArray<FGobulinEnemyDebugSnapshot>& OutSnapshots) const
{
	OutSnapshots.Reset();
	TArray<FCombatantHandle> Handles;
	ActiveEnemies.GetKeys(Handles);
	Handles.Sort(IsHandleBefore);
	OutSnapshots.Reserve(Handles.Num());

	for (const FCombatantHandle Handle : Handles)
	{
		FGobulinEnemyDebugSnapshot Snapshot;
		if (GetEnemyDebugSnapshot(Handle, Snapshot))
		{
			OutSnapshots.Add(MoveTemp(Snapshot));
		}
	}
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
	Result.bKilled = RuntimeData.CurrentHealth <= 0.0f;
	const float WorldTime = GetWorld()->GetTimeSeconds();
	const FGobulinEnemyReactionDefinition& Reaction = RuntimeData.Stats.Reaction;
	const FVector ImpactVelocity = (Request.Impulse * Reaction.KnockbackVelocityScale)
		.GetClampedToMaxSize(Reaction.MaximumLaunchVelocity);

	if (!Result.bKilled)
	{
		SetEnemyDebugMovementEvent(*Record, TEXT("DamageInterrupt"));
		LeaveContact(*Record);
		SetEnemyMoveStatus(*Record, EEnemyMoveStatus::Idle, WorldTime);
		Record->Actor->ApplyEnemyImpact(ImpactVelocity, false);

		const bool bMeetsDamageThreshold = Reaction.StaggerDamageRatioThreshold > 0.0f
			&& AppliedAmount / RuntimeData.Stats.MaxHealth >= Reaction.StaggerDamageRatioThreshold;
		const bool bHeavyAttack = Reaction.HeavyAttackTag.IsValid()
			&& Request.AttackTag.MatchesTag(Reaction.HeavyAttackTag);
		const bool bCanEnterStagger = WorldTime >= RuntimeData.Reaction.StaggerImmunityEndTime;
		if ((bMeetsDamageThreshold || bHeavyAttack) && bCanEnterStagger)
		{
			Result.ReactionTag = CombatTag_Reaction_Stagger;
			if (TransitionEnemy(
				Enemy,
				EEnemyState::Staggered,
				Reaction.StaggerDuration,
				CombatTag_EnemyStateReason_Damaged))
			{
				RuntimeData.Reaction.StaggerImmunityEndTime = WorldTime
					+ Reaction.StaggerDuration
					+ Reaction.StaggerImmunityDuration;
			}
		}
		else
		{
			Result.ReactionTag = ImpactVelocity.IsNearlyZero()
				? CombatTag_Reaction_Hit
				: CombatTag_Reaction_Knockback;
			if (RuntimeData.State.CurrentState != EEnemyState::Staggered)
			{
				TransitionEnemy(
					Enemy,
					EEnemyState::HitReacting,
					Reaction.LightInterruptDuration,
					CombatTag_EnemyStateReason_Damaged);
			}
		}
		return Result;
	}

	UCombatantRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UCombatantRegistrySubsystem>();
	FVector LethalLaunchVelocity = ImpactVelocity;
	FVector HorizontalVelocity(LethalLaunchVelocity.X, LethalLaunchVelocity.Y, 0.0f);
	if (HorizontalVelocity.IsNearlyZero() && Registry)
	{
		if (const AActor* SourceActor = Registry->ResolveActor(Request.Source))
		{
			HorizontalVelocity = (Record->Actor->GetActorLocation() - SourceActor->GetActorLocation()).GetSafeNormal2D()
				* Reaction.LethalFallbackHorizontalVelocity;
		}
	}
	if (HorizontalVelocity.IsNearlyZero())
	{
		HorizontalVelocity = -Request.HitNormal.GetSafeNormal2D()
			* Reaction.LethalFallbackHorizontalVelocity;
	}
	HorizontalVelocity *= Reaction.LethalHorizontalVelocityScale;
	const float LethalVerticalVelocity = FMath::Min(
		FMath::Max(LethalLaunchVelocity.Z, Reaction.LethalMinimumVerticalVelocity),
		Reaction.MaximumLaunchVelocity);
	const float MaximumHorizontalVelocity = FMath::Sqrt(FMath::Max(
		0.0f,
		FMath::Square(Reaction.MaximumLaunchVelocity)
			- FMath::Square(LethalVerticalVelocity)));
	HorizontalVelocity = HorizontalVelocity.GetClampedToMaxSize(MaximumHorizontalVelocity);
	LethalLaunchVelocity = FVector(
		HorizontalVelocity.X,
		HorizontalVelocity.Y,
		LethalVerticalVelocity);
	Result.ReactionTag = CombatTag_Reaction_Knockback;

	if (Registry)
	{
		Registry->SetCombatantActive(Enemy, false);
	}
	SetEnemyDebugMovementEvent(*Record, TEXT("LethalLaunch"));
	Record->RuntimeData.Target.Clear(WorldTime);
	LeaveContact(*Record);
	Record->RuntimeData.Crowd.NextNavigationRetryTime = 0.0f;
	SetEnemyMoveStatus(*Record, EEnemyMoveStatus::Idle, WorldTime);
	TransitionEnemy(
		Enemy,
		EEnemyState::Dying,
		Reaction.DeathMaximumDuration,
		CombatTag_EnemyStateReason_Killed);
	Record->Actor->BeginDeathPhysics(LethalLaunchVelocity);

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
	SetEnemyDebugMovementEvent(
		*Record,
		FString::Printf(TEXT("PathCompleted:%s"), *GetEnemyMoveDebugName(Status)));
	if (Status == EEnemyMoveStatus::Blocked
		&& Record->RuntimeData.Stats.Crowd.bEnableContinuousPiling)
	{
		RejectNavigationPath(*Record, WorldTime, TEXT("NavigationBlockedRejected"));
	}
	if ((Status == EEnemyMoveStatus::Blocked || Status == EEnemyMoveStatus::Failed)
		&& TryEnterCrowdPushing(*Record, WorldTime))
	{
		return;
	}

	SetEnemyMoveStatus(*Record, Status, WorldTime);
	if (Status == EEnemyMoveStatus::Reached || Status == EEnemyMoveStatus::Blocked)
	{
		// Reached 仍需用双方逻辑胶囊确认接触；Blocked 也可能只是撞到了目标胶囊。
		Record->RuntimeData.NextBehaviorUpdateTime = WorldTime;
	}
	else if (Status == EEnemyMoveStatus::Failed)
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

	FCombatantBodyShape BodyShape;
	BodyShape.CapsuleRadius = Archetype.Body.CapsuleRadius * RadialScale;
	BodyShape.CapsuleHalfHeight = Archetype.Body.CapsuleHalfHeight * VerticalScale;
	const FCombatantHandle Enemy = Registry->RegisterActor(Actor, Request.TeamId, BodyShape);
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
	Record.RuntimeData.BodyShape = BodyShape;
	Record.RuntimeData.Crowd.ReferenceCenterZ = Actor->GetActorLocation().Z;
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

	const bool bTimedState = NewState == EEnemyState::Spawning
		|| NewState == EEnemyState::HitReacting
		|| NewState == EEnemyState::Staggered
		|| NewState == EEnemyState::Dying;
	const float EffectiveDuration = bTimedState ? FMath::Max(Duration, KINDA_SMALL_NUMBER) : 0.0f;
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
		UE_VLOG(
			Actor,
			LogGobulinEnemy,
			Log,
			TEXT("State %s -> %s reason=%s duration=%.2f"),
			*GetEnemyStateDebugName(Transition.PreviousState),
			*GetEnemyStateDebugName(Transition.NewState),
			*Transition.Reason.ToString(),
			EffectiveDuration);
	}
	SetEnemyDebugMovementEvent(
		*Record,
		FString::Printf(
			TEXT("State:%s->%s"),
			*GetEnemyStateDebugName(Transition.PreviousState),
			*GetEnemyStateDebugName(Transition.NewState)));

	if (UCombatEventSubsystem* Events = GetWorld()->GetSubsystem<UCombatEventSubsystem>())
	{
		FEnemyStateChangedEvent Event;
		Event.Enemy = Enemy;
		Event.Transition = MoveTemp(Transition);
		Events->EnqueueEnemyStateChanged(Event);
	}
	return true;
}

void UGobulinEnemySubsystem::UpdateEnemyCrowd(float DeltaTime)
{
	if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
	{
		return;
	}

	// 限制单次求解步长，避免 PIE 卡顿或断点恢复时产生异常大的速度修正。
	const float StepTime = FMath::Min(DeltaTime, 1.0f / 20.0f);
	TArray<FEnemyCrowdParticle> Particles;
	Particles.Reserve(ActiveEnemies.Num());
	float CellSize = 1.0f;

	for (TPair<FCombatantHandle, FActorEnemyRecord>& Pair : ActiveEnemies)
	{
		FGobulinEnemyRuntimeData& RuntimeData = Pair.Value.RuntimeData;
		RuntimeData.Crowd.LocalNeighborCount = 0;
		RuntimeData.Crowd.LocalPressure = 0.0f;
		Pair.Value.Debug.DriveVelocityChange = FVector::ZeroVector;
		Pair.Value.Debug.SeparationVelocityChange = FVector::ZeroVector;
		Pair.Value.Debug.LiftVelocityChange = FVector::ZeroVector;

		AGobulinEnemyActor* Actor = Pair.Value.Actor.Get();
		if (!Actor
			|| !RuntimeData.IsAlive()
			|| !RuntimeData.Stats.Crowd.bEnableContinuousPiling)
		{
			continue;
		}

		FEnemyCrowdParticle& Particle = Particles.AddDefaulted_GetRef();
		Particle.Handle = Pair.Key;
		Particle.Actor = Actor;
		Particle.RuntimeData = &RuntimeData;
		Particle.Location = Actor->GetActorLocation();
		Particle.Radius = FMath::Max(
			1.0f,
			RuntimeData.BodyShape.CapsuleRadius
				* RuntimeData.Stats.Crowd.ParticleRadiusScale);
		Particle.DriveDirection = RuntimeData.Target.IsSet()
			? (RuntimeData.Target.LastKnownLocation - Particle.Location).GetSafeNormal2D()
			: FVector::ZeroVector;
		Particle.bCanReceiveForce = IsBehaviorState(RuntimeData.State.CurrentState);
		CellSize = FMath::Max(
			CellSize,
			Particle.Radius * 2.0f * RuntimeData.Stats.Crowd.NeighborRangeScale);
	}

	if (Particles.IsEmpty())
	{
		return;
	}

	auto GetCell = [CellSize](const FVector& Location)
	{
		return FIntVector(
			FMath::FloorToInt(Location.X / CellSize),
			FMath::FloorToInt(Location.Y / CellSize),
			FMath::FloorToInt(Location.Z / CellSize));
	};

	TMap<FIntVector, TArray<int32>> Grid;
	Grid.Reserve(Particles.Num());
	for (int32 Index = 0; Index < Particles.Num(); ++Index)
	{
		Grid.FindOrAdd(GetCell(Particles[Index].Location)).Add(Index);
	}

	TArray<FIntPoint> NeighborPairs;
	TArray<int32> NeighborCounts;
	NeighborCounts.SetNumZeroed(Particles.Num());
	for (int32 LeftIndex = 0; LeftIndex < Particles.Num(); ++LeftIndex)
	{
		const FIntVector CenterCell = GetCell(Particles[LeftIndex].Location);
		for (int32 Z = -1; Z <= 1; ++Z)
		{
			for (int32 Y = -1; Y <= 1; ++Y)
			{
				for (int32 X = -1; X <= 1; ++X)
				{
					const TArray<int32>* Cell = Grid.Find(CenterCell + FIntVector(X, Y, Z));
					if (!Cell)
					{
						continue;
					}

					for (const int32 RightIndex : *Cell)
					{
						if (RightIndex <= LeftIndex)
						{
							continue;
						}

						const FEnemyCrowdParticle& Left = Particles[LeftIndex];
						const FEnemyCrowdParticle& Right = Particles[RightIndex];
						const float NeighborRange = (Left.Radius + Right.Radius)
							* FMath::Max(
								Left.RuntimeData->Stats.Crowd.NeighborRangeScale,
								Right.RuntimeData->Stats.Crowd.NeighborRangeScale);
						if (FVector::DistSquared(Left.Location, Right.Location)
							> FMath::Square(NeighborRange))
						{
							continue;
						}

						NeighborPairs.Emplace(LeftIndex, RightIndex);
						++NeighborCounts[LeftIndex];
						++NeighborCounts[RightIndex];
					}
				}
			}
		}
	}

	for (int32 Index = 0; Index < Particles.Num(); ++Index)
	{
		FGobulinEnemyRuntimeData& RuntimeData = *Particles[Index].RuntimeData;
		RuntimeData.Crowd.LocalNeighborCount = NeighborCounts[Index];
		RuntimeData.Crowd.LocalPressure = static_cast<float>(NeighborCounts[Index])
			/ static_cast<float>(FMath::Max(
				1,
				RuntimeData.Stats.Crowd.MinimumLiftNeighborCount));

		if (RuntimeData.Movement.Status == EEnemyMoveStatus::CrowdPushing)
		{
			const FVector AppliedDrive = Particles[Index].Actor->ApplyCrowdFallbackDrive(
				Particles[Index].DriveDirection,
				RuntimeData.Stats.MoveSpeed,
				RuntimeData.Stats.Crowd,
				StepTime);
			if (FActorEnemyRecord* Record = ActiveEnemies.Find(Particles[Index].Handle))
			{
				Record->Debug.DriveVelocityChange = AppliedDrive;
			}
		}
	}

	TArray<FVector> VelocityChanges;
	VelocityChanges.SetNumZeroed(Particles.Num());
	TArray<bool> LiftApplied;
	LiftApplied.Init(false, Particles.Num());

	auto TryAccumulateLift = [&Particles, &NeighborCounts, &VelocityChanges, &LiftApplied, StepTime](
		int32 RearIndex,
		int32 FrontIndex,
		float PenetrationRatio)
	{
		const FEnemyCrowdParticle& Rear = Particles[RearIndex];
		const FEnemyCrowdParticle& Front = Particles[FrontIndex];
		const FGobulinEnemyCrowdDefinition& Definition = Rear.RuntimeData->Stats.Crowd;
		if (!Rear.bCanReceiveForce
			|| Rear.DriveDirection.IsNearlyZero()
			|| NeighborCounts[RearIndex] < Definition.MinimumLiftNeighborCount
			|| Rear.Location.Z >= Rear.RuntimeData->Crowd.ReferenceCenterZ + Definition.MaximumPileHeight)
		{
			return;
		}

		const FVector ToFront = (Front.Location - Rear.Location).GetSafeNormal2D();
		if (FVector::DotProduct(ToFront, Rear.DriveDirection) < 0.35f)
		{
			return;
		}

		const float CombinedRadius = Rear.Radius + Front.Radius;
		if (Rear.Location.Z - Front.Location.Z > CombinedRadius * 0.75f)
		{
			return;
		}

		const float DensityScale = FMath::Clamp(
			Rear.RuntimeData->Crowd.LocalPressure,
			1.0f,
			2.5f);
		VelocityChanges[RearIndex].Z += Definition.UpwardPressureAcceleration
			* DensityScale
			* FMath::Max(0.15f, PenetrationRatio)
			* StepTime;
		LiftApplied[RearIndex] = true;
	};

	for (const FIntPoint& Pair : NeighborPairs)
	{
		const int32 LeftIndex = Pair.X;
		const int32 RightIndex = Pair.Y;
		const FEnemyCrowdParticle& Left = Particles[LeftIndex];
		const FEnemyCrowdParticle& Right = Particles[RightIndex];
		const FVector Delta = Left.Location - Right.Location;
		const float CombinedRadius = Left.Radius + Right.Radius;
		const float Distance = Delta.Size();
		if (Distance >= CombinedRadius)
		{
			continue;
		}

		const float PenetrationRatio = FMath::Clamp(
			(CombinedRadius - Distance) / CombinedRadius,
			0.0f,
			1.0f);
		FVector HorizontalNormal = Delta.GetSafeNormal2D();
		if (HorizontalNormal.IsNearlyZero())
		{
			const float Angle = FMath::Fmod(
				static_cast<float>(Left.Handle.GetIndex()) * 2.39996323f,
				2.0f * PI);
			HorizontalNormal = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		}

		const float SeparationAcceleration = 0.5f * (
			Left.RuntimeData->Stats.Crowd.HorizontalSeparationAcceleration
			+ Right.RuntimeData->Stats.Crowd.HorizontalSeparationAcceleration);
		const FVector SeparationVelocityChange = HorizontalNormal
			* SeparationAcceleration
			* PenetrationRatio
			* StepTime;
		const int32 ReceiverCount = static_cast<int32>(Left.bCanReceiveForce)
			+ static_cast<int32>(Right.bCanReceiveForce);
		if (ReceiverCount > 0)
		{
			if (Left.bCanReceiveForce)
			{
				VelocityChanges[LeftIndex] += SeparationVelocityChange
					/ static_cast<float>(ReceiverCount);
			}
			if (Right.bCanReceiveForce)
			{
				VelocityChanges[RightIndex] -= SeparationVelocityChange
					/ static_cast<float>(ReceiverCount);
			}
		}

		TryAccumulateLift(LeftIndex, RightIndex, PenetrationRatio);
		TryAccumulateLift(RightIndex, LeftIndex, PenetrationRatio);
	}

	for (int32 Index = 0; Index < Particles.Num(); ++Index)
	{
		if (!Particles[Index].bCanReceiveForce)
		{
			continue;
		}

		const FGobulinEnemyCrowdDefinition& Definition = Particles[Index].RuntimeData->Stats.Crowd;
		FVector VelocityChange = VelocityChanges[Index];
		FVector HorizontalChange(VelocityChange.X, VelocityChange.Y, 0.0f);
		const float MaximumHorizontalChange = Definition.HorizontalSeparationAcceleration
			* StepTime
			* 1.5f;
		HorizontalChange = HorizontalChange.GetClampedToMaxSize(MaximumHorizontalChange);
		if (LiftApplied[Index])
		{
			// 抬升时保留少量横向分离，让后排能沿前排身体向上滑动，而不是被完全弹回。
			HorizontalChange *= 0.35f;
		}
		VelocityChange.X = HorizontalChange.X;
		VelocityChange.Y = HorizontalChange.Y;
		VelocityChange.Z = FMath::Min(
			VelocityChange.Z,
			Definition.UpwardPressureAcceleration * StepTime * 1.5f);
		const FVector AppliedChange = Particles[Index].Actor->ApplyCrowdVelocityChange(
			VelocityChange,
			Definition.MaximumLiftSpeed,
			StepTime);
		if (FActorEnemyRecord* Record = ActiveEnemies.Find(Particles[Index].Handle))
		{
			Record->Debug.SeparationVelocityChange = FVector(
				AppliedChange.X,
				AppliedChange.Y,
				0.0f);
			Record->Debug.LiftVelocityChange = FVector(0.0f, 0.0f, AppliedChange.Z);
		}
	}
}

bool UGobulinEnemySubsystem::TryEnterCrowdPushing(
	FActorEnemyRecord& Record,
	float WorldTime)
{
	if (!Record.RuntimeData.IsAlive()
		|| !Record.RuntimeData.Target.IsSet()
		|| !Record.RuntimeData.Stats.Crowd.bEnableContinuousPiling)
	{
		return false;
	}

	const bool bEnteringCrowdPushing = Record.RuntimeData.Movement.Status
		!= EEnemyMoveStatus::CrowdPushing;
	if (bEnteringCrowdPushing)
	{
		if (AGobulinEnemyActor* Actor = Record.Actor.Get())
		{
			Actor->StopEnemyMovement();
		}
		Record.RuntimeData.Crowd.NextNavigationRetryTime = WorldTime
			+ GetCrowdNavigationRetryDelay(Record.RuntimeData);
	}
	SetEnemyMoveStatus(Record, EEnemyMoveStatus::CrowdPushing, WorldTime);
	Record.RuntimeData.NextBehaviorUpdateTime = WorldTime
		+ Record.RuntimeData.Stats.DecisionInterval
			* GetCrowdStableUnit(Record.RuntimeData.Handle);
	if (Record.RuntimeData.State.CurrentState != EEnemyState::Moving)
	{
		TransitionEnemy(
			Record.RuntimeData.Handle,
			EEnemyState::Moving,
			0.0f,
			CombatTag_EnemyStateReason_PursuitResumed);
	}
	return true;
}

void UGobulinEnemySubsystem::ResetNavigationProgress(
	FActorEnemyRecord& Record,
	float WorldTime)
{
	FGobulinEnemyCrowdData& Crowd = Record.RuntimeData.Crowd;
	const AGobulinEnemyActor* Actor = Record.Actor.Get();
	if (!Actor || !Record.RuntimeData.Target.IsSet())
	{
		Crowd.NavigationProgressReferenceDistance = 0.0f;
		Crowd.NavigationProgressTargetLocation = FVector::ZeroVector;
		Crowd.NavigationLastProgressTime = WorldTime;
		Crowd.bNavigationProgressInitialized = false;
		return;
	}

	Crowd.NavigationProgressReferenceDistance = FVector::Dist(
		Actor->GetActorLocation(),
		Record.RuntimeData.Target.LastKnownLocation);
	Crowd.NavigationProgressTargetLocation = Record.RuntimeData.Target.LastKnownLocation;
	Crowd.NavigationLastProgressTime = WorldTime;
	Crowd.bNavigationProgressInitialized = true;
}

void UGobulinEnemySubsystem::ClearNavigationExecutionTracking(FActorEnemyRecord& Record)
{
	FGobulinEnemyCrowdData& Crowd = Record.RuntimeData.Crowd;
	Crowd.NextNavigationRetryTime = 0.0f;
	Crowd.NavigationProgressReferenceDistance = 0.0f;
	Crowd.NavigationProgressTargetLocation = FVector::ZeroVector;
	Crowd.NavigationLastProgressTime = 0.0f;
	Crowd.bNavigationProgressInitialized = false;
	Crowd.bNavigationPathRejected = false;
	Crowd.NavigationRejectionAgentLocation = FVector::ZeroVector;
	Crowd.NavigationRejectionTargetLocation = FVector::ZeroVector;
	Crowd.NavigationRejectionTargetDistance = 0.0f;
}

void UGobulinEnemySubsystem::RejectNavigationPath(
	FActorEnemyRecord& Record,
	float WorldTime,
	const TCHAR* DebugReason)
{
	AGobulinEnemyActor* Actor = Record.Actor.Get();
	if (!Actor || !Record.RuntimeData.Target.IsSet())
	{
		return;
	}

	FGobulinEnemyCrowdData& Crowd = Record.RuntimeData.Crowd;
	Crowd.bNavigationPathRejected = true;
	Crowd.NavigationRejectionAgentLocation = Actor->GetActorLocation();
	Crowd.NavigationRejectionTargetLocation = Record.RuntimeData.Target.LastKnownLocation;
	Crowd.NavigationRejectionTargetDistance = FVector::Dist(
		Crowd.NavigationRejectionAgentLocation,
		Crowd.NavigationRejectionTargetLocation);
	Crowd.bNavigationProgressInitialized = false;
	Crowd.NavigationLastProgressTime = WorldTime;
	SetEnemyDebugMovementEvent(Record, DebugReason);
	UE_VLOG(
		Actor,
		LogGobulinEnemy,
		Warning,
		TEXT("Navigation path rejected by execution watchdog: reason=%s location=%s target=%s distance=%.1f"),
		DebugReason,
		*Crowd.NavigationRejectionAgentLocation.ToCompactString(),
		*Crowd.NavigationRejectionTargetLocation.ToCompactString(),
		Crowd.NavigationRejectionTargetDistance);
}

bool UGobulinEnemySubsystem::UpdateNavigationExecutionWatchdog(
	FActorEnemyRecord& Record,
	float WorldTime)
{
	FGobulinEnemyRuntimeData& RuntimeData = Record.RuntimeData;
	AGobulinEnemyActor* Actor = Record.Actor.Get();
	if (!Actor
		|| !RuntimeData.IsAlive()
		|| !RuntimeData.Target.IsSet()
		|| RuntimeData.Movement.Status != EEnemyMoveStatus::Moving
		|| !RuntimeData.Stats.Crowd.bEnableContinuousPiling
		|| !Actor->IsEnemyGrounded())
	{
		return false;
	}

	FGobulinEnemyCrowdData& Crowd = RuntimeData.Crowd;
	const FGobulinEnemyCrowdDefinition& Definition = RuntimeData.Stats.Crowd;
	const FVector ActorLocation = Actor->GetActorLocation();
	const FVector TargetLocation = RuntimeData.Target.LastKnownLocation;
	const float TargetDistance = FVector::Dist(ActorLocation, TargetLocation);
	if (!Crowd.bNavigationProgressInitialized)
	{
		ResetNavigationProgress(Record, WorldTime);
		return false;
	}

	if (FVector::Dist(TargetLocation, Crowd.NavigationProgressTargetLocation)
		>= Definition.NavigationRecoveryDistance)
	{
		ResetNavigationProgress(Record, WorldTime);
		return false;
	}

	const float RequiredProgress = FMath::Max(
		KINDA_SMALL_NUMBER,
		Definition.NavigationMinimumTargetProgress);
	if (Crowd.NavigationProgressReferenceDistance - TargetDistance >= RequiredProgress)
	{
		ResetNavigationProgress(Record, WorldTime);
		return false;
	}

	if (Actor->GetVelocity().Size2D() >= Definition.NavigationStallSpeedThreshold
		|| WorldTime - Crowd.NavigationLastProgressTime < Definition.NavigationStallTimeout)
	{
		return false;
	}

	RejectNavigationPath(Record, WorldTime, TEXT("NavigationExecutionRejected"));
	return TryEnterCrowdPushing(Record, WorldTime);
}

bool UGobulinEnemySubsystem::HasNavigationRecoveryContextChanged(
	const FActorEnemyRecord& Record) const
{
	const FGobulinEnemyRuntimeData& RuntimeData = Record.RuntimeData;
	const FGobulinEnemyCrowdData& Crowd = RuntimeData.Crowd;
	const AGobulinEnemyActor* Actor = Record.Actor.Get();
	if (!Crowd.bNavigationPathRejected)
	{
		return true;
	}
	if (!Actor || !RuntimeData.Target.IsSet())
	{
		return false;
	}

	const FGobulinEnemyCrowdDefinition& Definition = RuntimeData.Stats.Crowd;
	const FVector ActorLocation = Actor->GetActorLocation();
	const FVector TargetLocation = RuntimeData.Target.LastKnownLocation;
	const float HeightDelta = FMath::Abs(
		ActorLocation.Z - Crowd.NavigationRejectionAgentLocation.Z);
	const float TargetMovement = FVector::Dist(
		TargetLocation,
		Crowd.NavigationRejectionTargetLocation);
	const float TargetProgress = Crowd.NavigationRejectionTargetDistance
		- FVector::Dist(ActorLocation, TargetLocation);
	return HeightDelta >= Definition.NavigationRecoveryHeight
		|| TargetMovement >= Definition.NavigationRecoveryDistance
		|| TargetProgress >= Definition.NavigationRecoveryDistance;
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
			|| !TargetSnapshot->IsValid()
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

	const bool bMaintainingCurrentContact = RuntimeData.ContactDamage.bInContact
		&& RuntimeData.ContactDamage.ContactTarget == TargetSnapshot->Handle;
	const float ContactTolerance = bMaintainingCurrentContact
		? RuntimeData.Stats.ContactDamage.ContactExitTolerance
		: RuntimeData.Stats.ContactDamage.ContactEnterTolerance;
	const bool bWithinContactDistance = AreUprightCapsulesWithinTolerance(
		Actor->GetActorLocation(),
		RuntimeData.BodyShape,
		TargetSnapshot->Location,
		TargetSnapshot->BodyShape,
		ContactTolerance);
	const bool bHasContact = bWithinContactDistance
		&& HasClearContactPath(*Actor, TargetSnapshot->Handle);
	if (bHasContact)
	{
		if (!RuntimeData.ContactDamage.bInContact
			|| RuntimeData.ContactDamage.ContactTarget != TargetSnapshot->Handle)
		{
			SetEnemyDebugMovementEvent(*Record, TEXT("ContactEntered"));
			UE_VLOG(Actor, LogGobulinEnemy, Log, TEXT("Contact entered target=%s"), *TargetSnapshot->Handle.ToString());
		}
		RuntimeData.ContactDamage.Enter(TargetSnapshot->Handle);
		EnterAttackReady(Enemy, *Record, WorldTime);
		TryApplyContactDamage(Enemy, *Record, *TargetSnapshot, WorldTime);
		return;
	}

	LeaveContact(*Record);
	if (bWithinContactDistance)
	{
		// 几何距离足够近但 CombatTrace 被世界阻挡，不能隔墙造成接触伤害。
		if (RuntimeData.Movement.Status != EEnemyMoveStatus::CrowdPushing
			&& TryEnterCrowdPushing(*Record, WorldTime))
		{
			return;
		}
		if (RuntimeData.Movement.Status != EEnemyMoveStatus::CrowdPushing)
		{
			ClearEnemyTarget(
				Enemy,
				EEnemyTargetChangeReason::NavigationFailed,
				CombatTag_EnemyStateReason_NavigationFailed,
				WorldTime);
			return;
		}
	}

	if (RuntimeData.Movement.Status == EEnemyMoveStatus::Moving)
	{
		if (UpdateNavigationExecutionWatchdog(*Record, WorldTime))
		{
			return;
		}

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

	auto SubmitMoveToTarget = [&]()
	{
		FEnemyMoveIntent Intent;
		Intent.Target = RuntimeData.Target.Handle;
		Intent.Destination = TargetSnapshot->Location;
		Intent.DesiredSpeed = RuntimeData.Stats.MoveSpeed;
		Intent.AcceptanceRadius = RuntimeData.BodyShape.CapsuleRadius
			+ TargetSnapshot->BodyShape.CapsuleRadius
			+ RuntimeData.Stats.ContactDamage.ContactEnterTolerance;
		Intent.RequestedTime = WorldTime;
		Intent.IntentSequence = ++RuntimeData.Movement.LastIntentSequence;
		RuntimeData.Movement.Intent = Intent;
		return Actor->RequestMoveToTarget(TargetActor, Intent);
	};

	if (RuntimeData.Movement.Status == EEnemyMoveStatus::CrowdPushing)
	{
		RuntimeData.Movement.Intent.Destination = TargetSnapshot->Location;
		const UCharacterMovementComponent* Movement = Actor->GetCharacterMovement();
		if (Movement && Movement->IsFalling())
		{
			return;
		}
		const bool bRecoveringRejectedPath = RuntimeData.Crowd.bNavigationPathRejected;
		if (bRecoveringRejectedPath && !HasNavigationRecoveryContextChanged(*Record))
		{
			return;
		}
		if (WorldTime < RuntimeData.Crowd.NextNavigationRetryTime)
		{
			return;
		}

		RuntimeData.Crowd.NextNavigationRetryTime = WorldTime
			+ GetCrowdNavigationRetryDelay(RuntimeData);
		if (!Actor->HasCompleteNavigationPathToTarget(
			TargetActor,
			RuntimeData.Movement.Intent))
		{
			if (bRecoveringRejectedPath)
			{
				RejectNavigationPath(
					*Record,
					WorldTime,
					TEXT("NavigationRecoveryProbeFailed"));
			}
			else
			{
				SetEnemyDebugMovementEvent(*Record, TEXT("CrowdPathProbeFailed"));
			}
			UE_VLOG(
				Actor,
				LogGobulinEnemy,
				Log,
				TEXT("Crowd fallback path probe failed; movement remains under crowd drive."));
			return;
		}

		const EEnemyMoveStatus RetryStatus = SubmitMoveToTarget();
		if (RetryStatus == EEnemyMoveStatus::Moving
			|| RetryStatus == EEnemyMoveStatus::Reached)
		{
			RuntimeData.Crowd.NextNavigationRetryTime = 0.0f;
			SetEnemyMoveStatus(*Record, RetryStatus, WorldTime);
			if (RetryStatus == EEnemyMoveStatus::Reached)
			{
				RuntimeData.NextBehaviorUpdateTime = WorldTime;
			}
			return;
		}

		SetEnemyDebugMovementEvent(*Record, TEXT("CrowdRepathFailed"));
		if (bRecoveringRejectedPath)
		{
			RejectNavigationPath(
				*Record,
				WorldTime,
				TEXT("NavigationRecoveryMoveFailed"));
		}
		UE_VLOG(
			Actor,
			LogGobulinEnemy,
			Log,
			TEXT("Crowd fallback repath failed after a successful path probe."));
		return;
	}

	if (RuntimeData.Movement.Status == EEnemyMoveStatus::Blocked)
	{
		if (TryEnterCrowdPushing(*Record, WorldTime))
		{
			return;
		}
		ClearEnemyTarget(
			Enemy,
			EEnemyTargetChangeReason::NavigationFailed,
			CombatTag_EnemyStateReason_NavigationFailed,
			WorldTime);
		return;
	}

	const EEnemyMoveStatus SubmissionStatus = SubmitMoveToTarget();
	if (SubmissionStatus != EEnemyMoveStatus::Moving
		&& SubmissionStatus != EEnemyMoveStatus::Reached
		&& TryEnterCrowdPushing(*Record, WorldTime))
	{
		return;
	}
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
		// MoveTo 的中心距离测试不能替代逻辑胶囊和遮挡验证，下一次行为更新再确认接触。
		RuntimeData.NextBehaviorUpdateTime = WorldTime;
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
	ClearNavigationExecutionTracking(Record);
	SetEnemyDebugMovementEvent(
		Record,
		FString::Printf(TEXT("TargetAssigned:%s"), *Target.Handle.ToString()));
	if (AGobulinEnemyActor* Actor = Record.Actor.Get())
	{
		UE_VLOG(Actor, LogGobulinEnemy, Log, TEXT("Target assigned %s"), *Target.Handle.ToString());
	}
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
		UE_VLOG(
			Actor,
			LogGobulinEnemy,
			Log,
			TEXT("Target cleared %s reason=%s"),
			*PreviousTarget.ToString(),
			*GetTargetChangeReasonDebugName(Reason));
	}
	SetEnemyDebugMovementEvent(
		*Record,
		FString::Printf(
			TEXT("TargetCleared:%s"),
			*GetTargetChangeReasonDebugName(Reason)));

	Record->RuntimeData.Target.Clear(WorldTime);
	ClearNavigationExecutionTracking(*Record);
	LeaveContact(*Record);
	// Blocked/Failed 事实已经在移动状态事件中发布；清除目标后执行器状态必须复位，
	// 否则导航重试获得新目标时会继续沿用上一目标的失败状态。
	Record->RuntimeData.Movement.Status = EEnemyMoveStatus::Idle;
	Record->RuntimeData.Movement.StatusChangeTime = WorldTime;

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
	if (NewStatus == EEnemyMoveStatus::Moving
		|| NewStatus == EEnemyMoveStatus::Reached)
	{
		Record.RuntimeData.Crowd.bNavigationPathRejected = false;
		Record.RuntimeData.Crowd.NavigationRejectionAgentLocation = FVector::ZeroVector;
		Record.RuntimeData.Crowd.NavigationRejectionTargetLocation = FVector::ZeroVector;
		Record.RuntimeData.Crowd.NavigationRejectionTargetDistance = 0.0f;
		if (NewStatus == EEnemyMoveStatus::Moving)
		{
			ResetNavigationProgress(Record, WorldTime);
		}
	}
	else if (PreviousStatus == EEnemyMoveStatus::Moving)
	{
		Record.RuntimeData.Crowd.bNavigationProgressInitialized = false;
	}
	if (PreviousStatus == EEnemyMoveStatus::CrowdPushing
		&& NewStatus != EEnemyMoveStatus::CrowdPushing)
	{
		Record.RuntimeData.Crowd.NextNavigationRetryTime = 0.0f;
	}
	SetEnemyDebugMovementEvent(
		Record,
		FString::Printf(
			TEXT("Move:%s->%s"),
			*GetEnemyMoveDebugName(PreviousStatus),
			*GetEnemyMoveDebugName(NewStatus)));
	if (AGobulinEnemyActor* Actor = Record.Actor.Get())
	{
		UE_VLOG(
			Actor,
			LogGobulinEnemy,
			Log,
			TEXT("Move status %s -> %s intent=%d"),
			*GetEnemyMoveDebugName(PreviousStatus),
			*GetEnemyMoveDebugName(NewStatus),
			Record.RuntimeData.Movement.Intent.IntentSequence);
	}
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

bool UGobulinEnemySubsystem::HasClearContactPath(
	const AGobulinEnemyActor& EnemyActor,
	FCombatantHandle Target) const
{
	const UWorld* World = GetWorld();
	const UCombatantRegistrySubsystem* Registry = World
		? World->GetSubsystem<UCombatantRegistrySubsystem>()
		: nullptr;
	const AActor* TargetActor = Registry ? Registry->ResolveActor(Target) : nullptr;
	if (!World || !TargetActor)
	{
		return false;
	}

	const FVector Start = EnemyActor.GetActorLocation();
	const FVector End = TargetActor->GetActorLocation();
	if (Start.Equals(End, KINDA_SMALL_NUMBER))
	{
		return true;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GobulinEnemyContact), false);
	QueryParams.AddIgnoredActor(&EnemyActor);
	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(
		Hits,
		Start,
		End,
		GobulinCollision::CombatTrace,
		QueryParams);
	for (const FHitResult& Hit : Hits)
	{
		if (Hit.GetActor() == TargetActor)
		{
			return true;
		}
		if (Hit.bBlockingHit)
		{
			return false;
		}
	}
	return false;
}

void UGobulinEnemySubsystem::LeaveContact(FActorEnemyRecord& Record)
{
	if (Record.RuntimeData.ContactDamage.bInContact)
	{
		SetEnemyDebugMovementEvent(Record, TEXT("ContactLeft"));
		if (AGobulinEnemyActor* Actor = Record.Actor.Get())
		{
			UE_VLOG(Actor, LogGobulinEnemy, Log, TEXT("Contact left."));
		}
	}
	Record.RuntimeData.ContactDamage.Leave();
}

void UGobulinEnemySubsystem::UpdateEnemyDebugTracking(float WorldTime)
{
	constexpr float SampleInterval = 0.5f;

	for (TPair<FCombatantHandle, FActorEnemyRecord>& Pair : ActiveEnemies)
	{
		FActorEnemyRecord& Record = Pair.Value;
		AGobulinEnemyActor* Actor = Record.Actor.Get();
		if (!Actor)
		{
			continue;
		}

		const FGobulinEnemyRuntimeData& RuntimeData = Record.RuntimeData;
		FEnemyDebugRecord& Debug = Record.Debug;
		const FVector ActorLocation = Actor->GetActorLocation();
		const bool bCanMeasureProgress = RuntimeData.IsAlive()
			&& RuntimeData.Target.IsSet()
			&& !RuntimeData.ContactDamage.bInContact;
		const float TargetDistance = bCanMeasureProgress
			? FVector::Dist(ActorLocation, RuntimeData.Target.LastKnownLocation)
			: 0.0f;

		if (!Debug.bInitialized || Debug.SampleTarget != RuntimeData.Target.Handle)
		{
			Debug.SampleLocation = ActorLocation;
			Debug.SampleTarget = RuntimeData.Target.Handle;
			Debug.SampleTargetDistance = TargetDistance;
			Debug.LastSampleTime = WorldTime;
			Debug.LastTargetProgressTime = WorldTime;
			Debug.DistanceMovedLastSample = 0.0f;
			Debug.TargetDistanceDeltaLastSample = 0.0f;
			Debug.NoTargetProgressTime = 0.0f;
			Debug.bInitialized = true;
		}
		else if (WorldTime - Debug.LastSampleTime >= SampleInterval)
		{
			Debug.DistanceMovedLastSample = FVector::DistXY(
				ActorLocation,
				Debug.SampleLocation);
			Debug.TargetDistanceDeltaLastSample = bCanMeasureProgress
				? Debug.SampleTargetDistance - TargetDistance
				: 0.0f;

			const float RequiredProgress = FMath::Max(
				KINDA_SMALL_NUMBER,
				RuntimeData.Stats.Crowd.NavigationMinimumTargetProgress);
			if (!bCanMeasureProgress
				|| Debug.TargetDistanceDeltaLastSample >= RequiredProgress)
			{
				Debug.LastTargetProgressTime = WorldTime;
			}

			Debug.SampleLocation = ActorLocation;
			Debug.SampleTargetDistance = TargetDistance;
			Debug.LastSampleTime = WorldTime;
		}

		Debug.NoTargetProgressTime = bCanMeasureProgress
			? FMath::Max(0.0f, WorldTime - Debug.LastTargetProgressTime)
			: 0.0f;
		const bool bWasStalled = Debug.bTargetProgressStalled;
		Debug.bTargetProgressStalled = bCanMeasureProgress
			&& Debug.NoTargetProgressTime >= RuntimeData.Stats.Crowd.NavigationStallTimeout
			&& Actor->GetVelocity().Size2D()
				< RuntimeData.Stats.Crowd.NavigationStallSpeedThreshold;

		if (!bWasStalled && Debug.bTargetProgressStalled)
		{
			SetEnemyDebugMovementEvent(Record, TEXT("TargetProgressStalled"));
			UE_VLOG(
				Actor,
				LogGobulinEnemy,
				Warning,
				TEXT("Enemy %s stopped making target progress: state=%s move=%s noProgress=%.2fs neighbors=%d pressure=%.2f"),
				*Pair.Key.ToString(),
				*GetEnemyStateDebugName(RuntimeData.State.CurrentState),
				*GetEnemyMoveDebugName(RuntimeData.Movement.Status),
				Debug.NoTargetProgressTime,
				RuntimeData.Crowd.LocalNeighborCount,
				RuntimeData.Crowd.LocalPressure);
		}
		else if (bWasStalled && !Debug.bTargetProgressStalled)
		{
			SetEnemyDebugMovementEvent(Record, TEXT("TargetProgressResumed"));
			UE_VLOG(
				Actor,
				LogGobulinEnemy,
				Log,
				TEXT("Enemy %s resumed target progress."),
				*Pair.Key.ToString());
		}
	}
}

void UGobulinEnemySubsystem::DrawEnemyDebug() const
{
#if ENABLE_DRAW_DEBUG
	const int32 DebugMode = CVarGobulinEnemyDebug.GetValueOnGameThread();
	UWorld* World = GetWorld();
	if (DebugMode <= 0 || !World)
	{
		return;
	}

	TArray<FGobulinEnemyDebugSnapshot> Snapshots;
	GetAllEnemyDebugSnapshots(Snapshots);
	const int32 SelectedIndex = CVarGobulinEnemyDebugHandle.GetValueOnGameThread();
	const int32 MaximumLabels = FMath::Max(
		0,
		CVarGobulinEnemyDebugMaxLabels.GetValueOnGameThread());
	int32 DrawnLabels = 0;

	auto DrawVector = [World](
		const FVector& Origin,
		const FVector& Value,
		float Scale,
		const FColor& Color)
	{
		if (!Value.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(
				World,
				Origin,
				Origin + Value * Scale,
				12.0f,
				Color,
				false,
				-1.0f,
				0,
				2.0f);
		}
	};

	for (const FGobulinEnemyDebugSnapshot& Snapshot : Snapshots)
	{
		const bool bSelected = Snapshot.Handle.GetIndex() == SelectedIndex;
		const FColor StateColor = Snapshot.bTargetProgressStalled
			? FColor::Red
			: (Snapshot.MoveStatus == TEXT("CrowdPushing") ? FColor::Orange : FColor::Cyan);

		DrawDebugCapsule(
			World,
			Snapshot.ActorLocation,
			Snapshot.CombatCapsuleHalfHeight,
			Snapshot.CombatCapsuleRadius,
			FQuat::Identity,
			StateColor,
			false,
			-1.0f,
			0,
			1.0f);
		DrawDebugSphere(
			World,
			Snapshot.ActorLocation,
			Snapshot.CrowdRadius,
			12,
			FColor::Yellow,
			false,
			-1.0f,
			0,
			0.75f);
		DrawVector(Snapshot.ActorLocation, Snapshot.ActualVelocity, 0.25f, FColor::Cyan);

		if (Snapshot.bHasTarget)
		{
			DrawDebugLine(
				World,
				Snapshot.ActorLocation,
				Snapshot.TargetLocation,
				FColor::Green,
				false,
				-1.0f,
				0,
				0.5f);
		}

		if (bSelected)
		{
			if (Snapshot.bHasGroundSupportSample)
			{
				const FColor SupportColor = Snapshot.bHasCenterGroundSupport
					? FColor::Green
					: FColor::Red;
				DrawDebugSphere(
					World,
					Snapshot.GroundSupportLocation,
					Snapshot.GroundSupportRadius,
					12,
					SupportColor,
					false,
					-1.0f,
					0,
					1.5f);
				DrawDebugLine(
					World,
					Snapshot.ActorLocation
						- FVector::UpVector * Snapshot.CombatCapsuleHalfHeight,
					Snapshot.GroundSupportLocation,
					SupportColor,
					false,
					-1.0f,
					0,
					1.5f);
			}
			DrawDebugSphere(
				World,
				Snapshot.ActorLocation,
				Snapshot.NeighborRange,
				20,
				FColor(128, 128, 128),
				false,
				-1.0f,
				0,
				1.0f);
			DrawDebugSphere(
				World,
				Snapshot.IntentDestination,
				10.0f,
				8,
				FColor::White,
				false,
				-1.0f,
				0,
				1.5f);
			DrawVector(Snapshot.ActorLocation, Snapshot.DriveVelocityChange, 8.0f, FColor::Green);
			DrawVector(Snapshot.ActorLocation, Snapshot.SeparationVelocityChange, 8.0f, FColor::Orange);
			DrawVector(Snapshot.ActorLocation, Snapshot.LiftVelocityChange, 8.0f, FColor::Magenta);

			for (int32 PointIndex = 0; PointIndex < Snapshot.PathPoints.Num(); ++PointIndex)
			{
				DrawDebugPoint(
					World,
					Snapshot.PathPoints[PointIndex],
					10.0f,
					FColor::White,
					false,
					-1.0f,
					0);
				if (PointIndex > 0)
				{
					DrawDebugLine(
						World,
						Snapshot.PathPoints[PointIndex - 1],
						Snapshot.PathPoints[PointIndex],
						FColor::White,
						false,
						-1.0f,
						0,
						2.0f);
				}
			}
		}

		const bool bShouldDrawLabel = bSelected
			|| DebugMode >= 2
			|| Snapshot.bTargetProgressStalled;
		if (!bShouldDrawLabel || DrawnLabels >= MaximumLabels)
		{
			continue;
		}

		const FString Label = FString::Printf(
			TEXT("#%s %s/%s PF:%s%s\nMM:%s G:%d Ledge:%d Support:%s R/D:%.1f/%.1f Vxy:%.0f Vz:%.0f\nN:%d P:%.2f Retry:%.2fs dMove:%.1f dGoal:%.1f NoProg:%.2fs\nNavReject:%d Rec Z/T/G:%.0f/%.0f/%.0f\nEvt:%s"),
			*Snapshot.Handle.ToString(),
			*Snapshot.EnemyState,
			*Snapshot.MoveStatus,
			*Snapshot.PathStatus,
			Snapshot.bPathIntentActive ? TEXT("*") : TEXT(""),
			*Snapshot.MovementMode,
			Snapshot.bGrounded ? 1 : 0,
			Snapshot.bCanWalkOffLedges ? 1 : 0,
			Snapshot.bHasGroundSupportSample
				? (Snapshot.bHasCenterGroundSupport ? TEXT("Valid") : TEXT("Invalid"))
				: TEXT("Unknown"),
			Snapshot.GroundSupportRadius,
			Snapshot.GroundSnapDownHeight,
			Snapshot.ActualVelocity.Size2D(),
			Snapshot.ActualVelocity.Z,
			Snapshot.LocalNeighborCount,
			Snapshot.LocalPressure,
			Snapshot.CrowdNavigationRetryRemaining,
			Snapshot.DistanceMovedLastSample,
			Snapshot.TargetDistanceDeltaLastSample,
			Snapshot.NoTargetProgressTime,
			Snapshot.bNavigationPathRejected ? 1 : 0,
			Snapshot.NavigationRecoveryHeightDelta,
			Snapshot.NavigationRecoveryTargetMovement,
			Snapshot.NavigationRecoveryTargetProgress,
			*Snapshot.LastMovementEvent);
		DrawDebugString(
			World,
			Snapshot.ActorLocation
				+ FVector::UpVector * (Snapshot.CombatCapsuleHalfHeight + 35.0f),
			Label,
			nullptr,
			StateColor,
			0.0f,
			true,
			0.85f);
		++DrawnLabels;
	}
#endif
}

void UGobulinEnemySubsystem::DumpEnemyDebug() const
{
	TArray<FGobulinEnemyDebugSnapshot> Snapshots;
	GetAllEnemyDebugSnapshots(Snapshots);
	UE_LOG(LogGobulinEnemy, Display, TEXT("[EnemyDebug] Snapshot count=%d"), Snapshots.Num());
	for (const FGobulinEnemyDebugSnapshot& Snapshot : Snapshots)
	{
		UE_LOG(
			LogGobulinEnemy,
			Display,
			TEXT("[EnemyDebug] #%s actor=%s state=%s age=%.2f move=%s age=%.2f path=%s intentActive=%d target=%d contact=%d movement=%s grounded=%d ledge=%d supportSample=%d centerSupport=%d supportRadius=%.1f snapDown=%.1f supportLocation=%s location=%s velocity=%s neighbors=%d pressure=%.2f crowdRadius=%.1f neighborRange=%.1f retry=%.2f sampleMove=%.1f targetDelta=%.1f noProgress=%.2f stalled=%d navRejected=%d recoveryZ=%.1f recoveryTargetMove=%.1f recoveryTargetProgress=%.1f targetIntentDelta=%.1f drive=%s separation=%s lift=%s event=%s"),
			*Snapshot.Handle.ToString(),
			*Snapshot.ActorName,
			*Snapshot.EnemyState,
			Snapshot.StateAge,
			*Snapshot.MoveStatus,
			Snapshot.MoveStatusAge,
			*Snapshot.PathStatus,
			Snapshot.bPathIntentActive ? 1 : 0,
			Snapshot.bHasTarget ? 1 : 0,
			Snapshot.bInContact ? 1 : 0,
			*Snapshot.MovementMode,
			Snapshot.bGrounded ? 1 : 0,
			Snapshot.bCanWalkOffLedges ? 1 : 0,
			Snapshot.bHasGroundSupportSample ? 1 : 0,
			Snapshot.bHasCenterGroundSupport ? 1 : 0,
			Snapshot.GroundSupportRadius,
			Snapshot.GroundSnapDownHeight,
			*Snapshot.GroundSupportLocation.ToCompactString(),
			*Snapshot.ActorLocation.ToCompactString(),
			*Snapshot.ActualVelocity.ToCompactString(),
			Snapshot.LocalNeighborCount,
			Snapshot.LocalPressure,
			Snapshot.CrowdRadius,
			Snapshot.NeighborRange,
			Snapshot.CrowdNavigationRetryRemaining,
			Snapshot.DistanceMovedLastSample,
			Snapshot.TargetDistanceDeltaLastSample,
			Snapshot.NoTargetProgressTime,
			Snapshot.bTargetProgressStalled ? 1 : 0,
			Snapshot.bNavigationPathRejected ? 1 : 0,
			Snapshot.NavigationRecoveryHeightDelta,
			Snapshot.NavigationRecoveryTargetMovement,
			Snapshot.NavigationRecoveryTargetProgress,
			Snapshot.TargetToIntentDistance,
			*Snapshot.DriveVelocityChange.ToCompactString(),
			*Snapshot.SeparationVelocityChange.ToCompactString(),
			*Snapshot.LiftVelocityChange.ToCompactString(),
			*Snapshot.LastMovementEvent);
	}
}

void UGobulinEnemySubsystem::SetEnemyDebugMovementEvent(
	FActorEnemyRecord& Record,
	FString Event)
{
	Record.Debug.LastMovementEvent = MoveTemp(Event);
}

void UGobulinEnemySubsystem::TryApplyContactDamage(
	FCombatantHandle Enemy,
	FActorEnemyRecord& Record,
	const FCombatantSnapshot& Target,
	float WorldTime)
{
	FGobulinEnemyRuntimeData& RuntimeData = Record.RuntimeData;
	const FGobulinEnemyContactDamageDefinition& Definition = RuntimeData.Stats.ContactDamage;
	if (!Definition.bEnabled
		|| !RuntimeData.ContactDamage.bInContact
		|| RuntimeData.ContactDamage.ContactTarget != Target.Handle
		|| WorldTime < RuntimeData.ContactDamage.NextDamageTime)
	{
		return;
	}

	UCombatSubsystem* Combat = GetWorld()
		? GetWorld()->GetSubsystem<UCombatSubsystem>()
		: nullptr;
	if (!Combat)
	{
		return;
	}

	FVector ContactDirection = Target.Location - Record.Actor->GetActorLocation();
	ContactDirection.Z = 0.0f;
	ContactDirection = ContactDirection.GetSafeNormal();
	if (ContactDirection.IsNearlyZero())
	{
		ContactDirection = FVector::ForwardVector;
	}

	FCombatDamageRequest Request;
	Request.Source = Enemy;
	Request.Target = Target.Handle;
	Request.BaseAmount = Definition.BaseDamage;
	Request.AttackTag = Definition.AttackTag;
	Request.DamageType = Definition.DamageType;
	Request.HitLocation = Target.Location - ContactDirection * Target.BodyShape.CapsuleRadius;
	Request.HitNormal = -ContactDirection;
	Combat->SubmitDamage(MoveTemp(Request));

	RuntimeData.ContactDamage.NextDamageTime = WorldTime + Definition.DamageInterval;
	++RuntimeData.ContactDamage.DamageSequence;
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
