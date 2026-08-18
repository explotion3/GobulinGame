# UE Project Context

*Last updated: 2026-08-18*

## Engine & Project Overview

**Engine version:** Unreal Engine 5.8（Launcher 构建）  
**Project name:** GobulinGame  
**Description:** 第一人称 2D 纸片风格的魔王城 PvE 游戏；当前优先开发单机基础战斗垂直切片，长期目标包含敌潮、增量成长、基地和合作联机。  
**Project type:** Game  
**Genre / domain:** First-person action / horde survivor / demon-castle defense  
**Target platforms:** Windows（当前目标）；Steam 联机和其他平台暂未进入实现阶段。

**当前开发状态：** M1 基础战斗框架开发中。玩家角色、第一把剑、相机反馈、命中反馈和正式敌人基础框架已经落地；AI、Mass 敌潮、Perk、基地和联机仍在后续路线中。

**文档分层：** 当前设计位于 `docs/`，暂缓系统位于 `docs/future/`，历史规划位于 `docs/archive/`。

## Module Structure

**Runtime module:** `GobulinGame`  
**Editor module:** `GobulinGameEditor`

| Module | Type | Notes |
|---|---|---|
| GobulinGame | Runtime | 玩家、战斗、敌人、纸片表现和未来游戏系统 |
| GobulinGameEditor | Editor | 当前编辑器资产工具和 Commandlet |

**GobulinGame Public dependencies:**

`Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `LevelSequence`, `MovieScene`, `MovieSceneTracks`, `AIModule`, `NavigationSystem`, `StateTreeModule`, `GameplayStateTreeModule`, `GameplayTags`, `CommonUI`, `CommonInput`, `MassCommon`, `MassActors`, `MassSpawner`, `MassSimulation`, `MassLOD`, `MassMovement`, `MassRepresentation`, `MassReplication`, `Niagara`, `UMG`, `Slate`。

## Plugin Dependencies

**Engine plugins enabled in `.uproject`:**

- ModelingToolsEditorMode — 编辑器建模工具。
- StateTree / GameplayStateTree — 后续 AI 和玩法状态树。
- MassGameplay — 后续海量敌潮。
- Niagara — 特效。
- CommonUI — 菜单、交互和复杂 UI。

**Custom plugin:**

- DreamShader — 材质和 DreamShaderLang 工具链；插件自身文档位于 `Plugins/DreamShader/Docs/`。

**Marketplace / Fab plugins:** 无其他已确认依赖。

## Coding Conventions

**Naming prefixes:** 标准 Unreal 前缀（`A/U/F/E/I`），正式类使用 `Gobulin` 命名空间前缀风格。  
**Header style:** `#pragma once`。  
**UObject references:** 优先使用 `UPROPERTY() TObjectPtr<T>`。  
**Source organization:** 当前单模块按 `Core/Combat/Player/Enemy/Horde/Base/Meta/Net/Data/UI/AI` 分目录；暂不拆 Public/Private 模块。  
**Editor-facing comments:** 新增编辑器属性说明使用中文。  
**Blueprint boundary:** C++ 负责玩法逻辑，蓝图负责数据资产、组件配置、表现组合和 GameMode/地图装配。  
**Current rendering convention:** 玩家第一人称武器使用 Plane + 贴图曲线；敌人使用自研 `USpriteFlipbook` + `USpriteBillboardComponent`。

## Gameplay Framework & Systems

**Current gameplay classes:**

- GameMode：`AGobulinGameGameMode` 仍为模板兼容类；当前默认地图仍使用 `BP_FirstPersonGameMode`，正式 GameMode 尚未切换。
- PlayerController：`AGobulinPlayerController`。
- Player Character：`AGobulinPlayerCharacter`。
- Enemy Runtime：`UGobulinEnemySubsystem` 管理生命周期，`AGobulinEnemyActor` 是当前不可直接摆放的最小 Actor 后端。
- Enemy Authoring：`AGobulinEnemySpawnArea` 是当前可摆放的区域生成入口。
- Legacy classes：`AGobulinGameCharacter`、`AGobulinGamePlayerController`、`ADemoEnemy` 仅作为旧模板/过渡代码。

**Current gameplay systems:**

| Class | Type | Responsibility |
|---|---|---|
| `UGobulinHitStopSubsystem` | `UWorldSubsystem` | 有效命中后的全局时间停顿 |
| `UBattleAttributeComponent` | `UActorComponent` | 角色运行时属性和属性变化 |
| `UGobulinSwordCombatComponent` | `UActorComponent` | 剑攻击状态、输入缓存、剑尖命中和伤害派发 |
| `UGobulinWeaponViewComponent` | `UActorComponent` | 第一人称 Plane 武器表现和曲线变换 |
| `UGobulinEnemySubsystem` | `UTickableWorldSubsystem` | 敌人定义加载、批量生成、句柄、状态推进和回收 |
| `AGobulinEnemySpawnArea` | `AActor` | 区域地面采样、候选缓存、批量提交和蓝图结果事件 |
| `AGobulinEnemyActor` | `AActor` | 当前敌人的碰撞、纸片表现和伤害端点 |

**GAS usage:** 未使用。当前采用轻量 `BattleAttributeComponent` + `BattleAttributeSet` + GameplayTags；未来是否局部引入 GAS 按路线图复审。

## Build Configuration

**Build targets:** `GobulinGame`（Game）和 `GobulinGameEditor`（Editor）。当前没有 Server / Client Target。  
**Custom macros / build flags:** 无已确认的项目级自定义宏。  
**Third-party libraries:** 无已确认的外部 C++ 库。  
**Platform notes:** 当前目标 Windows、DX12、SM6；NearClipPlane 为 5.0。  
**Engine modifications:** 无，引擎位于 `E:\UE_5.8`。

**Known template leftovers:**

- `Config/DefaultGame.ini` 仍保留 `First Person Template` 项目名。
- `GlobalDefaultGameMode` 仍指向 `BP_FirstPersonGameMode`。
- 这些内容属于后续编辑器迁移项，不代表新的 C++ 玩家/敌人框架不可用。

## Source Control & Documentation

**Source control:** Git 仓库已建立并用于项目版本管理。  
**Branching strategy:** 尚未固定。  
**Code review:** 尚未固定。  
**Documentation standard:** 设计文档放在 `docs/`；未来和历史文档分目录；代码新增的编辑器注释使用中文；每轮开发完成后同步当前实现、验收步骤和未完成项。
