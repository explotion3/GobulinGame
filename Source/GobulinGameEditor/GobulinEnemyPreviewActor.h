#pragma once

#include "CoreMinimal.h"
#include "Enemy/GobulinEnemyPresentationTypes.h"
#include "GameFramework/Actor.h"
#include "GobulinEnemyPreviewActor.generated.h"

class UArrowComponent;
class UCapsuleComponent;
class UGobulinEnemyArchetype;
class UGobulinEnemyPresentationComponent;
class USceneComponent;
struct FPropertyChangedEvent;

/** 编辑器装配台当前预览并烘焙的基本敌人动画类型。 */
UENUM(BlueprintType)
enum class EGobulinEnemyPreviewAnimation : uint8
{
	Idle UMETA(DisplayName = "四方向待机"),
	Run UMETA(DisplayName = "四方向奔跑"),
	Death UMETA(DisplayName = "死亡")
};

/**
 * 仅用于编辑器的敌人装配台：在视口调整组件，并把白名单参数写回 EnemyArchetype。
 * 不注册战斗句柄，不参与 PIE，也不会进入 Cook。
 */
UCLASS(Blueprintable)
class AGobulinEnemyPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AGobulinEnemyPreviewActor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/** 从目标资产重新载入身体与表现参数；会覆盖当前尚未烘焙的视口调整。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Enemy|Preview", meta = (DisplayName = "从敌人资产加载"))
	void LoadFromArchetype();

	/** 将胶囊、当前方向或死亡 Flipbook 和表现白名单参数写入目标资产，并标记为待保存。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Enemy|Preview", meta = (DisplayName = "烘焙到敌人资产"))
	void BakeToArchetype();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Preview|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CollisionCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Preview|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> FeetAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Preview|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGobulinEnemyPresentationComponent> PresentationComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Preview|Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> GroundForwardMarker;

	/** 烘焙目标，例如 /Game/_Game/Enemy/EA_BasicEnemy。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy|Preview", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGobulinEnemyArchetype> TargetArchetype;

	/** 当前视口显示和烘焙的是 Idle、Run 还是死亡动画。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy|Preview", meta = (AllowPrivateAccess = "true"))
	EGobulinEnemyPreviewAnimation PreviewAnimation = EGobulinEnemyPreviewAnimation::Run;

	/** Preview Animation 不是死亡时，当前视口显示和烘焙的方向槽。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy|Preview", meta = (AllowPrivateAccess = "true", EditCondition = "PreviewAnimation != EGobulinEnemyPreviewAnimation::Death", EditConditionHides))
	EGobulinEnemyMoveDirection PreviewDirection = EGobulinEnemyMoveDirection::TowardViewer;

	/** 是否在运行时水平朝向本地玩家；编辑器内仍以当前组件旋转展示。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Enemy|Preview", meta = (AllowPrivateAccess = "true"))
	bool bFaceLocalPlayer = true;

	void UpdateFeetAnchor();
	void RefreshPreviewAnimation();
	void LoadReferencedAssetsSynchronously();
	bool ValidateBake(FText& OutError) const;
};
