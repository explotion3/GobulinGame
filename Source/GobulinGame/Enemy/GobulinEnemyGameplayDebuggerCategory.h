#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"

/** Gameplay Debugger 中所选正式敌人的运行时诊断页。 */
class FGobulinEnemyGameplayDebuggerCategory final : public FGameplayDebuggerCategory
{
public:
	FGobulinEnemyGameplayDebuggerCategory();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
};

#endif
