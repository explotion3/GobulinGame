#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyDefinition.generated.h"

class USpriteFlipbook;

/**
 * 敌人基础定义：身份 / 战斗 / 纸片表现 / 掉落。
 * 剧情命名后置，当前只承载机制数据（对应 docs/08-M1设计包 E01/E02/E08）。
 */
UCLASS(BlueprintType)
class GOBULINGAME_API UEnemyDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 功能 ID（E01-E14） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FName EnemyId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float MaxHealth = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat")
	float MoveSpeed = 250.0f;

	/** 纸片图集（自定义 SpriteFlipbook，非 Paper2D） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<USpriteFlipbook> Flipbook;

	/** 独立动作图集：优先于 Flipbook 使用（裁剪后的单动作条） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<USpriteFlipbook> IdleFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<USpriteFlipbook> AttackFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<USpriteFlipbook> HurtFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<USpriteFlipbook> DeathFlipbook;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	FName IdleClip = TEXT("Idle");

	/** 无移动动画时留空，保持 Idle */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	FName MoveClip = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	FName HurtClip = TEXT("Block");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	FName DeathClip = TEXT("Downed");

	/** 受击反馈动画的停留时间，之后回到 Idle/Move */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	float HurtFeedbackDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	int32 GoldMin = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	int32 GoldMax = 2;
};
