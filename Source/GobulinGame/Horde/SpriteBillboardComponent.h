#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "SpriteBillboardComponent.generated.h"

class USpriteFlipbook;
class UMaterialInstanceDynamic;

/**
 * 面向玩家的 2D 纸片组件：静态四边形 + 材质翻页动画。
 * - 纸片永远正对本地玩家位置（广告牌），不参与敌人的逻辑朝向；
 * - 动画方向行按“敌人逻辑朝向 vs 观察方向”逐客户端选择；
 * - 杂兵可走 Mass 实例化（不创建本组件）；精英/BOSS/NPC 使用本组件逐实例播放。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API USpriteBillboardComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	USpriteBillboardComponent();

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetFlipbook(USpriteFlipbook* InFlipbook);

	/** 设置翻页材质（未注册时只记录引用，OnRegister 时创建 MID）。 */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetSpriteMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintPure, Category = "Sprite")
	UMaterialInterface* GetSpriteMaterial() const { return SpriteMaterial; }

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void PlayClip(FName ClipId, int32 DirectionIndex = -1, bool bRestart = true);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetDirection(int32 DirectionIndex);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void TickFrame(float DeltaTime);

	/** 设置敌人的逻辑朝向（世界空间水平方向），纸片本身仍面向观察者。 */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetFacingDirection(const FVector& InFacingDirection);

	/** 重新启用“跟随 Owner 移动速度方向作为逻辑朝向”。 */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetFacingFollowsVelocity(bool bEnabled);

	/** 设置默认自动播放的 Clip；NAME_None 时播放图集第一个 Clip。 */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetDefaultClipId(FName ClipId);

	UFUNCTION(BlueprintPure, Category = "Sprite")
	float GetFacingYaw() const { return FacingYaw; }

	USpriteFlipbook* GetFlipbook() const { return Flipbook; }

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** 根据面板配置的 SpriteMaterial/Flipbook 创建动态材质实例（幂等） */
	void InitializeMaterial();

	void UpdateFacing();

	int32 ComputeDirectionIndexForViewer(const FVector& ToViewerH) const;

	void ApplyFrameUV();

	/** 没有手动播放时，自动播放 DefaultClipId 或图集第一个 Clip。 */
	void TryAutoPlay();

	/** 生成动态材质实例所用的基础材质（图集翻页材质，参数名 UVOffset/UVScale） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UMaterialInterface> SpriteMaterial;

	/** 是否作为广告牌面向本地玩家位置；关闭后保持自身旋转，不自动选方向。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	bool bAutoFaceCamera = true;

	/** 没有显式朝向时，跟随 Owner 移动速度的水平方向作为逻辑朝向。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	bool bFacingFollowsVelocity = true;

	/** 方向行顺序与默认约定（0=正面，观察者绕行时递增）相反时开启。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	bool bReverseDirectionOrder = false;

	/** 赋值 Flipbook 后自动开始播放，避免蓝图中忘记调用 PlayClip。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	bool bAutoPlayFirstClip = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<USpriteFlipbook> Flipbook;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	UPROPERTY(Transient)
	FName CurrentClipId;

	/** 敌人的逻辑朝向（世界 Yaw），与纸片旋转无关；由移动方向或 SetFacingDirection 驱动。 */
	UPROPERTY(Transient)
	float FacingYaw = 0.0f;

	/** 自动播放优先使用的 Clip；NAME_None = 图集第一个 Clip。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	FName DefaultClipId = NAME_None;

	int32 CurrentDirection = 0;
	int32 CurrentFrame = 0;
	float AccumulatedTime = 0.0f;
	bool bPlaying = false;
};
