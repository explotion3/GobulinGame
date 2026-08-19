#pragma once

#include "CoreMinimal.h"
#include "EnemyCrowdTypes.generated.h"

/**
 * 连续怪潮堆积参数。这里只描述后端无关的规则；Actor 与未来 Mass 后端消费同一组语义。
 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyCrowdDefinition
{
	GENERATED_BODY()

	/** 是否让该敌人参与连续怪潮求解；开启后活敌之间不再使用刚体碰撞分离。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd")
	bool bEnableContinuousPiling = true;

	/** 虚拟拥挤粒子半径相对逻辑胶囊半径的比例；越小越容易形成紧密、互相遮叠的怪潮。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.1"))
	float ParticleRadiusScale = 0.65f;

	/** 邻居统计范围相对两粒子半径和的倍率；它决定局部密度而不是实际分离距离。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "1.0"))
	float NeighborRangeScale = 1.6f;

	/** 至少有多少个局部邻居时，后排压力才允许转化为向上的攀爬力。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "1"))
	int32 MinimumLiftNeighborCount = 3;

	/** 虚拟粒子重叠时的水平分离加速度，单位为厘米/秒平方。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float HorizontalSeparationAcceleration = 2200.0f;

	/** 后排受到足够局部压力时的最大向上加速度，需高于角色重力才能形成堆积。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float UpwardPressureAcceleration = 1800.0f;

	/** NavMesh 无法继续时，怪物仍朝目标施压的水平加速度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float FallbackDriveAcceleration = 1400.0f;

	/** 进入 CrowdPushing 后重新探测 NavMesh 路径的基础间隔；运行时会按句柄确定性错峰。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.05", Units = "s"))
	float FallbackNavigationRetryInterval = 0.5f;

	/** PathFollowing 声称正在移动、但敌人没有取得目标进展时，多久后否决当前路径并回退到 CrowdPushing。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.1", Units = "s"))
	float NavigationStallTimeout = 1.5f;

	/** 重新计算停滞计时前必须累计取得的目标距离进展。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float NavigationMinimumTargetProgress = 5.0f;

	/** 只有水平速度低于该值时才把无目标进展视为导航执行停滞，避免普通绕行被误判。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm/s"))
	float NavigationStallSpeedThreshold = 10.0f;

	/** 路径被物理执行否决后，目标移动或敌人与目标距离改善达到该值，才允许再次接受导航路径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float NavigationRecoveryDistance = 75.0f;

	/** 路径被物理执行否决后，敌人中心高度变化达到该值，才允许再次接受导航路径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float NavigationRecoveryHeight = 45.0f;

	/**
	 * 脚底中心用于确认有效地面的小胶囊半径，单位为厘米。
	 * 建议保持在 2-4 cm；过小会放大三角形接缝，过大则会重新出现悬边。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.5", Units = "cm"))
	float GroundSupportRadius = 3.0f;

	/**
	 * 仍视为连续地面的最大向下吸附距离，单位为厘米。
	 * 建议保持在 5-8 cm；更大的落差直接进入 Falling，不按下台阶处理。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.5", Units = "cm"))
	float GroundSnapDownHeight = 8.0f;

	/** 怪潮压力能够产生的最大上升速度；不限制受击或致死击飞。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaximumLiftSpeed = 420.0f;

	/** 相对出生中心高度的技术性堆积上限，用于避免异常压力把敌人持续送出世界。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumPileHeight = 1600.0f;

	bool IsValid() const
	{
		return FMath::IsFinite(ParticleRadiusScale)
			&& ParticleRadiusScale >= 0.1f
			&& FMath::IsFinite(NeighborRangeScale)
			&& NeighborRangeScale >= 1.0f
			&& MinimumLiftNeighborCount >= 1
			&& FMath::IsFinite(HorizontalSeparationAcceleration)
			&& HorizontalSeparationAcceleration >= 0.0f
			&& FMath::IsFinite(UpwardPressureAcceleration)
			&& UpwardPressureAcceleration >= 0.0f
			&& FMath::IsFinite(FallbackDriveAcceleration)
			&& FallbackDriveAcceleration >= 0.0f
			&& FMath::IsFinite(FallbackNavigationRetryInterval)
			&& FallbackNavigationRetryInterval >= 0.05f
			&& FMath::IsFinite(NavigationStallTimeout)
			&& NavigationStallTimeout >= 0.1f
			&& FMath::IsFinite(NavigationMinimumTargetProgress)
			&& NavigationMinimumTargetProgress >= 0.0f
			&& FMath::IsFinite(NavigationStallSpeedThreshold)
			&& NavigationStallSpeedThreshold >= 0.0f
			&& FMath::IsFinite(NavigationRecoveryDistance)
			&& NavigationRecoveryDistance >= 0.0f
			&& FMath::IsFinite(NavigationRecoveryHeight)
			&& NavigationRecoveryHeight >= 0.0f
			&& FMath::IsFinite(GroundSupportRadius)
			&& GroundSupportRadius >= 0.5f
			&& FMath::IsFinite(GroundSnapDownHeight)
			&& GroundSnapDownHeight >= 0.5f
			&& FMath::IsFinite(MaximumLiftSpeed)
			&& MaximumLiftSpeed >= 0.0f
			&& FMath::IsFinite(MaximumPileHeight)
			&& MaximumPileHeight >= 0.0f;
	}
};

/** 单个敌人的后端无关怪潮观测数据，后续可直接映射为 Mass Fragment。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyCrowdData
{
	GENERATED_BODY()

	/** 出生时胶囊中心高度；技术性堆积上限以此为基准。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	float ReferenceCenterZ = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	int32 LocalNeighborCount = 0;

	/** 0 表示无局部压力；1 表示邻居数量刚达到抬升阈值，可暂时大于 1。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	float LocalPressure = 0.0f;

	/** CrowdPushing 下一次允许重新提交导航路径的 World 时间。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	float NextNavigationRetryTime = 0.0f;

	/** Moving 状态中用于累计目标进展的参考距离。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	float NavigationProgressReferenceDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	FVector NavigationProgressTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	float NavigationLastProgressTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	bool bNavigationProgressInitialized = false;

	/** true 表示 NavMesh 路径曾被 Character 的实际执行结果否决，不能仅凭相同路径再次恢复 Moving。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	bool bNavigationPathRejected = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	FVector NavigationRejectionAgentLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	FVector NavigationRejectionTargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd")
	float NavigationRejectionTargetDistance = 0.0f;
};
