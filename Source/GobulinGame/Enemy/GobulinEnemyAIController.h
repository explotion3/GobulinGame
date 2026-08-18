#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Enemy/EnemyNavigationTypes.h"
#include "GobulinEnemyAIController.generated.h"

/**
 * Actor 后端的路径执行器。它不做索敌或玩法决策，只执行 EnemySubsystem 给出的移动意图。
 */
UCLASS(NotBlueprintable)
class GOBULINGAME_API AGobulinEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGobulinEnemyAIController();

	EEnemyMoveStatus RequestMoveToTarget(AActor* TargetActor, const FEnemyMoveIntent& Intent);
	void StopEnemyMove();

protected:
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

private:
	int32 ActiveIntentSequence = 0;
	bool bHasActiveIntent = false;
	bool bSuppressMoveCompletion = false;
};
