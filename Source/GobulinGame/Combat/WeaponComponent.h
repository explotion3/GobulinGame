#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class UWeaponDefinition;

/** 通用武器组件：按 WeaponDefinition 数据执行开火与命中（hitscan） */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponDefinition(UWeaponDefinition* InDefinition);

	/** 尝试开火：受射速限制，命中者实现 IDamageable 则结算伤害 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	/** 按距离计算最终伤害（线性衰减到 FalloffEnd 处 60%） */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetDamageAtDistance(float Distance) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UWeaponDefinition> WeaponDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace")
	float MaxTraceDistance = 10000.0f;

protected:
	float NextFireTime = 0.0f;

	bool GetCameraView(FVector& OutLocation, FRotator& OutRotation) const;
};
