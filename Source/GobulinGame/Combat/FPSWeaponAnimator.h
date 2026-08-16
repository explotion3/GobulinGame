#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSWeaponAnimator.generated.h"

class UFPSWeaponOverlayComponent;

/**
 * 第一人称 2D 武器动画控制器：驱动 Overlay 的 Clip 切换与程序化运动参数。
 * 自动在 Idle/Move 间切换；Fire/Reload/Melee 由玩法代码显式触发。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UFPSWeaponAnimator : public UActorComponent
{
	GENERATED_BODY()

public:
	UFPSWeaponAnimator();

	UFUNCTION(BlueprintCallable, Category = "WeaponAnimator")
	void SetOverlay(UFPSWeaponOverlayComponent* InOverlay);

	UFUNCTION(BlueprintCallable, Category = "WeaponAnimator")
	void PlayClip(FName ClipId, bool bRestart = true);

	UFUNCTION(BlueprintCallable, Category = "WeaponAnimator")
	void TriggerFire();

	UFUNCTION(BlueprintCallable, Category = "WeaponAnimator")
	void TriggerReload();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UFPSWeaponOverlayComponent> Overlay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponAnimator")
	FName IdleClip = TEXT("Idle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponAnimator")
	FName MoveClip = TEXT("Move");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponAnimator")
	FName FireClip = TEXT("Fire");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponAnimator")
	FName ReloadClip = TEXT("Reload");

	/** 移动速度超过该值切换 Move 状态 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponAnimator")
	float MoveThreshold = 10.0f;

	FName ActiveClip = NAME_None;
	FRotator PreviousControlRotation = FRotator::ZeroRotator;
	bool bLockedByAction = false;
};
