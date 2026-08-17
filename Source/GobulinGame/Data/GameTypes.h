#pragma once

#include "CoreMinimal.h"
#include "GameTypes.generated.h"

/** 称号类型：战斗风格 */
UENUM(BlueprintType)
enum class ETitleType : uint8
{
	Melee		UMETA(DisplayName = "Melee"),
	Spell		UMETA(DisplayName = "Spell"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Summon		UMETA(DisplayName = "Summon"),
	Engineer	UMETA(DisplayName = "Engineer")
};

/** 称号作用：团队职能 */
UENUM(BlueprintType)
enum class ETitleFunction : uint8
{
	Damage		UMETA(DisplayName = "Damage"),
	Tank		UMETA(DisplayName = "Tank"),
	Support		UMETA(DisplayName = "Support"),
	Control		UMETA(DisplayName = "Control"),
	BaseDefense	UMETA(DisplayName = "BaseDefense"),
	Economy		UMETA(DisplayName = "Economy")
};

/** 敌人类型：行为形态 */
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
	Infantry	UMETA(DisplayName = "Infantry"),
	Ranged		UMETA(DisplayName = "Ranged"),
	Fast		UMETA(DisplayName = "Fast"),
	Flying		UMETA(DisplayName = "Flying"),
	Tank		UMETA(DisplayName = "Tank"),
	Elite		UMETA(DisplayName = "Elite"),
	Boss		UMETA(DisplayName = "Boss")
};

/** 敌人作用：对玩家的威胁方式 */
UENUM(BlueprintType)
enum class EEnemyFunction : uint8
{
	Chaff		UMETA(DisplayName = "Chaff"),
	Damage		UMETA(DisplayName = "Damage"),
	Control		UMETA(DisplayName = "Control"),
	Support		UMETA(DisplayName = "Support"),
	Siege		UMETA(DisplayName = "Siege"),
	Pressure	UMETA(DisplayName = "Pressure")
};

/** 魔王军兵种 */
UENUM(BlueprintType)
enum class EMinionType : uint8
{
	MeleeInfantry	UMETA(DisplayName = "MeleeInfantry"),
	RangedArcher	UMETA(DisplayName = "RangedArcher"),
	AoEArtillery	UMETA(DisplayName = "AoEArtillery")
};

/** 设施类型 */
UENUM(BlueprintType)
enum class EFacilityType : uint8
{
	Core		UMETA(DisplayName = "Core"),
	Production	UMETA(DisplayName = "Production"),
	Military	UMETA(DisplayName = "Military"),
	Defense		UMETA(DisplayName = "Defense"),
	Utility		UMETA(DisplayName = "Utility"),
	Economy		UMETA(DisplayName = "Economy")
};

/** Perk 系 */
UENUM(BlueprintType)
enum class EPerkCategory : uint8
{
	Weapon		UMETA(DisplayName = "Weapon"),
	Body		UMETA(DisplayName = "Body"),
	Loot		UMETA(DisplayName = "Loot"),
	BaseDefense	UMETA(DisplayName = "BaseDefense"),
	Bounty		UMETA(DisplayName = "Bounty")
};
