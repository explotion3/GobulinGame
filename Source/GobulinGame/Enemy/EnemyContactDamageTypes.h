#pragma once

#include "CoreMinimal.h"
#include "Combat/CombatantHandle.h"
#include "GameplayTagContainer.h"
#include "EnemyContactDamageTypes.generated.h"

/** 基本敌人的接触伤害配置；Actor 与未来 Mass 后端共享同一语义。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyContactDamageDefinition
{
	GENERATED_BODY()

	/** 关闭后仍可追踪和贴近目标，但不会提交接触伤害。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage")
	bool bEnabled = true;

	/** 每个伤害节拍提交的基础伤害；会随生成请求的 PowerScale 缩放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage", meta = (EditCondition = "bEnabled", ClampMin = "0.0"))
	float BaseDamage = 10.0f;

	/** 同一个敌人两次接触伤害之间的最短间隔。卡顿后不会补发历史节拍。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage", meta = (EditCondition = "bEnabled", ClampMin = "0.05", Units = "s"))
	float DamageInterval = 0.8f;

	/** 双方胶囊表面距离不大于该值时首次建立接触。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage", meta = (ClampMin = "0.0", Units = "cm"))
	float ContactEnterTolerance = 5.0f;

	/** 已接触后，胶囊表面距离超过该值才视为脱离；必须不小于进入容差。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage", meta = (ClampMin = "0.0", Units = "cm"))
	float ContactExitTolerance = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage", meta = (EditCondition = "bEnabled", Categories = "Combat.Attack"))
	FGameplayTag AttackTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Contact Damage", meta = (EditCondition = "bEnabled", Categories = "Combat.Damage"))
	FGameplayTag DamageType;

	bool IsValid() const
	{
		return FMath::IsFinite(BaseDamage)
			&& BaseDamage >= 0.0f
			&& FMath::IsFinite(DamageInterval)
			&& DamageInterval >= 0.05f
			&& FMath::IsFinite(ContactEnterTolerance)
			&& ContactEnterTolerance >= 0.0f
			&& FMath::IsFinite(ContactExitTolerance)
			&& ContactExitTolerance >= ContactEnterTolerance
			&& (!bEnabled || BaseDamage > 0.0f);
	}
};

/** 单个敌人的接触伤害运行时数据；不保存 Actor、组件或计时器。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FEnemyContactDamageData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Contact Damage")
	FCombatantHandle ContactTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Contact Damage")
	bool bInContact = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Contact Damage")
	float NextDamageTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Contact Damage")
	int32 DamageSequence = 0;

	void Enter(FCombatantHandle Target)
	{
		ContactTarget = Target;
		bInContact = Target.IsSet();
	}

	/** 脱离不会重置 NextDamageTime，避免快速进出绕过伤害冷却。 */
	void Leave()
	{
		ContactTarget.Reset();
		bInContact = false;
	}
};
