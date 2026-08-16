#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "FPSWeaponOverlayComponent.generated.h"

class USpriteFlipbook;
class UMaterialInstanceDynamic;

/**
 * 第一人称 2D 武器贴图层：挂在相机下的 Quad + Flipbook 帧动画。
 * 在帧动画之上叠加程序化摆动（Bob/Sway/Recoil/ADS），仅本地可见。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UFPSWeaponOverlayComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UFPSWeaponOverlayComponent();

	UFUNCTION(BlueprintCallable, Category = "WeaponOverlay")
	void SetFlipbook(USpriteFlipbook* InFlipbook);

	UFUNCTION(BlueprintCallable, Category = "WeaponOverlay")
	void SetSpriteMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable, Category = "WeaponOverlay")
	void PlayClip(FName ClipId, bool bRestart = true);

	UFUNCTION(BlueprintCallable, Category = "WeaponOverlay")
	void SetAiming(bool bInAiming);

	UFUNCTION(BlueprintCallable, Category = "WeaponOverlay")
	void AddRecoil(float Strength = -1.0f);

	/** 每帧由 Animator 驱动：传入移动速度、视角变化量、是否着地 */
	UFUNCTION(BlueprintCallable, Category = "WeaponOverlay")
	void UpdateProceduralMotion(float DeltaTime, float MovementSpeed, const FVector2D& LookDelta, bool bIsGrounded);

	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/** 根据面板配置的 SpriteMaterial/Flipbook 创建动态材质实例（幂等） */
	void InitializeMaterial();

	/** 武器图集：行 = 行为状态（Idle/Move/Fire/Reload/Melee/Switch/ADS），列 = 帧 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponOverlay")
	TObjectPtr<USpriteFlipbook> Flipbook;

	/** 翻页材质（Unlit + 半透明，参数名 UVOffset/UVScale） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponOverlay")
	TObjectPtr<UMaterialInterface> SpriteMaterial;

	/** 静止时的局部位置（相机坐标系） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	FVector RestLocation = FVector(40.0f, 18.0f, -20.0f);

	/** 静止时的局部旋转（Plane 默认 +Z，预转 90° 面向相机，可在编辑器微调） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	FRotator RestRotation = FRotator(90.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	float BobFrequency = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	float BobAmplitude = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	float SwayScale = 0.015f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	float RecoilKick = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	float RecoilRecoverySpeed = 10.0f;

	/** 瞄准时的额外位移（向屏幕中心收拢） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponOverlay|Motion")
	FVector AimLocationOffset = FVector(0.0f, 4.0f, 2.0f);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	FName CurrentClipId = NAME_None;
	int32 CurrentFrame = 0;
	float AccumulatedTime = 0.0f;
	bool bPlaying = false;
	bool bAiming = false;
	float BobPhase = 0.0f;
	float CurrentRecoil = 0.0f;
	FVector2D CurrentSway = FVector2D::ZeroVector;

	void TickFrame(float DeltaTime);
	void ApplyProceduralTransform(float DeltaTime);
	void ApplyFrameUV();
};
