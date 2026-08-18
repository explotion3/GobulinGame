#pragma once

#include "CoreMinimal.h"
#include "Enemy/EnemyStateTypes.h"
#include "GobulinEnemyPresentationTypes.generated.h"

class UMaterialInterface;
class UPaperFlipbook;

/** 敌人的语义表现状态；多个玩法状态可以共用同一个动画状态。 */
UENUM(BlueprintType)
enum class EGobulinEnemyVisualState : uint8
{
	Inactive,
	Alive,
	Death
};

/** 基本敌人相对本地观察者的四方向移动表现。左右均以观察者屏幕方向为准。 */
UENUM(BlueprintType)
enum class EGobulinEnemyMoveDirection : uint8
{
	TowardViewer UMETA(DisplayName = "朝向观察者"),
	AwayFromViewer UMETA(DisplayName = "远离观察者"),
	ViewerLeft UMETA(DisplayName = "观察者左侧"),
	ViewerRight UMETA(DisplayName = "观察者右侧")
};

/** 基本敌人的方向化待机与奔跑动画类型。 */
UENUM(BlueprintType)
enum class EGobulinEnemyLocomotionAnimation : uint8
{
	Idle UMETA(DisplayName = "待机"),
	Run UMETA(DisplayName = "奔跑")
};

/** Actor 与未来 Mass 后端共用的敌人身体尺寸。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyBodyDefinition
{
	GENERATED_BODY()

	/** 胶囊体半径，单位为厘米。请直接调整尺寸，不要缩放预览 Actor。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Body", meta = (ClampMin = "1.0", Units = "cm"))
	float CapsuleRadius = 34.0f;

	/** 胶囊体半高，单位为厘米，必须不小于半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Body", meta = (ClampMin = "1.0", Units = "cm"))
	float CapsuleHalfHeight = 88.0f;

	bool IsValid() const;
};

/** 一组相对本地观察者的四方向 Flipbook。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyDirectionalFlipbookSet
{
	GENERATED_BODY()

	/** 敌人朝向本地观察者时使用。FieldPaladin 资源对应 D。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	TSoftObjectPtr<UPaperFlipbook> TowardViewer;

	/** 敌人远离本地观察者时使用。FieldPaladin 资源对应 U。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	TSoftObjectPtr<UPaperFlipbook> AwayFromViewer;

	/** 敌人朝本地观察者屏幕左侧时使用。FieldPaladin 资源对应 L。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	TSoftObjectPtr<UPaperFlipbook> ViewerLeft;

	/** 敌人朝本地观察者屏幕右侧时使用。FieldPaladin 资源对应 R。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	TSoftObjectPtr<UPaperFlipbook> ViewerRight;

	TSoftObjectPtr<UPaperFlipbook> GetForDirection(EGobulinEnemyMoveDirection Direction) const;
	bool SetForDirection(EGobulinEnemyMoveDirection Direction, UPaperFlipbook* Flipbook);
	bool IsValid() const;
};

/** 基本敌人的四方向 Idle、Run 与死亡动画集合；Paper2D 是当前 Actor 后端的编辑源。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyFlipbookSet
{
	GENERATED_BODY()

	/** 静止时按最后一个有效移动方向循环播放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	FGobulinEnemyDirectionalFlipbookSet Idle;

	/** 移动时按实际速度相对本地观察者的方向循环播放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	FGobulinEnemyDirectionalFlipbookSet Run;

	/** 死亡时单次播放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	TSoftObjectPtr<UPaperFlipbook> Death;

	TSoftObjectPtr<UPaperFlipbook> GetForLocomotion(
		EGobulinEnemyMoveDirection Direction,
		EGobulinEnemyLocomotionAnimation Animation) const;
	bool SetForLocomotion(
		EGobulinEnemyMoveDirection Direction,
		EGobulinEnemyLocomotionAnimation Animation,
		UPaperFlipbook* Flipbook);
	bool IsValid() const;
};

/** 可从 EnemyPreviewActor 烘焙、并由运行时后端消费的纸片表现定义。 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FGobulinEnemyPaperPresentationDefinition
{
	GENERATED_BODY()

	/** 相对敌人脚底锚点的表现 Transform。Actor 世界 Transform 不会被烘焙。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	FTransform VisualTransformFromGround = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	FGobulinEnemyFlipbookSet Flipbooks;

	/** 为空时使用各 PaperFlipbook 自带的默认材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	TSoftObjectPtr<UMaterialInterface> MaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	FLinearColor SpriteColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	int32 TranslucencySortPriority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	bool bCastShadow = false;

	/** 运行时让纸片水平朝向本地玩家；编辑器预览仍保留烘焙的基础旋转。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation")
	bool bFaceLocalPlayer = true;

	/** 水平速度低于该值时从 Run 切换到同方向 Idle，单位为厘米/秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MinimumDirectionalSpeed = 10.0f;

	/** 四方向分界附近的切换滞后，避免路径和避让造成动画来回闪烁。推荐 0.1 到 0.2。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DirectionSwitchHysteresis = 0.12f;

	bool IsValid() const;
	UPaperFlipbook* GetLoadedLocomotionFlipbook(
		EGobulinEnemyMoveDirection Direction,
		EGobulinEnemyLocomotionAnimation Animation) const;
	UPaperFlipbook* GetLoadedDeathFlipbook() const;
};

GOBULINGAME_API EGobulinEnemyVisualState GetEnemyVisualState(EEnemyState EnemyState);
