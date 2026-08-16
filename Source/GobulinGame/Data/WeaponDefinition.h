#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/CurveTable.h"
#include "GameTypes.h"
#include "WeaponDefinition.generated.h"

/**
 * 武器基础定义：类型 × 作用 × 弹药 × 手感参数。
 * 剧情命名后置，当前只承载机制数据。
 */
UCLASS(BlueprintType)
class GOBULINGAME_API UWeaponDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 功能 ID（W01-W10） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName WeaponId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	EWeaponType WeaponType = EWeaponType::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	EWeaponFunction PrimaryFunction = EWeaponFunction::HordeClear;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	EWeaponFunction SecondaryFunction = EWeaponFunction::EnergyReturn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	EAmmoType AmmoType = EAmmoType::None;

	/** 基础伤害（距离衰减后按 Falloff 曲线缩放） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float BaseDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	int32 MaxTier = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	int32 ModSlotCount = 3;

	/** Tier 缩放曲线（伤害/射速/换弹等），按 RowName 区分属性 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCurveTable> TierScaling;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handfeel")
	float FireRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handfeel")
	float Spread = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handfeel")
	float Recoil = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handfeel")
	float ProjectileSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handfeel")
	float FalloffStartDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Handfeel")
	float FalloffEndDistance = 0.0f;

	/** 第一人称本地武器的 2D 图集与材质；为空时只执行逻辑开火，不显示武器层。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<class USpriteFlipbook> FirstPersonFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<class UMaterialInterface> FirstPersonSpriteMaterial;
};
