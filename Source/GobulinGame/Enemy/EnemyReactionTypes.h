#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EnemyReactionTypes.generated.h"

/** Actor 与未来 Mass 后端共用的受击、硬直和死亡表现参数。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyReactionDefinition
{
	GENERATED_BODY()

	/** 普通受击暂停追踪和接触伤害的时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", Units = "s"))
	float LightInterruptDuration = 0.12f;

	/** 单次实际伤害达到最大生命比例时触发硬直；例如 0.3 表示 30%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaggerDamageRatioThreshold = 0.30f;

	/** 具备该攻击标签的伤害无视数值阈值，直接尝试触发硬直。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (Categories = "Combat.Attack"))
	FGameplayTag HeavyAttackTag;

	/** 硬直的最短持续时间；若仍在空中，会等待落地或达到空中上限。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", Units = "s"))
	float StaggerDuration = 0.45f;

	/** 硬直结束后的保护时间，避免高频伤害把敌人永久锁住。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", Units = "s"))
	float StaggerImmunityDuration = 0.25f;

	/** 非致死受击等待落地的最长时间，超过后恢复追踪，避免异常地形永久卡状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", Units = "s"))
	float MaximumAirborneReactionDuration = 0.75f;

	/** 伤害请求中 Impulse 转换为 Character 发射速度时的倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0"))
	float KnockbackVelocityScale = 1.0f;

	/** 防止外部伤害源把物理冲量直接当速度导致敌人飞出关卡。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "1.0", Units = "cm/s"))
	float MaximumLaunchVelocity = 1200.0f;

	/** 致死时对水平击退速度追加的倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0"))
	float LethalHorizontalVelocityScale = 2.2f;

	/** 伤害源未提供水平冲量时，致死击飞使用的基础水平速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", Units = "cm/s"))
	float LethalFallbackHorizontalVelocity = 220.0f;

	/** 致死击飞保证的最小垂直速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.0", Units = "cm/s"))
	float LethalMinimumVerticalVelocity = 420.0f;

	/** 死亡飞行阶段使用的 CharacterMovement 重力倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.01"))
	float DeathGravityScale = 1.6f;

	/** 尸体胶囊半径倍率；保持半高不变，稍微收窄可减少拥挤处卡死。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CorpseCapsuleRadiusScale = 0.9f;

	/** 普通命中白闪持续时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float HitFlashDuration = 0.08f;

	/** 致死命中红闪持续时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float LethalFlashDuration = 0.14f;

	/** 致死后延迟多久开始逐渐变黑。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float DeathDarkenDelay = 0.06f;

	/** 从原色过渡到最大变黑程度所需时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float DeathDarkenDuration = 0.60f;

	/** 尸体最终变黑程度，1 为纯黑，建议保留少量轮廓。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeathMaximumDarken = 0.90f;

	/** 尸体落地后保持可见的时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.0", Units = "s"))
	float CorpseSettleDelay = 0.35f;

	/** 尸体不透明度从 1 降到 0 的时间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction|Presentation", meta = (ClampMin = "0.01", Units = "s"))
	float CorpseFadeDuration = 0.45f;

	/** 未正常落地时的死亡总时限；到期仍会完成回收。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Reaction", meta = (ClampMin = "0.1", Units = "s"))
	float DeathMaximumDuration = 4.0f;

	bool IsValid() const
	{
		return FMath::IsFinite(LightInterruptDuration) && LightInterruptDuration >= 0.0f
			&& FMath::IsFinite(StaggerDamageRatioThreshold) && StaggerDamageRatioThreshold >= 0.0f && StaggerDamageRatioThreshold <= 1.0f
			&& FMath::IsFinite(StaggerDuration) && StaggerDuration >= 0.0f
			&& FMath::IsFinite(StaggerImmunityDuration) && StaggerImmunityDuration >= 0.0f
			&& FMath::IsFinite(MaximumAirborneReactionDuration) && MaximumAirborneReactionDuration >= 0.0f
			&& FMath::IsFinite(KnockbackVelocityScale) && KnockbackVelocityScale >= 0.0f
			&& FMath::IsFinite(MaximumLaunchVelocity) && MaximumLaunchVelocity >= 1.0f
			&& FMath::IsFinite(LethalHorizontalVelocityScale) && LethalHorizontalVelocityScale >= 0.0f
			&& FMath::IsFinite(LethalFallbackHorizontalVelocity) && LethalFallbackHorizontalVelocity >= 0.0f
			&& FMath::IsFinite(LethalMinimumVerticalVelocity) && LethalMinimumVerticalVelocity >= 0.0f
			&& FMath::IsFinite(DeathGravityScale) && DeathGravityScale > 0.0f
			&& FMath::IsFinite(CorpseCapsuleRadiusScale) && CorpseCapsuleRadiusScale >= 0.1f && CorpseCapsuleRadiusScale <= 1.0f
			&& FMath::IsFinite(HitFlashDuration) && HitFlashDuration >= 0.0f
			&& FMath::IsFinite(LethalFlashDuration) && LethalFlashDuration >= 0.0f
			&& FMath::IsFinite(DeathDarkenDelay) && DeathDarkenDelay >= 0.0f
			&& FMath::IsFinite(DeathDarkenDuration) && DeathDarkenDuration >= 0.0f
			&& FMath::IsFinite(DeathMaximumDarken) && DeathMaximumDarken >= 0.0f && DeathMaximumDarken <= 1.0f
			&& FMath::IsFinite(CorpseSettleDelay) && CorpseSettleDelay >= 0.0f
			&& FMath::IsFinite(CorpseFadeDuration) && CorpseFadeDuration >= 0.01f
			&& FMath::IsFinite(DeathMaximumDuration)
			&& DeathMaximumDuration >= FMath::Max(0.1f, CorpseFadeDuration);
	}
};

/** 每个敌人的受击运行时记忆；不依赖 Actor 或 Mass Entity。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyReactionData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Reaction")
	float StaggerImmunityEndTime = 0.0f;
};
