#include "Enemy/GobulinEnemyArchetype.h"

const FPrimaryAssetType UGobulinEnemyArchetype::PrimaryAssetType(TEXT("EnemyArchetype"));
const FName UGobulinEnemyArchetype::PresentationBundle(TEXT("Presentation"));

FPrimaryAssetId UGobulinEnemyArchetype::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryAssetType, GetFName());
}

bool UGobulinEnemyArchetype::IsDefinitionValid() const
{
	return FMath::IsFinite(MaxHealth)
		&& MaxHealth >= 1.0f
		&& FMath::IsFinite(MoveSpeed)
		&& MoveSpeed >= 0.0f
		&& FMath::IsFinite(TargetAcquisitionRadius)
		&& TargetAcquisitionRadius >= 0.0f
		&& FMath::IsFinite(TargetLoseRadius)
		&& TargetLoseRadius >= TargetAcquisitionRadius
		&& FMath::IsFinite(AttackReadyDistance)
		&& AttackReadyDistance >= 0.0f
		&& FMath::IsFinite(ResumeMoveDistance)
		&& ResumeMoveDistance >= AttackReadyDistance
		&& FMath::IsFinite(DecisionInterval)
		&& DecisionInterval >= 0.05f
		&& FMath::IsFinite(NavigationRetryDelay)
		&& NavigationRetryDelay >= 0.05f
		&& FMath::IsFinite(AvoidanceConsiderationRadius)
		&& AvoidanceConsiderationRadius >= 1.0f
		&& FMath::IsFinite(SpawnDuration)
		&& SpawnDuration >= 0.0f
		&& FMath::IsFinite(DeathDuration)
		&& DeathDuration >= 0.0f
		&& Body.IsValid()
		&& Presentation.IsValid();
}

FGobulinEnemyRuntimeStats UGobulinEnemyArchetype::BuildRuntimeStats(float PowerScale) const
{
	const float SafePowerScale = FMath::IsFinite(PowerScale) ? FMath::Max(PowerScale, 0.01f) : 1.0f;

	FGobulinEnemyRuntimeStats Stats;
	Stats.MaxHealth = FMath::Max(1.0f, MaxHealth * SafePowerScale);
	Stats.MoveSpeed = FMath::Max(0.0f, MoveSpeed);
	Stats.TargetAcquisitionRadius = FMath::Max(0.0f, TargetAcquisitionRadius);
	Stats.TargetLoseRadius = FMath::Max(Stats.TargetAcquisitionRadius, TargetLoseRadius);
	Stats.AttackReadyDistance = FMath::Max(0.0f, AttackReadyDistance);
	Stats.ResumeMoveDistance = FMath::Max(Stats.AttackReadyDistance, ResumeMoveDistance);
	Stats.DecisionInterval = FMath::Max(0.05f, DecisionInterval);
	Stats.NavigationRetryDelay = FMath::Max(0.05f, NavigationRetryDelay);
	Stats.SpawnDuration = FMath::Max(0.0f, SpawnDuration);
	Stats.DeathDuration = FMath::Max(0.0f, DeathDuration);
	return Stats;
}
