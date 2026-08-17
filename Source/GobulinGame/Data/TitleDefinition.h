#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameTypes.h"
#include "TitleDefinition.generated.h"

/**
 * 称号基础定义：类型 × 作用 + 预设（武器偏好/起始 Perk）。
 * 剧情命名后置，当前只承载机制数据。
 */
UCLASS(BlueprintType)
class GOBULINGAME_API UTitleDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 功能 ID（S01-S05） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName TitleId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	ETitleType TitleType = ETitleType::Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	ETitleFunction PrimaryFunction = ETitleFunction::Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	ETitleFunction SecondaryFunction = ETitleFunction::Tank;

	/** 开局武器偏好 ID。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TArray<FName> PreferredWeaponIds;

	/** 开局 Perk 预设（P01 起） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	TArray<FName> StartingPerkIds;
};
