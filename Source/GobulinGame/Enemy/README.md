# 敌人运行时地基

敌人公共边界保持后端中立：

- `FEnemySpawnRequest` 用 `FPrimaryAssetId` 指定 Archetype。
- `FCombatantHandle` 是战斗调用方共享的唯一身份。
- `FGobulinEnemyRuntimeData` 不持有 Actor、组件、UObject 或 Mass Entity 指针。
- `UGobulinEnemySubsystem` 拥有生命周期、索敌、接触伤害、受击和回收规则；当前内部保存 Actor 适配记录。
- `AGobulinEnemyActor` 是当前 ACharacter 适配器，只承载胶囊、CharacterMovement、脚底锚点和 Paper2D 表现。
- `AGobulinEnemyAIController` 只执行 NavMesh 移动意图，不拥有索敌、Behavior Tree、Blackboard 或攻击策略。
- `AGobulinEnemySpawnArea` 是可摆放的关卡入口，负责缓存/复核地面候选并提交批量生成请求，不拥有生成后的敌人。
- 已确认的生成、状态、伤害、死亡和退役事实通过 `UCombatEventSubsystem` 发布。

`FEnemySpawnRequest::SpawnTransform` 表示地面接触锚点。Actor 后端按 Archetype 胶囊半高抬高自身，并在生成事件中继续返回实际采用的地面锚点。

敌人 Archetype 资产放在 `/Game/_Game/Enemy`，由 Asset Manager 作为 `EnemyArchetype` 扫描。Paper2D Flipbook 和材质是 `Presentation` Bundle 中的软引用。编辑器专用 `AGobulinEnemyPreviewActor` 可以把胶囊和白名单表现参数烘焙回 Archetype。

基本表现包含相对观察者的四方向 `Idle`、四方向 `Run` 和一个 `Death`。方向按实际水平速度和本地相机选择；低于速度阈值时播放最后有效方向的 Idle，普通击飞/硬直期间固定为面向玩家 Idle。Death 是否循环由 Archetype 配置。存活/死亡分别使用项目自有 Masked/Translucent 材质，材质实例提供共享闪色、峰值覆盖、逐黑和亮度参数，每敌人的闪色/淡出进度继续由 Custom Primitive Data 驱动。

最小行为主线为：

`Inactive -> Spawning -> SeekingTarget -> Moving <-> ReadyToAttack -> Dying -> Inactive`

存活状态可被伤害打断为 `HitReacting` 或 `Staggered`，恢复后回到 `SeekingTarget`。目标选择读取战斗注册表的队伍、存活、位置和身体快照。Actor 导航使用 CharacterMovement 和 RVO。基本敌人只在逻辑胶囊接触且 CombatTrace 没有世界遮挡时提交首次及周期伤害，不拥有额外 Overlap 触发器或攻击计时器。

致死后胶囊改为 `EnemyCorpse`：项目通道默认响应为 Block，尸体自身只阻挡世界和其他尸体并忽略 Pawn/CombatTrace，保证地面双向阻挡同时不妨碍存活单位。未来 Mass 适配器应把 RuntimeStats 放入 Const Shared Fragment，把目标、移动、接触和受击数据拆为每实体 Fragment，并保持同一伤害与事件顺序。
