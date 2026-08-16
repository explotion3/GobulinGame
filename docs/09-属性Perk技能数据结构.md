# 属性 / Perk / 技能数据结构设计

> 版本：v0.1（数据结构稿）  
> 日期：2026-08-09  
> 范围：自研轻量属性系统（ADR-006）的数据结构，不含剧情命名；代码实现以本表为准。

## 1. 属性系统

### 1.1 设计原则

- 属性用 **GameplayTags 作为 Key**（如 `Attribute.Health.Max`），开放可扩展。
- 最终值 = `Clamp((Base + ΣAdd) × ΣMult)`；乘法型属性默认 Base = 1.0，加法型属性默认 Base = 0 或实际值。
- 所有数值来源（Perk/Mod/技能/Buff/设施/悬赏）统一走修改器，不散落在各系统代码里。
- 属性变化广播事件，HUD/表现层订阅，不直接轮询。

### 1.2 属性清单（首发，`DT_BattleAttributes`）

| 属性 Tag | 类型 | 默认值 | 下限 | 上限 | 说明 |
|---|---|---:|---:|---:|---|
| Attribute.Health.Max | 加法 | 100 | 10 | 9999 | 最大生命 |
| Attribute.Health.Current | 加法 | 100 | 0 | 9999 | 当前生命（运行时） |
| Attribute.Armor.Flat | 加法 | 0 | 0 | 50 | 护甲（减伤换算） |
| Attribute.Mana.Max | 加法 | 100 | 10 | 9999 | 最大魔力 |
| Attribute.Mana.RegenPerSec | 加法 | 2 | 0 | 100 | 每秒魔力回复 |
| Attribute.Movement.Speed | 加法 | 500 | 100 | 2000 | 移动速度 cm/s |
| Attribute.Movement.Stamina | 加法 | 100 | 0 | 200 | 体力 |
| Attribute.Damage.Multiplier | 乘法 | 1.0 | 0.1 | 10 | 全武器伤害倍率 |
| Attribute.Crit.Chance | 加法 | 0.05 | 0 | 1 | 暴击率 |
| Attribute.Crit.Multiplier | 乘法 | 2.0 | 1.0 | 10 | 暴击伤害倍率 |
| Attribute.Piercing.Count | 加法 | 0 | 0 | 10 | 穿透目标数 |
| Attribute.FireRate.Multiplier | 乘法 | 1.0 | 0.2 | 5 | 射速倍率 |
| Attribute.ReloadSpeed.Multiplier | 乘法 | 1.0 | 0.2 | 5 | 换弹速度倍率 |
| Attribute.MagSize.Bonus | 加法 | 0 | 0 | 50 | 弹匣容量加成 |
| Attribute.AOE.RangeMultiplier | 乘法 | 1.0 | 0.5 | 3 | AOE 范围倍率 |
| Attribute.Loot.Multiplier | 乘法 | 1.0 | 0.1 | 10 | 战利品倍率 |
| Attribute.Exp.Multiplier | 乘法 | 1.0 | 0.1 | 10 | 经验倍率 |
| Attribute.BaseDefense.TurretDamageMultiplier | 乘法 | 1.0 | 0.1 | 10 | 炮塔伤害倍率 |
| Attribute.BaseDefense.RepairSpeedMultiplier | 乘法 | 1.0 | 0.1 | 10 | 修复速度倍率 |
| Attribute.Minion.StrengthMultiplier | 乘法 | 1.0 | 0.1 | 10 | 魔王军强度倍率 |
| Attribute.Bounty.ChargeMultiplier | 乘法 | 1.0 | 0.1 | 10 | 悬赏充能速度倍率 |
| Attribute.Bounty.RageDurationMultiplier | 乘法 | 1.0 | 0.1 | 10 | 狂潮时长倍率 |

### 1.3 修改器结构 `FAttributeModifier`

| 字段 | 类型 | 说明 |
|---|---|---|
| ModifierId | FName | 唯一 ID（可重复用于同来源多效果） |
| AttributeTag | FGameplayTag | 目标属性 |
| Operation | 枚举 | Add / Multiply / Override |
| Value | float | 数值 |
| Duration | 枚举 | Permanent / Timed（秒）/ UntilDeath |
| RemainingTime | float | Timed 时剩余时间 |
| StackRule | 枚举 | Single / Stackable / Refresh |
| MaxStacks | int32 | Stackable 上限 |
| CurrentStacks | int32 | 当前层数 |
| SourceId | FName | 来源（PerkId / SkillId / ModId / FacilityId） |
| SourceTags | FGameplayTagContainer | 来源标签（用于条件与驱散） |

### 1.4 结算顺序

1. 取 Base 值；
2. 应用全部 Add 修改器（可叠加，受 StackRule 限制）；
3. 应用全部 Multiply 修改器；
4. Override 若存在则直接覆盖；
5. 按属性表 Clamp；
6. 广播 `OnAttributeChanged`。

### 1.5 数据结构对应类

| 类 | 职责 |
|---|---|
| `UBattleAttributeSet`（UObject） | 持有 BaseValues（TMap<FGameplayTag,float>）与修改器列表，提供 GetFinalValue / AddModifier / RemoveModifier / RemoveAllFromSource |
| `UBattleAttributeDefaults`（UDataAsset） | 属性默认值与 Clamp 范围（读 `DT_BattleAttributes`） |
| `UBattleAttributeComponent`（ActorComponent） | 挂在玩家/单位上，运行时属性容器与事件广播 |

## 2. Perk 数据结构

### 2.1 Perk 定义 `UPerkDefinition`（UPrimaryDataAsset）

| 字段 | 类型 | 说明 |
|---|---|---|
| PerkId | FName | P01-P40 |
| Category | EPerkCategory | 兵刃/体魄/掠夺/城防/悬赏 |
| PerkType | 枚举 | Passive（M1 只做被动） |
| MaxLevel | int32 | 1 或 3 |
| Rarity | 枚举 | Common / Rare / Epic |
| Weight | float | 三选一出现权重 |
| Prerequisites | TArray<FName> | 前置 PerkId |
| RequiredTags | FGameplayTagContainer | 需要属性标签（如某武器类型） |
| PerLevelEffects | TArray<FAttributeModifier> | 每级效果（等级 N 应用前 N 组） |
| GrantedTags | FGameplayTagContainer | 获得后授予的标签 |

### 2.2 Perk 五系分配（P01-P40 初表）

#### 兵刃系 P01-P08

| ID | 名称（机制名） | 每级效果 | 最大级 | 权重 |
|---|---|---|---|---|
| P01 | 换弹速度 | ReloadSpeed ×1.1 | 3 | 100 |
| P02 | 暴击率 | Crit.Chance +3% | 3 | 100 |
| P03 | 穿透 | Piercing.Count +1 | 1 | 60 |
| P04 | 弹匣扩容 | MagSize.Bonus +2 | 3 | 80 |
| P05 | 后座控制 | Recoil（新增手部参数） -5% | 3 | 80 |
| P06 | 射速 | FireRate ×1.04 | 3 | 90 |
| P07 | 爆头强化 | Crit.Multiplier ×1.15 | 2 | 70 |
| P08 | 武器伤害 | Damage.Multiplier ×1.08 | 3 | 100 |

#### 体魄系 P09-P16

| ID | 机制名 | 每级效果 | 最大级 | 权重 |
|---|---|---|---|---|
| P09 | 生命强化 | Health.Max +20 | 3 | 100 |
| P10 | 护甲 | Armor.Flat +2 | 3 | 80 |
| P11 | 冲刺强化 | Stamina.Max +20 | 2 | 70 |
| P12 | 吸血 | 击杀回复生命（2%） | 3 | 60 |
| P13 | 魔力上限 | Mana.Max +15 | 3 | 90 |
| P14 | 魔力回复 | Mana.Regen +1 | 3 | 80 |
| P15 | 移速 | Movement.Speed +3% | 2 | 60 |
| P16 | 减伤 | 受击伤害 ×0.95 | 3 | 80 |

#### 掠夺系 P17-P24

| ID | 机制名 | 每级效果 | 最大级 | 权重 |
|---|---|---|---|---|
| P17 | 金币嗅觉 | Loot ×1.1 | 3 | 100 |
| P18 | 经验渴望 | Exp ×1.08 | 3 | 90 |
| P19 | 掉落率 | 掉落概率 +5% | 3 | 70 |
| P20 | 图纸碎片 | 碎片掉落 +10% | 2 | 50 |
| P21 | 击杀回魔 | 击杀回 2 魔力 | 3 | 90 |
| P22 | 击杀回血 | 击杀回 1 生命 | 3 | 70 |
| P23 | 悬赏投手 | 魔核充能 ×1.1 | 3 | 60 |
| P24 | 拾取范围 | 拾取半径 +30% | 2 | 60 |

#### 城防系 P25-P32

| ID | 机制名 | 每级效果 | 最大级 | 权重 |
|---|---|---|---|---|
| P25 | 炮塔强化 | TurretDamage ×1.1 | 3 | 80 |
| P26 | 快速修复 | RepairSpeed ×1.15 | 3 | 80 |
| P27 | 魔王军强化 | Minion.Strength ×1.1 | 3 | 80 |
| P28 | 兵种扩容 | 魔王军上限 +1 | 2 | 60 |
| P29 | 建造预算 | 临时防御预算 +20% | 2 | 60 |
| P30 | 陷阱强化 | 陷阱伤害 ×1.15 | 3 | 60 |
| P31 | 城墙耐久 | 城墙耐久 +10% | 2 | 70 |
| P32 | 炮塔过载 | 过载时长 +20% | 2 | 50 |

#### 悬赏系 P33-P40

| ID | 机制名 | 每级效果 | 最大级 | 权重 |
|---|---|---|---|---|
| P33 | 充能加速 | Bounty.Charge ×1.1 | 3 | 80 |
| P34 | 狂潮延长 | RageDuration ×1.15 | 3 | 70 |
| P35 | 悬赏收益 | 悬赏掉落 +10% | 3 | 90 |
| P36 | 精英吸引 | 精英出现概率 +5% | 2 | 50 |
| P37 | 追加冷却 | 追加悬赏冷却 -20% | 2 | 60 |
| P38 | 魔核扩容 | 魔核上限 +1 | 2 | 50 |
| P39 | 狂潮火力 | 狂潮期间伤害 ×1.1 | 3 | 70 |
| P40 | 谨慎守城 | 低风险时修复免费（原型） | 1 | 30 |

### 2.3 三选一规则

- 候选池 = 未满级 + 前置满足 + RequiredTags 满足的 Perk；
- 按 Weight 加权随机抽 3 个，不重复；
- 同一系连续 3 次未出现时，下一次强制出现该系一个候选（保底，M2 平衡期验证）；
- 升级时 Perk 已满级则不进入候选池。

## 3. 主动技（技能）数据结构

### 3.1 技能定义 `USkillDefinition`（UPrimaryDataAsset）

| 字段 | 类型 | 说明 |
|---|---|---|
| SkillId | FName | SK_ 前缀 |
| Category | EPerkCategory | 归属系 |
| TargetType | 枚举 | Self / Directional / Point / Enemy / Area |
| CastType | 枚举 | Instant / Channel / Projectile |
| ManaCost | float | 魔力消耗（0 = 无） |
| Cooldown | float | 冷却秒数 |
| CastTime | float | 施法时间（0 = 瞬发） |
| Range | float | 有效距离 |
| Radius | float | 作用半径 |
| Effects | TArray<FAttributeModifier> | 属性效果 |
| Damage | FDamageSpec | 伤害规格（数值/倍率/暴击可用） |
| SpawnAction | FName | 生成物（投射物/区域/召唤）引用 ID |
| LinkedTitles | TArray<FName> | 默认携带该技能的称号 |
| MaxLevel | int32 | 1-3 |
| PerLevelUpgrade | FSkillUpgrade | 每级升级（伤害/CD/范围） |

### 3.2 技能执行流程

```
输入触发 → 校验（冷却/魔力/标签/目标）→ 扣除消耗
  → 施法前摇（可被打断）→ 生成效果/投射物/区域
  → 应用伤害与修改器 → 广播技能事件 → 进入冷却
```

### 3.3 称号默认技能组（M1 实例）

| 称号 | 技能 | SkillId | 消耗 | CD | 效果 |
|---|---|---|---|---|---|
| S01 | 冲锋 | SK_M1 | 体力 20 | 3s | 突进 8m、撞飞沿途 |
| S01 | 格挡 | SK_M2 | 0 | 6s | 0.5s 窗口减伤 80% |
| S02 | 火雨 | SK_M3 | 魔力 30 | 8s | 6m 半径 8 发 ×15 |
| S02 | 定身 | SK_M4 | 魔力 15 | 12s | 3m 半径减速 50% 2s |

## 4. 与其他系统的关系

### 4.1 事件

- 属性变化、Perk 获得、技能释放统一走 `UGameEventBus`；
- HUD/表现层订阅事件更新显示，不直接读属性容器轮询。

### 4.2 网络

- 属性与修改器服务器权威；客户端只读取最终值用于表现；
- 技能释放走服务器校验（冷却/消耗/目标），客户端预测表现；
- 修改器 SourceId 需要服务器校验（不能凭空出现 P01）。

### 4.3 存档

- 局内 Perk/技能/临时 Buff **不存档**；
- 永久解锁（图纸、科技、称号解锁）按现有玩家档案字段存；
- 属性默认值来自 `DT_BattleAttributes`，存档不存属性本身。

## 5. 待补内容（不阻塞本数据结构）

- Recoil 属性目前未列入属性表，作为武器手感参数先行，后续再决定是否属性化；
- 技能 SpawnAction 引用表（投射物/区域/召唤物 ID → 行为）在 M2 与武器系统一起设计；
- 减伤换算公式（Armor → 减伤%）在数值平衡阶段定。
