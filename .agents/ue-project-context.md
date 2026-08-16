# UE Project Context

*Last updated: 2026-08-09*

## Engine & Project Overview

**Engine version:** UE 5.8（Launcher 构建，EngineAssociation 为 GUID `{8A382E35-49F4-59BC-3EB9-9C87C94EF78E}`）
**Project name:** GobulinGame
**Description:** FPS × 类幸存者 × 增量养成 × 基地建设 × 联机合作的魔王主题割草游戏（工作名《魔王城》），当前为规划阶段，尚未开始正式开发。
**Project type:** Game
**Genre / domain:** First-person shooter / horde survivor / incremental / demon-castle defense / co-op
**Target platforms:**
- Windows（首发，Steam）
- 架构上预留 EOS 跨平台接口，但无移动端/主机计划

**规划文档：** `docs/`（README、GDD、TDD、联机方案、路线图、ADR）。后续 UE 任务请先阅读 [docs/README.md](../docs/README.md) 与相关设计文档，再参考本上下文。

## Module Structure

**Primary game module:** GobulinGame（当前唯一模块，由 UE 5.8 First Person 模板生成）

| Module | Type | Notes |
|--------|------|-------|
| GobulinGame | Runtime | 当前全部代码所在；规划按 Core/Combat/Horde/Base/Meta/Net/Data/UI/AI 目录分区，M4 后评估拆分模块 |

**Key dependencies per module:**
- **GobulinGame**: PublicDeps: Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, StateTreeModule, GameplayStateTreeModule, UMG, Slate

**现有模板类（后续将重构/扩展）：**

- `AGobulinGameCharacter`
- `AGobulinGamePlayerController`
- `AGobulinGameGameMode`
- `AGobulinGameCameraManager`

## Plugin Dependencies

**Engine plugins enabled（.uproject）：**

- ModelingToolsEditorMode — 编辑器建模工具（Editor only）
- StateTree — 行为/逻辑状态树
- GameplayStateTree — 游戏玩法状态树集成
- MassGameplay — 千人敌潮（Mass 全家桶）
- Niagara — 特效
- CommonUI — 交互 UI 底层（含 CommonInput）

**规划中需要启用的插件：**

- OnlineSubsystemSteam — PC 联机（M4）

**Marketplace / Fab plugins:** 无（未采购）
**Custom plugins:** 无（暂不创建）

## Coding Conventions

**Naming prefixes:** 标准 UE（U/A/F/E/I）
**Header style:** `#pragma once`
**Log categories in use:** 尚未建立；规划分类：LogGame / LogBattle / LogHorde / LogBase / LogMeta / LogNetwork
**Assertion style:** 尚未确定（建议 check/ensure 按场景混合）
**Header organization:** 当前单模块扁平（`Source/GobulinGame/`）；规划按目录分区，后续拆 Public/Private
**Additional rules:**
- 核心逻辑 C++，蓝图只做内容与 UI 组装
- 数据驱动优先：DataAsset / DataTable / GameplayTags，避免硬编码数值
- 从 M1 起默认服务器权威、客户端预测的写法习惯（联机后置但架构前置）
- 美术方向：3D 低模场景 + 全 2D 纸片角色（含 BOSS），技术落点见 docs/03-技术设计文档.md 第 5.5 节
- UI 分层：交互型 UI 用 CommonUI（菜单/大厅/悬赏/建造/结算），战斗 HUD 用普通 UMG
- Lyra 仅作选择性参考，不作为工程基底（ADR-017）
- 第一人称武器用 2D 贴图层，不做 3D AnimBP 主线（ADR-018）

## Subsystems in Use

**Gameplay framework（当前模板状态）：**

- GameMode: `ABaseGameMode`
- GameState: 未自定义（模板默认；规划为 `ABaseGameState`）
- PlayerController: `AGobulinGamePlayerController`
- Pawn / Character: `AGobulinGameCharacter`
- PlayerState: 未自定义（规划为 `ABasePlayerState`）

**Subsystems（规划）：**

| Class | Type | Responsibility |
|-------|------|----------------|
| USessionManager | UGameInstanceSubsystem | 会话/邀请/玩家档案加载 |
| UHordeDirector | UWorldSubsystem | 勇者潮生成、悬赏、Mass 队列 |
| UResourceManager | UWorldSubsystem | 局内货币/共享预算 |
| UBuildingManager | UWorldSubsystem | 地块、建造/拆除校验 |
| UMetaProfile | ULocalPlayerSubsystem | 玩家永久档案 |
| USaveManager | UGameInstanceSubsystem | 世界/玩家存档、版本迁移 |

**Custom systems:** 规划中（详见 docs/03-技术设计文档.md 与 docs/04-联机方案.md）

**GAS usage:** 未使用。规划为自研轻量属性/Perk 系统，预留 GAS 接口（ADR-006）；敌潮不引入 GAS，M2 可评估玩家侧局部引入。

## Build Configuration

**Build targets:** Game / Editor（当前）；Server / Client 在 M4 联机阶段加入 Target 配置
**Custom macros / build flags:** 无
**Third-party libraries:** 无
**Platform-specific notes:**
- Windows：DX12、SM6（DefaultEngine.ini 已配置）；NearClipPlane=5.0
- Ray tracing / Lumen / Virtual Shadow Maps 已开启（模板默认；最终按性能目标调整）
**Engine modifications:** 无

## Team Context

**Team size:** 未提供（按 2-4 人小团队估算规划）
**Source control:** 未配置（建议尽早确定 Git 或 Perforce 并加入 `.gitignore`/忽略规则）
**Branching strategy:** 未确定
**Code review:** 未确定
**Documentation standards:** 规划文档位于 `docs/`；代码注释与日志标准待 M1 确立

## 状态说明

本文件由 ue-project-context 技能自动起草，内容来自 .uproject、Build.cs、Config 与规划文档。未确认项（编码规范、团队流程等）标注为「未确定」，将在开发过程中更新。
