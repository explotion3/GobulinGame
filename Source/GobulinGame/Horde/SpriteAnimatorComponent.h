#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpriteAnimatorComponent.generated.h"

class USpriteBillboardComponent;

/**
 * 轻量 2D 动画控制器：负责切换 Clip；方向行由 SpriteBillboard 按本地玩家视角自动选择。
 * 精英/BOSS/NPC 使用；杂兵走 Mass 实例化，不创建本组件。
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GOBULINGAME_API USpriteAnimatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpriteAnimatorComponent();

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetBillboard(USpriteBillboardComponent* InBillboard);

	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void SetClip(FName ClipId);

protected:
	UPROPERTY(Transient)
	TObjectPtr<USpriteBillboardComponent> Billboard;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sprite")
	FName IdleClip = TEXT("Idle");
};
