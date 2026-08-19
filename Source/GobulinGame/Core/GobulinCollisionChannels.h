#pragma once

#include "Engine/EngineTypes.h"

namespace GobulinCollision
{
	/** 专用于近战等战斗形状查询；配置名称必须与 DefaultEngine.ini 的 CombatTrace 一致。 */
	inline constexpr ECollisionChannel CombatTrace = ECC_GameTraceChannel2;

	/** 死亡敌人的专用物体通道；尸体只互相阻挡，并阻挡世界静态/动态物体。 */
	inline constexpr ECollisionChannel EnemyCorpse = ECC_GameTraceChannel3;

	/** 活敌的根碰撞通道；活敌之间由连续怪潮求解器分离，不使用刚体互撞。 */
	inline constexpr ECollisionChannel EnemyBody = ECC_GameTraceChannel4;

	/** 只用于禁止怪潮越界的硬边界；普通世界物体仍可在足够压力下被翻越。 */
	inline constexpr ECollisionChannel EnemySwarmBoundary = ECC_GameTraceChannel5;
}
