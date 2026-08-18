#pragma once

#include "Engine/EngineTypes.h"

namespace GobulinCollision
{
	/** 专用于近战等战斗形状查询；配置名称必须与 DefaultEngine.ini 的 CombatTrace 一致。 */
	inline constexpr ECollisionChannel CombatTrace = ECC_GameTraceChannel2;

	/** 死亡敌人的专用物体通道；尸体只互相阻挡，并阻挡世界静态/动态物体。 */
	inline constexpr ECollisionChannel EnemyCorpse = ECC_GameTraceChannel3;
}
