#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "SpriteFlipbook.generated.h"

/** 一段动画片段：引用图集中的连续帧 */
USTRUCT(BlueprintType)
struct FSpriteClip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clip")
	FName ClipId;

	/** 起始帧序号（从 0 开始，行优先） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clip")
	int32 StartFrame = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clip")
	int32 FrameCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Clip")
	bool bLoop = true;
};

/**
 * 2D 纸片角色的图集资产：一张图集 = 方向行 × 动画帧列。
 * 支持 1 方向（面向相机）与 8 方向（4 方向绘制 + 水平镜像）两种布局。
 */
UCLASS(BlueprintType)
class GOBULINGAME_API USpriteFlipbook : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlas")
	TObjectPtr<UTexture2D> Atlas;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlas")
	int32 Columns = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlas")
	int32 Rows = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlas")
	float FrameRate = 8.0f;

	/** 方向数：1 = 始终面向相机；4 = 四向；8 = 八向 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlas")
	int32 DirectionCount = 1;

	/** true = 每个方向独占若干行；false = 所有方向共用行（依赖水平镜像） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlas")
	bool bDirectionsUseSeparateRows = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Clips")
	TArray<FSpriteClip> Clips;

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	FVector2D GetFrameSize() const;

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	FVector2D GetFrameOffset(int32 DirectionIndex, int32 FrameIndex) const;

	const FSpriteClip* GetClip(FName ClipId) const;
};
