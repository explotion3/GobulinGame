#pragma once

#include "Engine/EngineTypes.h"

namespace GobulinCollision
{
	/** 专用于近战等战斗形状查询；配置名称必须与 DefaultEngine.ini 的 CombatTrace 一致。 */
	inline constexpr ECollisionChannel CombatTrace = ECC_GameTraceChannel2;
}

