#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GobulinEnemySwarmBoundary.generated.h"

class UBoxComponent;

/**
 * 关卡中明确禁止活敌跨越的硬边界。玩家与普通场景物体会忽略它；默认高度足以覆盖常规怪潮。
 */
UCLASS(Blueprintable, ClassGroup = (Enemy), meta = (DisplayName = "Gobulin Enemy Swarm Boundary"))
class GOBULINGAME_API AGobulinEnemySwarmBoundary : public AActor
{
	GENERATED_BODY()

public:
	AGobulinEnemySwarmBoundary();

	UFUNCTION(BlueprintPure, Category = "Enemy|Crowd")
	UBoxComponent* GetBoundaryBox() const { return BoundaryBox; }

private:
	/** 边界盒需要从可行走区域下方延伸到怪潮技术上限以上，避免敌人从上下绕过。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Crowd", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BoundaryBox;
};
