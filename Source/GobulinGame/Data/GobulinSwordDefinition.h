#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GobulinSwordDefinition.generated.h"

class UCurveVector;
class UCameraShakeBase;
class UMaterialInterface;
class USoundBase;
class UTexture2D;

/** 第一人称 2D 剑的静态配置数据。 */
UCLASS(BlueprintType)
class GOBULINGAME_API UGobulinSwordDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Presentation")
	TObjectPtr<UTexture2D> WeaponTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Presentation")
	TObjectPtr<UMaterialInterface> WeaponMaterial;

	/** 根据归一化攻击时间 [0, 1] 叠加的局部位置偏移。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Animation")
	TObjectPtr<UCurveVector> AttackLocationCurve;

	/** 根据归一化攻击时间 [0, 1] 叠加的局部旋转偏移，单位为度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Animation")
	TObjectPtr<UCurveVector> AttackRotationCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat", meta = (ClampMin = "0.01"))
	float AttackDuration = 0.45f;

	/** 攻击姿态结束后返回待机姿态所需的时间。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat", meta = (ClampMin = "0.0"))
	float RecoveryDuration = 0.10f;

	/** 输入缓存窗口开启后，是否允许缓存一次攻击输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat")
	bool bCanBufferAttack = true;

	/** 按住攻击键时，是否在当前攻击结束后自动开始下一次攻击。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat")
	bool bCanAutoRepeatAttack = true;

	/** 剑尖开始对目标造成伤害的归一化时间点。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Animation|Events", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HitWindowStartNormalizedTime = 0.18f;

	/** 剑尖停止对目标造成伤害的归一化时间点。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Animation|Events", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float HitWindowEndNormalizedTime = 0.65f;

	/** 攻击时间轴超过该归一化时间点后，允许缓存一次攻击输入。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Animation|Events", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float InputBufferOpenNormalizedTime = 0.75f;

	/** 第一人称挥剑进入快速阶段时播放的破风音效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Audio")
	TObjectPtr<USoundBase> SwingSound;

	/** 播放挥剑破风音效的归一化时间点。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float SwingSoundNormalizedTime = 0.20f;

	/** 确认造成伤害的位置播放的命中音效。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Audio")
	TObjectPtr<USoundBase> HitSound;

	/** 每次攻击首次确认命中时可选播放的相机震动。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Feedback")
	TSubclassOf<UCameraShakeBase> HitCameraShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Feedback", meta = (ClampMin = "0.0"))
	float HitCameraShakeScale = 1.0f;

	/** 每次攻击首次有效命中后，全局停顿的真实时间，单位为秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Feedback", meta = (ClampMin = "0.0", ClampMax = "0.2"))
	float HitStopDuration = 0.04f;

	/** 命中停顿期间使用的全局时间膨胀值；越接近 0，冻结感越强。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Feedback", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float HitStopTimeDilation = 0.05f;

	/** 沿剑尖轨迹进行球体扫掠检测的半径，单位为厘米。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat", meta = (ClampMin = "0.0"))
	float TipTraceRadius = 6.0f;

	/** 攻击期间是否在场景中绘制剑尖检测轨迹。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Debug")
	bool bDebugDrawTipTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat", meta = (ClampMin = "0.0"))
	float BaseDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sword|Combat")
	FName DamageSourceId = TEXT("Sword");
};
