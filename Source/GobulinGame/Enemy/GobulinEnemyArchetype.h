#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/EnemyContactDamageTypes.h"
#include "Enemy/EnemyCrowdTypes.h"
#include "Enemy/EnemyReactionTypes.h"
#include "Enemy/GobulinEnemyPresentationTypes.h"
#include "Enemy/GobulinEnemyRuntimeData.h"
#include "GobulinEnemyArchetype.generated.h"

/** Data-only definition shared by Actor and future Mass enemy backends. */
UCLASS(BlueprintType)
class GOBULINGAME_API UGobulinEnemyArchetype : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType PrimaryAssetType;
	static const FName PresentationBundle;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	bool IsDefinitionValid() const;
	FGobulinEnemyRuntimeStats BuildRuntimeStats(float PowerScale) const;

	/** 基础生命值；生成请求的 PowerScale 会统一缩放该值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** 后续移动处理器使用的基础速度，单位为厘米/秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MoveSpeed = 300.0f;

	/** 无目标时搜索敌对战斗单位的最大三维距离。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Targeting", meta = (ClampMin = "0.0", Units = "cm"))
	float TargetAcquisitionRadius = 5000.0f;

	/** 已锁定目标超过该距离时丢失目标；必须不小于搜索距离。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Targeting", meta = (ClampMin = "0.0", Units = "cm"))
	float TargetLoseRadius = 6500.0f;

	/** 胶囊表面接触后立即造成一次伤害，随后按独立节拍重复。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Combat")
	FGobulinEnemyContactDamageDefinition ContactDamage;

	/** 索敌与距离决策频率；路径跟随本身仍由 CharacterMovement 每帧执行。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Targeting", meta = (ClampMin = "0.05", Units = "s"))
	float DecisionInterval = 0.25f;

	/** 路径请求失败后再次尝试索敌和移动前的等待时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "0.05", Units = "s"))
	float NavigationRetryDelay = 1.0f;

	/** Actor 原型是否启用 CharacterMovement 的 RVO 基础避让。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement")
	bool bUseRVOAvoidance = true;

	/** RVO 查询附近移动体的范围；仅影响当前 Actor 后端。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Movement", meta = (ClampMin = "1.0", Units = "cm"))
	float AvoidanceConsiderationRadius = 250.0f;

	/** 连续堆积、局部压力和脱离 NavMesh 后直推所使用的统一怪潮参数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd")
	FGobulinEnemyCrowdDefinition Crowd;

	/** 出生状态持续时间；为零时会在下一个子系统更新中完成出生。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Lifecycle", meta = (ClampMin = "0.0", Units = "s"))
	float SpawnDuration = 0.15f;

	/** 普通受击、硬直、致死击飞与尸体回收的统一参数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Reaction")
	FGobulinEnemyReactionDefinition Reaction;

	/** 逻辑身体尺寸；生成占位、Actor 胶囊和未来 Mass 碰撞共同读取。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Body")
	FGobulinEnemyBodyDefinition Body;

	/** Paper2D 编辑源与 Actor 表现参数；相关软引用按 Presentation Bundle 加载。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Presentation", meta = (AssetBundles = "Presentation"))
	FGobulinEnemyPaperPresentationDefinition Presentation;
};
