#include "Enemy/GobulinEnemyArchetype.h"

#include "Core/CombatTags.h"

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
		&& ContactDamage.IsValid()
		&& FMath::IsFinite(DecisionInterval)
		&& DecisionInterval >= 0.05f
		&& FMath::IsFinite(NavigationRetryDelay)
		&& NavigationRetryDelay >= 0.05f
		&& FMath::IsFinite(AvoidanceConsiderationRadius)
		&& AvoidanceConsiderationRadius >= 1.0f
		&& FMath::IsFinite(SpawnDuration)
		&& SpawnDuration >= 0.0f
		&& Reaction.IsValid()
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
	Stats.ContactDamage = ContactDamage;
	Stats.ContactDamage.BaseDamage = FMath::Max(0.0f, ContactDamage.BaseDamage * SafePowerScale);
	Stats.ContactDamage.DamageInterval = FMath::Max(0.05f, ContactDamage.DamageInterval);
	Stats.ContactDamage.ContactEnterTolerance = FMath::Max(0.0f, ContactDamage.ContactEnterTolerance);
	Stats.ContactDamage.ContactExitTolerance = FMath::Max(
		Stats.ContactDamage.ContactEnterTolerance,
		ContactDamage.ContactExitTolerance);
	if (!Stats.ContactDamage.AttackTag.IsValid())
	{
		Stats.ContactDamage.AttackTag = CombatTag_Attack_Melee;
	}
	if (!Stats.ContactDamage.DamageType.IsValid())
	{
		Stats.ContactDamage.DamageType = CombatTag_Damage_Physical;
	}
	Stats.DecisionInterval = FMath::Max(0.05f, DecisionInterval);
	Stats.NavigationRetryDelay = FMath::Max(0.05f, NavigationRetryDelay);
	Stats.SpawnDuration = FMath::Max(0.0f, SpawnDuration);
	Stats.Reaction = Reaction;
	if (!Stats.Reaction.HeavyAttackTag.IsValid())
	{
		Stats.Reaction.HeavyAttackTag = CombatTag_Attack_Melee_Heavy;
	}
	return Stats;
}
