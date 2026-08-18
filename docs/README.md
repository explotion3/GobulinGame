# Gobulin 项目文档

> 工作名：《魔王城》（暂定）  
> 引擎：Unreal Engine 5.8  
> 最后更新：2026-08-19  
> 当前状态：M1 单机基础垂直切片开发中

## 文档使用规则

当前文档分为三层：

- 根目录：当前仍会直接影响开发的产品、技术和决策文档。
- `future/`：已经确定方向，但暂不作为当前开发阻塞项的系统设计。
- `archive/`：历史规划或已被新实现替代的文档，仅用于追溯。

代码和已验收的编辑器资产是当前实现的事实来源；文档中的“规划”内容不能覆盖已经落地的代码行为。

## 文档地图

| 文档 | 用途 | 当前状态 |
|---|---|---|
| [01-参考调研.md](./01-参考调研.md) | 竞品、类型和差异化研究 | 参考资料 |
| [02-游戏设计文档.md](./02-游戏设计文档.md) | 长期产品目标、核心循环和内容方向 | 目标设计 |
| [03-技术设计文档.md](./03-技术设计文档.md) | 当前技术架构、实现边界和未来扩展 | 持续同步 |
| [05-开发路线图.md](./05-开发路线图.md) | 里程碑、当前阶段目标和验收标准 | M1 进行中 |
| [06-设计决策记录.md](./06-设计决策记录.md) | 已确认的产品与技术决策 | 持续追加 |
| [07-基础内容设计.md](./07-基础内容设计.md) | 内容类型、作用和 ID 规范 | 基础内容设计 |
| [10-敌人系统开发记录.md](./10-敌人系统开发记录.md) | 敌人协议、运行时实现、PIE 接入和后续开发记录 | 持续维护 |
| [11-玩家系统开发记录.md](./11-玩家系统开发记录.md) | 玩家角色、输入、相机和第一把剑的实现与验收 | 持续维护 |
| [future/04-联机方案.md](./future/04-联机方案.md) | M4 联机目标方案 | 暂缓 |
| [future/09-属性Perk技能数据结构.md](./future/09-属性Perk技能数据结构.md) | 属性、Perk 和主动技能的后续设计 | 暂缓 |
| [archive/08-M1设计包-旧版.md](./archive/08-M1设计包-旧版.md) | W01/W03/W05 旧武器体系和旧 M1 数值 | 历史归档 |

## 当前实现摘要

### 玩家

- `AGobulinPlayerCharacter`：正式第一人称玩家角色。
- `AGobulinPlayerController`：本地 Enhanced Input 上下文和相机管理入口。
- `UBattleAttributeComponent`：玩家运行时属性容器。
- `UGobulinCameraFeedbackComponent`：移动镜头反馈、侧倾和上下晃动。
- `UGobulinPlayerStatusWidget`：当前原生 C++ 战斗反馈界面，显示生命、受伤红闪、死亡与重开入口；正式 UI 可用其蓝图子类替换。

### 第一把武器：剑

- `UGobulinSwordCombatComponent`：攻击状态、输入缓存、剑尖轨迹命中和伤害派发。
- `UGobulinWeaponViewComponent`：相机挂载的 2D Plane 武器表现。
- `UGobulinSwordFeedbackComponent`：挥剑/命中音效、相机震动和 Hit Stop。
- `UGobulinSwordDefinition`：剑的贴图、材质、曲线、命中和反馈参数。
- 当前武器使用 Plane + 贴图变换曲线，不使用旧 Flipbook 武器体系。

### 正式敌人基础框架

- `UGobulinEnemySubsystem`：World 级敌人生命周期服务，统一处理定义加载、生成、状态推进和回收。
- `AGobulinEnemyActor`：当前最小 Actor 后端，提供纸片表现、碰撞和伤害端点，不直接摆放到关卡。
- `AGobulinEnemySpawnArea`：可摆放的区域生成入口，提供地面采样、候选缓存、批量请求和蓝图结果事件。
- `UGobulinEnemyArchetype`：Actor 与未来 Mass 后端共用的数据定义。
- 句柄、伤害、生成、状态和事件已通过独立协议解耦，详见[敌人系统开发记录](./10-敌人系统开发记录.md)。

## 当前开发顺序

1. 完成 SpawnArea 的 PIE 批量生成和战斗人工验收。
2. 验收最小敌人的目标选择、移动、四方向表现、周期接触伤害和玩家临时状态 UI。
3. 完成单机战斗垂直切片，再进入 Perk、悬赏和掉落等局内系统。
4. 联机、Mass 敌潮和基地系统暂按 `future/` 与路线图规划推进。

项目工程上下文见 [.agents/ue-project-context.md](../.agents/ue-project-context.md)。
