#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GobulinPlayerStatusWidget.generated.h"

class SBorder;
class SProgressBar;
class STextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGobulinRestartRequestedSignature);

/**
 * 正式 UI 接入前使用的原生玩家状态界面。
 * C++ 默认实现可直接运行；蓝图子类可以提供自己的 WidgetTree，并通过表现事件接管视觉。
 */
UCLASS(BlueprintType, Blueprintable)
class GOBULINGAME_API UGobulinPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 更新当前生命值。最大生命值小于等于零时，进度按空处理。 */
	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void SetHealth(float CurrentHealth, float MaximumHealth);

	/** 播放一次已经确认生效的受伤反馈。 */
	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void PlayDamageFeedback(float AppliedDamage);

	/** 显示死亡遮罩与重开入口。 */
	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void ShowDeath();

	/** 隐藏死亡遮罩。供后续复活流程复用。 */
	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void HideDeath();

	/** 由原生按钮或后续蓝图按钮调用，不直接负责重载关卡。 */
	UFUNCTION(BlueprintCallable, Category = "Player|UI")
	void RequestRestart();

	UPROPERTY(BlueprintAssignable, Category = "Player|UI")
	FGobulinRestartRequestedSignature OnRestartRequested;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 红闪持续时间（秒）。推荐 0.15 到 0.35。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|UI|Damage")
	float DamageFlashDuration = 0.22f;

	/** 轻微伤害时的红闪不透明度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|UI|Damage")
	float MinimumDamageFlashOpacity = 0.18f;

	/** 大额伤害时的红闪不透明度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|UI|Damage")
	float MaximumDamageFlashOpacity = 0.52f;

	/** 蓝图子类使用自己的 WidgetTree 时，通过这些事件接管视觉表现。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Player|UI", meta = (DisplayName = "玩家生命值已变化"))
	void ReceiveHealthChanged(float CurrentHealth, float MaximumHealth, float HealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player|UI", meta = (DisplayName = "玩家受到伤害"))
	void ReceiveDamageFeedback(float AppliedDamage);

	UFUNCTION(BlueprintImplementableEvent, Category = "Player|UI", meta = (DisplayName = "玩家死亡界面已显示"))
	void ReceiveDeathShown();

	UFUNCTION(BlueprintImplementableEvent, Category = "Player|UI", meta = (DisplayName = "玩家死亡界面已隐藏"))
	void ReceiveDeathHidden();

private:
	FReply HandleRestartClicked();
	void UpdateNativeHealthVisuals();
	void UpdateNativeDamageFlash(float Opacity);

	TSharedPtr<SProgressBar> HealthProgressBar;
	TSharedPtr<STextBlock> HealthText;
	TSharedPtr<SBorder> DamageFlashBorder;
	TSharedPtr<SBorder> DeathOverlay;

	float DisplayedCurrentHealth = 0.0f;
	float DisplayedMaximumHealth = 0.0f;
	float DamageFlashRemaining = 0.0f;
	float DamageFlashPeakOpacity = 0.0f;
	bool bDeathVisible = false;
};
