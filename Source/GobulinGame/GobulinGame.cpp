// Copyright Epic Games, Inc. All Rights Reserved.

#include "GobulinGame.h"
#include "Modules/ModuleManager.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "Enemy/GobulinEnemyGameplayDebuggerCategory.h"
#include "GameplayDebugger.h"
#endif

class FGobulinGameModule final : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

#if WITH_GAMEPLAY_DEBUGGER
		IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
		GameplayDebugger.RegisterCategory(
			TEXT("GobulinEnemy"),
			IGameplayDebugger::FOnGetCategory::CreateStatic(
				&FGobulinEnemyGameplayDebuggerCategory::MakeInstance),
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
		GameplayDebugger.NotifyCategoriesChanged();
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& GameplayDebugger = IGameplayDebugger::Get();
			GameplayDebugger.UnregisterCategory(TEXT("GobulinEnemy"));
			GameplayDebugger.NotifyCategoriesChanged();
		}
#endif

		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FGobulinGameModule, GobulinGame, "GobulinGame");
