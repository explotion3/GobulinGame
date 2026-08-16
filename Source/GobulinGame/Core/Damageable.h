#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Damageable.generated.h"

class AActor;

/** 通用伤害信息：所有伤害入口统一使用 */
USTRUCT(BlueprintType)
struct GOBULINGAME_API FDamageInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float Amount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FName DamageSourceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bIsCritical = false;
};

/** 可受伤接口：玩家、敌人、建筑、魔王殿统一实现 */
UINTERFACE(MinimalAPI, Blueprintable)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

class GOBULINGAME_API IDamageable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Damage")
	void TakeDamage(const FDamageInfo& DamageInfo);
};
