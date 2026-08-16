#pragma once

#include "CoreMinimal.h"
#include "Core/Damageable.h"
#include "GameFramework/Character.h"
#include "DemoEnemy.generated.h"

class USpriteBillboardComponent;
class UBattleAttributeComponent;
class UEnemyDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDemoEnemyDiedDelegate, ADemoEnemy*, Enemy);

/**
 * M1 占位敌人：纸片精灵 + 属性组件 + 统一伤害入口。
 * 简单行为：朝第一个玩家移动；后续由 Mass/StateTree 替换。
 */
UCLASS()
class GOBULINGAME_API ADemoEnemy : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	ADemoEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** IDamageable：统一伤害入口 */
	virtual void TakeDamage_Implementation(const FDamageInfo& DamageInfo) override;

	/** 兼容经典伤害入口（UGameplayStatics::ApplyDamage 等） */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FDemoEnemyDiedDelegate OnEnemyDied;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TObjectPtr<UEnemyDefinition> EnemyDefinition;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsDead() const { return bDead; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpriteBillboardComponent> SpriteBillboard;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBattleAttributeComponent> Attributes;

	/** 死亡动画播完后的销毁延迟 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy|Death")
	float DeathDestroyDelay = 2.0f;

	void ApplyDefinition();
	void UpdateSpriteState();
	void ApplyNormalSprite();
	void PlayHurtFeedback();
	void Die();

	UFUNCTION()
	void OnHurtTimer();

	UFUNCTION()
	void OnDeathDestroyTimer();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FTimerHandle DeathDestroyTimer;
	FTimerHandle HurtTimer;
	bool bDead = false;
	bool bPlayingHurt = false;
	FName ActiveClip = NAME_None;
};
