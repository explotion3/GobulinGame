---
type: production-guide
scope: wacom-dreamshader
status: active
updated: 2026-07-16
tags:
  - wacom/materials
  - dreamshader
  - unreal
---

# DreamShader 生产实践与排错指南

> [!info] 本文职责
> 本文记录 Wacom 项目实际使用 DreamShader 1.4.1 制作 UI、卡牌与 Niagara 材质时验证过的工作流、常见陷阱和长期维护约定。具体卡牌表现合同仍以 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md) 和 [UI_Battle_WBP_Binding.md](./UI_Battle_WBP_Binding.md) 为准；插件兼容债见 [TechDebt.md](./TechDebt.md)。

## 1. 真源与职责划分

DreamShader 材质应拆成四层，各自只保存一种事实：

| 层 | 位置 | 负责内容 |
|---|---|---|
| Material source | `DShader/Material/**/*.dsm` | 材质 Domain、Blend Mode、纹理与参数声明、Graph 接线、生成资产路径 |
| Shared helper | `DShader/Shared/*.dsh` | 可复用的 UV、Mask、像素图案、合成和数学函数 |
| Material Instance | `/Game/DreamMaterials/**/MI_*` | 颜色、亮度、线宽、深度、密度等审美调参 |
| Runtime / Style | C++、WBP、DataAsset | 语义进度、卡牌身份、持续时间、音效、资源选择和生命周期 |

`.dsm`、`.dsh` 和对应设置脚本是可编辑的长期制作真源。`/Game/DreamMaterials` 的全部生成资产同时以 Git LFS 纳入版本控制，保证普通 checkout 后可以直接解析 Map / WBP / DataAsset 的引用。生成 `.uasset` 不取代 `.dsm/.dsh`，也不得只在材质图中保存无法回写的制作修改。

参数归属原则：

- 颜色、箔片强度、像素密度、描边宽度、金属对比度等视觉参数放 Material Instance。
- `Amount`、`Time`、`Tilt`、`Seed`、`EffectKind` 等运行时状态由 C++ 写入 MID。
- 总时长、音效、选择哪套材质等表现编排可以放 Style DataAsset。
- 不要在 DataAsset、MI 和 C++ 默认值中复制同一组颜色或算法参数。
- 不要在生成材质图中手工修改无法回写到 `.dsm` 的节点；下次 `-Force` 会覆盖这些修改。

## 2. 推荐目录与命名

```text
DShader/
  Material/
    Card/M_FirstPersonCard_Fake3D.dsm
    UI/M_Wacom_SimplePulse_UI.dsm
    World/M_WacomBattleEnemyPartImpactPixel.dsm
  Shared/
    WacomFirstPersonCardSurface.dsh
    WacomCardSurfaceParallax.dsh
  Texture/
    Card/...
Scripts/
  Setup<Feature>Assets.py
```

约定：

- Wacom 自有 helper 使用 `Wacom` 前缀，降低与插件样例或第三方函数重名的风险。
- `.dsm` 的 `Shader(Name="DreamMaterials/<Category>/<Name>")` 决定生成资产路径。移动源文件时不要顺手改变 `Name`，否则会生成第二份资产并破坏现有引用。
- 通用数学拆到 `.dsh`；材质的纹理、参数、Domain 和最终 Outputs 留在 `.dsm`。
- 一个 setup 脚本只处理一项功能的 MI、纹理导入、Style 和默认引用。不要用全量脚本覆盖已经由美术手调的其它资产。

## 3. 三类常用材质合同

### 3.1 RetainerBox UI 效果

```dshader
TextureSampleParameter2D Texture = Path(Engine, "/EngineResources/DefaultTexture") [
    SamplerType="Color";
    SamplerSource="FromTextureAsset";
];

Settings = {
    Domain = "UI";
    ShadingModel = "Unlit";
    BlendMode = "PremultipliedAlpha";
    TwoSided = true;
}
```

必须同时满足：

- Retainer 输入参数名精确为 `Texture`，并与 RetainerBox 的 Texture Parameter 配置一致。
- DreamShader 的 `PremultipliedAlpha` 会生成 Unreal 的 `AlphaComposite`；不要误改成普通 Translucent。
- 输出使用 `Base.EmissiveColor` 与 `Base.Opacity`。
- 裁切或消散时 RGB 与 Alpha 必须一起处理。只清 Alpha、保留黑色 RGB，容易产生黑块、暗边或只剩阴影的假象。
- 光效、消散前沿和残片通常不应写入接触阴影 caster；阴影应读取仍存在的原始卡面 Alpha。

### 3.2 普通 UMG Image 材质

仍使用 `UI + Unlit + PremultipliedAlpha`，但没有 Retainer 注入的 `Texture` 合同。卡面核心复合材质可以声明 `ArtTexture / FrameTexture / RarityTexture`，由每个 Widget 的独立 MID 写入。

不要让多个卡牌 Widget 共享同一个可变 MID，否则一张牌更新插画、稀有度或倾斜时会污染其他卡牌。

费用与 EffectBadge 数字一类局部反馈应优先直接绑定数字 `UImage`，而不是为了一个小区域接管整张卡的 Retainer。PaperSprite 数字先通过 App-private Atlas 工具读取 `AtlasTexture / StartUV / SizeUV`，再把旧、新纹理和 UV Rect 写给每个数字自己的 MID；播放结束必须恢复权威 PaperSprite Brush、authored RenderTransform 与 Pivot。多位数只有在旧、新位数一致且每位 Sprite 都能解析时才播放，失败应直接刷新正式值，不能留下空白或半套 MID。

短时 Pressed、Commit 或规则拒绝不应默认制作成整卡 UI 材质。first-person card layer 已删除历史 `M_FirstPersonCard_FeedbackEdge` 及其 `InteractionFeedbackImage` 宿主：实体按压和提交脉冲由 Motion Mixer/Card Depth 表达，拒绝四角标记由 Slate Paint 直接绘制。只有确实需要采样卡面、改变 Surface Alpha/RGB 或复用复杂像素算法时，才为交互语义增加 DreamShader 材质；不要为了颜色 tint 或几条硬边恢复常驻 Overlay/MID。

### 3.3 Niagara Sprite 材质

- 使用 `Surface / Unlit / Translucent / TwoSided`。
- 在 `.dsm` 的 `Settings` 中持久声明 `bUsedWithNiagaraSprites = true`。不要只在 Material Editor 中手动勾选 `Used With Niagara Sprites`；DreamShader 下次重新生成父材质时会按真源覆盖未声明的 Usage。
- Dynamic Material Parameter 四通道应先在 Niagara 中写入 `Particles.DynamicMaterialParameter`，再在 Sprite Renderer 的动态材质绑定中绑定同一变量。
- Wacom 当前约定常用 `X=ShapeKind`、`Y=NormalizedAge`、`Z=PaletteVariant`、`W=Semantic/Decorative`；具体效果以对应领域文档为准。
- Material Instance 的球体预览不能代表 Sprite 最终效果。应在 Niagara 预览和 PIE 中检查朝向、尺寸、透明度、Fixed Bounds 与摄像机遮挡。

推荐的 Niagara 父材质设置：

```dshader
Settings = {
    Domain = "Surface";
    ShadingModel = "Unlit";
    BlendMode = "Translucent";
    TwoSided = true;
    bUsedWithNiagaraSprites = true;
}
```

`Setup<Feature>Assets.py` 可以再次读取并验证该标记，作为旧生成资产的防御性修复；但脚本不能替代 `.dsm` 真源声明，否则编辑器自动生成后问题会复发。

### 3.4 Paper2D Sprite 轮廓材质

敌人部位交互描边使用 `Surface / Unlit / Translucent / TwoSided`，真源为 `DShader/Material/World/M_WacomBattleEnemyPartInteractionOutline.dsm` 与 `DShader/Shared/WacomBattleEnemyPartInteractionOutline.dsh`，生成资产固定为 `/Game/DreamMaterials/World/M_WacomBattleEnemyPartInteractionOutline`。

- Paper2D 运行时代理依赖参数名 `SpriteTexture` 注入当前 Sprite 图集；代理是带最多两像素透明留白的 Plane，不复制或缩放原 Sprite Component。C++ 从 `BakedRenderData` 解出 `OutlineAtlasUVOrigin / AxisX / AxisY`，并写入 `OutlineSourceInvPixelSize / OutlineCanvasToSourceScale`；不能把 Sprite 当作独占整张纹理，也不能用包含 collision bounds 的 Component bounds 反推视觉矩形。
- 上述 UV 参数都是 Vector Parameter，参与 UV 算术前必须用显式 `ComponentMask(OutputType="float2", R=true, G=true)`。直接把默认 `float3` 输出与 `float2` 方向相乘会在 PCD3D_SM6 报 `Arithmetic between types float3 and float2 are undefined`，并让运行时显示默认灰色材质。
- 每个 Sprite sample 都先把 source-local UV clamp 到半像素安全区，再乘独立 inside mask，防止代理留白夹取 Atlas 相邻区域；随后显式用 `ComponentMask(OutputType="float1", A=true)` 读取 Alpha。`.dsh` helper 分别计算 `Dilated1 - CenterMask` 与 `Dilated2 - Dilated1`：一像素可选态只使用内环，两像素悬停态叠加较暗外环，材质不得输出中心主体。
- `Scripts/SetupBattleEnemyPartInteractionAssets.py` 只把已生成父材质接到 Target Preview Style，不再创建第二套手工 MaterialExpression 图。NullRHI 的 `RecompileMaterial()` 不能替代一次带 `-AllowCommandletRendering` 的 PCD3D_SM6 定向生成与日志审计。

## 4. `.dsh` 的使用技巧

复杂算法优先写成 `Function SelfContained`：

```dshader
Function SelfContained WacomExample_ComputeMask(
    in float2 uv,
    in float amount,
    out float mask)
{
    float2 centered = uv - float2(0.5, 0.5);
    mask = step(length(centered), saturate(amount));
}
```

实践要点：

- helper 只处理数值，不直接拥有项目资产路径或具体 Style。
- 输出数量保持少而明确；多个相关标量可打包为 `float3/float4`，但消费端 ComponentMask 必须与真实维度一致。
- 像素效果使用局部 UV、源像素尺寸倒数和稳定 Seed；不要使用逐帧随机值，否则帧率变化会导致图案游动或闪烁。
- 只有明确需要持续动画时才读取 Time。一次性效果优先由 Playback 写入归一化进度和确定性 Seed。
- 重复的 3×3 Alpha 邻域、Bayer 阈值或 UV 投影只计算一次并复用，避免每增加一个表现模块就复制整组纹理采样。
- 对图集 Sprite：先在局部 `0..1` UV 中做位移、inside mask 和裁切，再映射到 atlas scale/bias。直接在图集 UV 上视差会采样到相邻稀有度边框。
- 卡面 `BackColor` 一类插画底板不能以 Alpha=1 的全屏颜色直接参与最终合成，否则会把透明圆角重新填成矩形，并污染 Retainer 实时 Alpha 阴影。应先通过局部 UV 生成居中缩放遮罩（当前默认 `BackColorScale=0.96`），再作为插画下层合成。
- 纯色 `BackColor` 没有可识别纹理，不应随 Tilt 平移。需要无深度图层次时，额外采样一次透明插画 Alpha，使用偏移 Alpha 乘 `(1-ArtAlpha)` 与 `BackColorAlpha` 生成硬像素接触投影，再按 `BackColor → 投影 → Art` 合成。投影不能写入最终卡体 Alpha，也不能成为 Retainer 外部接触阴影 caster。
- 像素插画视差必须同时设置最大源像素位移和半像素采样安全边界：先把目标 offset 限制到 `MaxArtParallaxPixels`，再把位移后的局部 UV Clamp 到 `0.5*InvSize .. 1-0.5*InvSize`。只做 inside mask 不能阻止过滤器在纹理边缘外取样。
- 卡面材质中的“层深”和“表面反光”是两个独立合同：Art、Frame、Rarity 都可以拥有不同 UV 深度，并分别使用标量开关控制角度反光。当前默认 `ArtReflectionEnabled=0 / FrameReflectionEnabled=1 / RarityReflectionEnabled=0`；反光关闭时必须精确恢复源 RGB，但不能顺带关闭该层 UV 视差。插画具备宽幅柔和覆膜高光能力但默认关闭，实体 Frame 默认使用方向金属高光，Rarity 的 foil / iridescence 能力保留并由内容按主题显式开启。
- DreamShader Graph 区不支持的 HLSL 写法应移入 `Function SelfContained`。例如 `step()` 可以在 `.dsh` helper 内使用，但某些插件版本不能在 Graph assignment 中直接解析；复合赋值 `value += expression` 也可能把左值误判为 Graph 变量类型。生产源使用显式 `value = value + expression`，并把 mode 选择、阈值分支放进 helper，再由 Graph 只接收输出。

## 5. 已经踩过的坑

### 5.1 Masks sampler 与默认纹理不匹配

典型错误：

```text
Sampler type is Masks, should be Color for /Engine/EngineResources/DefaultTexture
```

原因是参数声明为 `SamplerType="Masks"`，默认资产却是 Color 类型的 Engine DefaultTexture。即使运行时稍后会写入正确纹理，材质初次编译仍会失败。

正确做法：

- Masks 参数的默认 Path 直接指向已经按 Masks 导入的项目纹理。
- 纹理设置使用 `Compression=Masks`、`sRGB=false`、`Filter=Nearest`；像素 UI 通常再使用 `NoMipmaps + UI LOD Group`。
- Color 参数才使用 Engine DefaultTexture 作为安全默认值。

### 5.2 ComponentMask 默认通道与维度错误

项目使用的 DreamShader 1.4.1 曾出现 `UE.Expression(ComponentMask)` 新节点保留 Unreal 默认 R/G，再叠加 DSL 指定通道的问题，导致期望的单通道 B/A 变成 RGB 或 RGA。项目插件已做本地修复：创建节点后先清空默认通道，再应用 DSL 参数。

仍需遵守：

- 明确写 `OutputType="float1/float2/float3"` 和 R/G/B/A 开关。
- 不要从 `float3` 读取 A；这会产生 `Not enough components ... for component mask 0001`。
- Named Reroute 显示的是上游类型，人工检查生成图时要看最终消费端 `FExpressionInput` 的 mask。
- 更新 DreamShader 插件后必须重新验证本地 ComponentMask 补丁并强制生成关键材质。

### 5.3 Retainer 首帧 MID 生命周期

嵌套 Retainer 的运行时 MID 可能晚于 `NativeConstruct` 创建。把首帧空 MID 缓存成“已经初始化”会造成 Hover/Drag 倾斜永久不更新，或切换 Surface Effect 后无法恢复。

正确做法：

- 分别缓存 WBP 创作源 Effect Material 和当前 Slate 运行时 MID。
- 切换效果时只切一次源材质，再重新获取 Retainer 实际 MID。
- `SetEffectMaterial()` 后不能立即把 `GetEffectMaterial()==nullptr` 当成永久失败；先写入进度 0 View、请求 Retainer Render，并在后续 Tick 重试取得实际 MID。
- 材质源有效、MID 已取得且参数已写入仍不等于“可起播”。必须等待拥有该请求 Generation 的 Widget 完成一次真实 Slate Paint；Ready 首 Tick 使用零 Delta，避免编译/PSO 抖动把 authored 时间直接推进到中段。
- `CostDigitImage` / EffectBadge 这类直接 `UImage` 材质不经过 Retainer，但同样要在安装 MID Brush 后等待一次 Paint。结束时恢复权威 PaperSprite Brush；MID 可以按材质源/最大位数缓存到 Widget teardown，不要每次反馈重新创建。
- Reset、ForceComplete、Slot 复用和 Destruct 都恢复创作源并清零运行时参数。
- 不要长期持有另一个 Widget 的动态材质强引用。

### 5.4 消散后只剩阴影或黑色矩形

常见原因：

- 材质裁切了卡面 Alpha，却仍用未裁切的矩形或旧 Alpha 生成接触阴影。
- 外部 `CardShadowImage` 与 Retainer 内实时 Alpha 阴影同时存在。
- 输出不符合预乘 Alpha，透明区域 RGB 污染背景。

当前约定：

- `Fake3DSurfaceRetainer` 的实时 Alpha 接触阴影是卡牌唯一阴影来源。
- caster visibility 必须乘入当前消散可见度；残片和发光不参与 caster。
- 倾斜阴影不要只依赖固定的 Base/Lift Offset。当前卡牌合同由 C++ 写入 `ContactShadowTiltOffsetXUV / YUV`，DreamShader 使用 `WacomFirstPersonCard_ResolveContactShadowOffset` 把它与 authored lift offset 相加；所有会临时接管同一 Retainer 的 Surface 材质都必须声明并消费这两个参数，否则材质切换时阴影会跳回卡框下方。
- 阴影软硬和方向正确但整体偏淡时，优先使用运行时 `ContactShadowOpacityMultiplier`；该参数由 Anchor 统一写入基础与临时 Surface 材质，不要分别提高多个 `.dsm` 的 Base/Lift Opacity。
- 材质无法绘制到 Retainer 捕获范围之外。当前 `WBP_FPCardView` 使用 `456 x 520` 捕获面包住居中的 `360 x 424` 卡面内容，四边各留 `48 px`；只设置 `Clip To Bounds - Without Intersecting` 不能弥补尺寸不足，也不要通过拉伸内部卡面来获得余量。
- 先验证 `Texture.A`、inside mask、caster mask，再调阴影颜色和软硬度。

### 5.5 生成成功不等于资产接线完成

DreamShader commandlet 只负责根据 `.dsm/.dsh` 生成基础 Material。它不会自动：

- 创建或更新 Material Instance。
- 导入纹理并修正 Compression / sRGB / Filter。
- 填 Style DataAsset。
- 把资产引用写入 WBP、Actor 或 Anchor。

因此每项正式效果都应有幂等 `Scripts/Setup<Feature>Assets.py`。脚本应：

- 缺失必要输入时明确失败。
- 只更新自己拥有的资产和字段。
- 保存被修改资产。
- 不重建或覆盖无关 MI/DA 的人工参数。

### 5.6 `-AllowCommandletRendering` 的全局错误会误导定位

允许命令行渲染后，Editor 会加载并编译其它已引用材质。当前项目的 `M_CardSurface_CosmicFoil` 与 `M_CosmicBlob` 仍缓存失效的 `/DreamShaderGenerated/*.ush` 路径，因此命令整体可能返回失败，即使本次目标材质已经生成且没有自身 SM6 错误。

排错时必须：

1. 搜索目标材质名对应的 `Missing cached shadermap`、`Error` 和 `Generated` 行。
2. 区分目标材质错误与其它资产的全局启动错误。
3. 不能仅凭进程 Exit Code 判断目标材质失败。
4. 旧 include 问题应单独强制重生成并保存对应旧材质，不要通过修改新材质绕过。

### 5.7 DreamMaterials 的双层真源

`Content/DreamMaterials` 整体受 Git 管理，当前全部 `.uasset` 由 Git LFS 存储。新 worktree 应直接取得完整目录，不再建立本地依赖 Junction，也不再依靠每个 worktree 单独生成实验资产才能打开工程。

提交前应确认至少包含：

- `.dsm`
- 被 import 的 `.dsh`
- 必要的确定性源纹理
- setup 脚本
- C++ 参数合同与领域文档
- 正式运行时引用的生成 Material / MI `.uasset`

修改 `.dsm/.dsh` 后必须强制重生成并同时提交对应 `.uasset`；新增生成资产也必须一并提交。不得只交付源码却留下旧运行时资产，也不得只提交本机生成的 `.uasset` 而遗漏制作真源。

### 5.8 材质预览不等于真实宿主

- UI 材质在球体预览中可能只显示灰色或透明，Retainer 输入也不会自动存在。
- Niagara 材质第一帧预览不代表 Burst 条件、User Parameter 或 Dynamic Material Parameter 正确。
- 像素卡面最终还会经过 UMG 缩放、DPI、Retainer、Nearest 过滤和 authored clipping。

审美与空间表现必须在真实 WBP、Niagara System 和 PIE 中验收。

### 5.9 编辑器启动与跨 worktree 元数据

生成资产的 `DreamShader.SourceFile` 必须保存为工程相对路径，例如 `DShader/Material/Card/M_FirstPersonCard_Fake3D.dsm`。不得把 main、Codex worktree 或开发者机器的绝对路径写入受 Git LFS 管理的 `.uasset`；插件读取旧绝对路径时会按当前 `DShader` 根目录解析其稳定后缀，保证同一源文件换 worktree 后仍能命中 Source Hash 跳过逻辑。

普通持久化材质模式下，`bAutoCompileOnEditorStartup` 默认关闭。Editor 启动只重建轻量的 import dependency graph，不排队、加载或保存全部生成 Material；修改 `.dsm/.dsh` 时仍由 `bAutoCompileOnSave` 触发定向编译，也可以通过 `Tools > Recompile DSM` 显式全量编译。Virtual Material Mode 需要在内存中恢复 transient 资产，因此不受该启动开关限制。

`Content/DreamMaterials` 继续整体由 Git LFS 管理。禁止用忽略生成资产来掩盖启动重写；升级元数据格式时应在隔离 worktree 执行一次受控 `DreamShader compile -All -Force`，提交迁移后的父 Material / Material Function，再在两个不同绝对路径的 worktree 验证普通启动不产生资产 diff。

## 6. 推荐生成流程

在项目根目录执行：

```powershell
$ProjectDir = (Get-Location).Path
$EditorCmd = 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Source = Join-Path $ProjectDir 'DShader\Material\Card\M_WacomCardSurfaceComposite.dsm'

& $EditorCmd (Join-Path $ProjectDir 'Wacom.uproject') `
  -run=DreamShader compile `
  "-Source=$Source" `
  -Force -Unattended -NoPause -NoSplash
```

需要验证 SM6 时增加 `-AllowCommandletRendering`，随后检查 `Saved/Logs/Wacom.log`：

```powershell
rg -n -C 3 "M_WacomCardSurfaceComposite|LogShaderCompilers: Error|LogMaterial: Error|LogShaders: Error|LogDreamShader: Error" Saved/Logs/Wacom.log
```

基础 Material 成功后再运行该功能的 setup 脚本：

```powershell
& $EditorCmd (Join-Path $ProjectDir 'Wacom.uproject') `
  "-ExecutePythonScript=$ProjectDir\Scripts\SetupCardSurfacePerspectiveAssets.py" `
  -Unattended -NoPause -NoSplash
```

该脚本必须写入实际运行时使用的宿主，而不只是名称相近的通用资产。当前第一人称链是 `WBP_FPCardView -> WBP_FirstPersonCardView`，因此脚本会同时配置 `WBP_FirstPersonCardView` 与通用 `WBP_CardView`。如果只有出血装饰位移、核心插画/实体卡框/稀有度边框完全静止，优先检查实际嵌套 CardView 的 `CardSurfaceMaterial`，不要继续放大 Parallax Strength。

如果脚本会修改 WBP、Blueprint CDO、DataAsset 或纹理，运行前关闭 Unreal Editor，避免编辑器内存中的旧资产在退出时覆盖脚本结果。

## 7. 参数设计技巧

- 参数名是运行时合同，命名后不要随意改。C++ 的 `FName`、DreamShader 参数和 MI 必须一致。
- 参数按 `Group / SortPriority / Description` 分类；Description 使用中文说明单位、效果和建议区间。
- Runtime 参数与 MI 调参分组，避免美术误改 `Amount / Time / Seed`。
- 对像素效果同时暴露逻辑密度和实际源像素尺寸；只写固定 UV 宽度会随分辨率和 DPI 改变视觉粗细。
- 使用 `SurfaceInvSize` 或 `CardSourceInvSize` 把像素位移换成 UV，不在 C++ 中复制材质内部尺寸假设。
- Seed 必须来自稳定语义（Card ID、Event Sequence、目标 ID），不能来自当前帧时间。
- Reduced Motion 不等于关闭语义反馈：保留静态标记、中心方印或短淡出，只关闭方向传播、粒子位移和循环动画。
- 卡牌插画与稀有度边框不是同一种资产合同：插画使用 `UTexture2D`；`RarityBorder` 使用 `UPaperSprite`，必须从 baked atlas texture 与 source rect 计算局部 UV，不能直接把 Sprite 当成整张 Texture 采样。
- 核心卡面把 `RarityBorder` 贴在实体 Frame 的同一 UV 平面：位置直接复用 `frameUVAndMask`，箔片反光仍独立计算。不要用重新增加 `RarityDepthPixels` 的方式强化稀有度，否则倾斜时会与卡框发生可见错位。
- 嵌入式插画窗口应从真实 `FrameTexture.a` 推导内沿，而不是按矩形 UV 画假边框。当前 helper 使用四方向 Nearest Alpha 采样生成静态内阴影、背光侧加深和受光侧窄高光；它只调整 BackColor+Art 合成色，不得改写最终卡体 Alpha。
- 插画 Alpha 投影与整张卡牌的 Retainer 接触阴影职责不同：前者只表达卡框内部 Art/BackColor 的层差，必须裁在 BackColor 内；后者从完整实时卡体 Alpha 投到场景背景。不要把插画投影合入 caster，否则会出现内部光效继续投影自身的双重阴影。
- `CardIllustrationDepthMap` 是可选的五级灰度 Mask：黑色更深、白色更靠近 Frame，中灰为基准。材质必须用 `ArtDepthEnabled` 显式关闭缺失深度图的局部变化，并把最终深度限制在 Frame 后方；不能依赖一张默认 Noise 恰好呈现为平面。推荐导入为 `Masks / sRGB=false / Nearest / NoMipmaps / UI`。具体卡图可用 Image2 参考原始透明插画生成初稿，但必须保持同构图、同尺寸、硬像素轮廓，并在导入前量化为有限灰度级。
- 稳定 Bayer、`step / smoothstep`、hash、色板选择等算法优先写进带 `Wacom` 前缀的 `.dsh` `Function SelfContained`。`.dsm` Graph 只负责参数、采样、helper 调用和最终合成：这样既避免在 Graph 中重复展开分支节点，也能绕开 Graph 表达式对部分 HLSL 风格函数调用的解析限制。费用数字重组的硬像素顺序、Tone 色板选择和 Preview/Rewrite 模式判断分别由 `WacomCard_ComputeCostDigitRewriteMasks`、`WacomCard_SelectCostDigitRewritePalette` 与 `WacomCard_SelectCostDigitEffectMode` 提供。`step()` 可在 `.dsh` 自包含函数中使用，但不能直接写在当前 DreamShader Graph 赋值表达式里；后者会报 `Unknown Graph function 'step'` 并导致运行时只能直接换数字。
- 对 `UImage` 内的 PaperSprite 做双值过渡时，优先用 `UPaperSprite::GetSlateAtlasData()` 提取 Atlas Texture、StartUV 与 SizeUV，然后把两组 Atlas Rect 交给直接 UI 材质；不要假设 Sprite 独占整张纹理。为了避免新值先闪一帧，Layer 必须在 ViewData 更新前锁定旧 Sprite，再刷新权威数据并启动 MID。此类局部 Image MID 不需要 Retainer 的 `Texture` 参数，也不应复制 Fake3D、接触阴影或卡面几何换算。
- 当临时 Surface Effect 要把另一张纹理映射到卡牌主体、同时保留实时出血装饰轮廓时，不能直接用整张 Retainer UV 采样图案。运行时应从 `CardContentSizeBox` 的本地布局尺寸解析 Retainer 局部 `BodyRectMin/Max`，将该矩形重映射为 `0–1` 牌背 UV；矩形外仍使用实时 `Texture.a` 作为完整剪影，并以 MI 的统一边缘色填充。纹理资产必须先导入，再生成引用它作为默认值的 `.dsm`；推荐 setup 脚本提供 import-only 阶段，随后定向 DreamShader compile，最后创建 MI/Style 与宿主引用。这样既不会把牌背图案拉伸到 EffectBadge，也不会让缺少默认纹理导致生成节点回退为错误采样器。
- Retainer 捕获 UV 不等于卡体 UV。需要贴合卡面的临时效果必须复用实时卡面采样所使用的 Fake3D `ProjectedUV`，再通过居中的 `CardBodyRectMin/Max` 映射到未 Clamp 的 `CardLocalUV`；Preview、Commit、像素网格和稳定 hash 都应使用这套卡体空间。Body Rect 只允许由 Retainer 与 `CardContentSizeBox` 的本地布局尺寸计算，不能通过 `LocalToAbsolute / AbsoluteToLocal` 读取已经混入手牌扇形旋转或 RenderTransform 的几何。效果若只属于卡体，最终 mask 还应乘以 `CardBodyMask × Texture.a`，避免 Retainer bleed 和出血装饰被误当成卡面。
- 正面结晶一类 Retainer Surface Effect 应把“卡体已组装可见度”和“外溢装饰像素”拆成两条 mask：前者同时裁切实时 `Texture.rgb/a` 并作为接触阴影 caster，后者只合成发光颜色/Alpha，绝不能写回 shadow caster。稳定结晶顺序使用 Card ID Seed、局部量化 UV 和 `.dsh` 自包含 hash；不要使用材质 `Time`、Noise Texture 或逐帧随机。稀有度色只在完成边缘阶段选择，不应预先改染完整卡面。
- `M_FirstPersonCard_SurfaceEffects_GainReveal.dsm` 的生成资产、默认 MI 与 Style 由 `Scripts/SetupFirstPersonCardGainRevealAssets.py` 定向维护；设置 `WACOM_SKIP_GAIN_REVEAL_ANCHOR=1` 可在保留其它分支玩家 Blueprint 的情况下只重建材质与 Style。脚本不得调用其它 DreamShader 全量资产设置流程。
- 长时间 Held 的封存刻印仍应使用临时 Surface MID，而不是把算法塞进普通手牌基础材质。`M_FirstPersonCard_SurfaceEffects_RetainSeal.dsm` 保留原始卡面 RGB/Alpha，并只把角标、外缘与中心方印作为额外预乘颜色合成；这些亮纹不能写回接触阴影 caster。`RetainSealPhase / Progress` 由 Playback 写入，Held 使用静态强度，不读取 `Time`。默认 MI、Style 与 Anchor 由 `Scripts/SetupFirstPersonCardRetainSealAssets.py` 定向维护；设置 `WACOM_SKIP_RETAIN_SEAL_ANCHOR=1` 可跳过玩家 Blueprint 保存。
- Battle 玩家状态条的 `M_WacomBattle_PlayerVitals.dsm` 是直接绑定 `UImage` 的 UI 材质，不是 Retainer Effect。权威 HP、延迟伤害、Action Preview、护盾可见度和短反馈量由 `UPlayerStatusBar` 每次刷新/活动 Tick 显式写入；材质不读取 `Time`，低血也不循环闪烁。默认 MI 只由 `Scripts/SetupBattlePlayerVitalsAssets.py` 定向维护，WBP/HUD 布局则由 `WacomBuildPlayerStatusUI -BuildVitalsV2` 独立构建，两个工具不得互相保存对方负责的资产。
- `M_WacomBattleCardPileSelectionOutline.dsm` 是牌堆浏览条目下方的直接 UI 材质，不属于卡面 Retainer。`SelectionAmount / SelectionReducedMotion / SelectionSeed` 由虚拟化 Entry 写入；只有 Hover、焦点或点击锁定的条目才创建 MID，离开且未锁定或条目回收时立即释放。流光允许读取材质 `Time`，因为它是明确的低强度循环装饰；Simplified Motion 必须把流动固定为稳定静态亮边。父材质由 DreamShader 定向生成，默认 MI 只由 `Scripts/SetupBattleCardPileSelectionOutlineAssets.py` 更新，WBP/Style 则由 `WacomBuildBattlePileDetailsUI` 管理。

## 8. 性能原则

- 普通手牌常驻材质只保留 Fake3D、必要接触阴影和已确认的核心表面能力。
- 昂贵的消散、刻印或更新效果只在活动 outgoing/target Slot 上临时切换 MID。
- 不要为了一个短效果增加第二个 Retainer。
- 纹理邻域采样集中在 shared helper 中复用；先评估是否能复用已有 Alpha 结果。
- 不启用的效果不要仅靠 `Amount=0` 长期留在生产材质里承担采样成本。
- 多个小 Sprite/牌印优先批量 Slate CustomVerts 或 Niagara Burst，不创建逐粒子 UWidget。

## 9. 验证边界

自动化测试适合保护稳定合同：

- `.dsm` 的 Domain、Blend Mode、参数名和必要 import。
- Retainer 参数名是否为 `Texture`。
- 材质源是否意外读取 Time/Noise。
- C++ 是否把正确的语义进度、Tilt、Seed 写入视图合同。
- Reset、ForceComplete、Widget 复用是否恢复基础材质。
- 纹理导入设置和 MI/Style 引用是否有效。

PIE 负责判断审美与手感：

- 颜色、亮度、线宽、像素密度、拖尾长度。
- Hover/Drag 是否自然，DPI 下像素是否稳定。
- 卡面透明边缘、实体出血装饰和接触阴影是否正确。
- 明暗背景、16:9/21:9、连续快速触发和低帧率表现。

测试材质合同不是为了替代材质实例预览，而是防止参数改名、Domain 变化、Retainer 合同破坏、错误采样器和清理失败等无法靠一次人工观察稳定发现的回归。

## 10. 新材质提交检查表

- [ ] `.dsm` 生成路径与预期 `/Game/DreamMaterials/<Category>` 一致。
- [ ] 可复用算法已放入带 `Wacom` 前缀的 `.dsh`。
- [ ] UI/Retainer/Niagara Domain 与 Blend Mode 正确。
- [ ] Retainer 采样参数精确命名为 `Texture`。
- [ ] Color/Masks sampler 与默认纹理资产类型一致。
- [ ] ComponentMask 的输入维度和通道开关一致。
- [ ] 透明区域同时正确处理 RGB 与 Alpha。
- [ ] 图集先做局部 UV 裁切，再映射 atlas。
- [ ] Runtime、MI 和 Style 参数没有重复所有权。
- [ ] setup 脚本幂等且不会覆盖无关人工资产。
- [ ] Reset、ForceComplete、Slot 复用和 teardown 能恢复基础材质。
- [ ] DreamShader 强制生成成功，目标材质无自身 SM6 错误。
- [ ] 相关 C++ 编译和合同测试通过。
- [ ] 在真实 WBP/Niagara/PIE 中完成视觉验收。
