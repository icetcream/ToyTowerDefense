# AGENTS.md

本文件给后续接手 `ToyTowerDefense` 的 Agent 使用，用来快速理解项目结构、开发约束和验证方式。

## 项目概况

`ToyTowerDefense` 是 UE5 C++ 塔防项目。当前阶段主要完成：

- 开始界面、主界面、研究所界面。
- 研究所图鉴。
- 玩具盒制作队列。
- 存档和离线制作进度补算。
- 自定义 `UTTDDeveloperSettings` 配置入口。
- `GameplayTag -> DataTable` 数据表映射结构。
- Lyra `GameplayMessageRouter` 插件接入，作为项目消息系统。
- 基于 `UWorldSubsystem` 的全局对象池，按 Class 管理实现池化接口的对象。

详细业务说明见：

- `docs/需求整理.md`
- `docs/方案设计.md`
- `docs/项目使用与设计逻辑.md`

## 关键入口

| 文件 | 作用 |
| --- | --- |
| `Config/DefaultEngine.ini` | 默认地图和默认 GameMode |
| `Config/DefaultGame.ini` | `UTTDDeveloperSettings` 配置 |
| `ToyTowerDefense.uproject` | 模块和插件启用配置 |
| `Source/ToyTowerDefense/Private/TTDGameMode.cpp` | 设置默认 `ATTDMenuPlayerController` |
| `Source/ToyTowerDefense/Private/TTDMenuPlayerController.cpp` | 菜单 UI 切换 |
| `Source/ToyTowerDefense/Private/TTDResearchSubsystem.cpp` | 研究所核心业务 |
| `Source/ToyTowerDefense/Private/UI/TTDResearchLabWidget.cpp` | 研究所 UI |
| `Source/ToyTowerDefense/Public/TTDTypes.h` | 研究所数据结构和 DataTable Row |
| `Source/ToyTowerDefense/Public/TTDDeveloperSettings.h` | 项目自定义配置入口 |
| `Source/ToyTowerDefense/Public/TTDGameplayMessages.h` | 消息 payload 和 channel 声明 |
| `Source/ToyTowerDefense/Public/TTDObjectPoolWorldSubsystem.h` | 全局对象池入口 |
| `Source/ToyTowerDefense/Public/TTDPooledObjectInterface.h` | 池化对象接口 |

## 当前架构

玩家流程：

`开始界面 -> 主界面 -> 研究所 -> 图鉴 / 研发`

核心运行链路：

1. `ATTDGameMode` 指定 `ATTDMenuPlayerController`。
2. `ATTDMenuPlayerController::BeginPlay` 显示开始界面。
3. UI Widget 负责菜单跳转和玩家交互。
4. `UTTDResearchSubsystem` 负责研究所数据、存档、制作队列和消息广播。
5. `FTTDCraftingQueueModel` 负责纯 C++ 队列逻辑，便于测试。
6. `UTTDSaveGame` 保存解锁内容、制作队列和上次保存时间。
7. `UGameplayMessageSubsystem` 将研究所变化通知给 UI 和后续系统。
8. `UTTDObjectPoolWorldSubsystem` 按 Class 复用实现 `UTTDPooledObjectInterface` 的 Actor/UObject。

## 配置规则

用户希望“所有需要配置的内容放在自定义 DeveloperSettings 里填写”，并采用 `GameplayTag -> DataTable` 格式。

新增玩法配置时优先放入：

- `UTTDDeveloperSettings`
- `Config/DefaultGame.ini`
- DataTable，并通过 `GameplayTagDataTables` 映射

当前 DataTable tag：

| GameplayTag | Row Struct |
| --- | --- |
| `TTD.DataTable.Research.Parts` | `FTTDPartDefinition` |
| `TTD.DataTable.Research.Diagrams` | `FTTDDiagramDefinition` |
| `TTD.DataTable.Research.ToyBoxes` | `FTTDToyBoxDefinition` |
| `TTD.DataTable.ObjectPool.Definitions` | `FTTDObjectPoolDefinition` |

避免继续把玩法数值、默认 ID、资源路径、UI 文案等写死在 C++ 中。若必须保留 fallback，请清楚标注其用途，并优先让正式运行路径来自配置。

对象池使用规则：

- 通过 `GetWorld()->GetSubsystem<UTTDObjectPoolWorldSubsystem>()` 获取池。
- 业务侧按 Class 获取对象，不按 GameplayTag 获取。
- 进入池的对象必须实现 `UTTDPooledObjectInterface`。
- 未配置但实现接口的 Class 会临时创建，不进入池统计；未实现接口的 Class 会被拒绝。
- 已配置 Class 达到 `MaxSize` 后不会继续创建池对象。

## 消息系统

项目使用 `Plugins/GameplayMessageRouter` 作为消息系统。一般不要修改插件源码，除非用户明确要求。

当前项目消息定义在：

- `Source/ToyTowerDefense/Public/TTDGameplayMessages.h`
- `Source/ToyTowerDefense/Private/TTDGameplayMessages.cpp`

当前 channel：

| Channel | 用途 |
| --- | --- |
| `TTD.Message.Research.Queue.Changed` | 队列变化或制作项完成 |
| `TTD.Message.Research.Queue.Enqueued` | 玩具盒加入制作队列 |
| `TTD.Message.Research.Queue.Claimed` | 完成玩具盒被领取 |
| `TTD.Message.Research.Collection.Changed` | 图鉴/零件解锁状态变化 |

新增跨模块通知时，优先新增 GameplayTag channel 和明确的 payload USTRUCT。

## 编码约束

- 保持改动范围小，不做无关重构。
- 不要回滚用户已有改动。
- 手工编辑文件时使用 `apply_patch`。
- 搜索文件或文本优先用 `rg`。
- UE C++ 修改后优先跑 Editor build。
- 涉及研究所队列、设置、消息时，补充或更新自动化测试。
- 插件 `Plugins/GameplayMessageRouter` 是外部 Lyra 插件副本，默认只读对待。

## 构建与测试

常用构建命令：

```powershell
E:\UE\SourceEngine\Engine\Build\BatchFiles\Build.bat ToyTowerDefenseEditor Win64 Development -Project=E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -WaitMutex -NoHotReloadFromIDE
```

常用自动化测试命令：

```powershell
E:\UE\SourceEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -unattended -nop4 -nosplash -NullRHI -NoSound -ExecCmds="Automation RunTests ToyTowerDefense; Quit" -TestExit="Automation Test Queue Empty"
```

当前测试：

- `ToyTowerDefense.Research.CraftingQueueModel`
- `ToyTowerDefense.Settings.DeveloperSettings`
- `ToyTowerDefense.Messaging.GameplayMessageRouter`
- `ToyTowerDefense.ObjectPool.WorldSubsystem`

## 已知待推进点

- `DefaultGame.ini` 当前尚未配置实际 DataTable 资产路径。
- `UTTDResearchSubsystem::SeedDefinitions` 仍有 C++ 内置样例数据 fallback。
- 自动保存间隔、默认选中玩具盒、UI 刷新间隔、图鉴列数仍待配置化。
- UI 文案和样式仍主要写在 C++ Widget 中，后续可迁到 StringTable、DataTable 或 Widget Blueprint。
- 图纸研发、仓库、关卡挑战仍是占位入口。
- 对象池配置表结构已接入，但项目内容侧还没有实际 DataTable 资产。

## 推荐阅读顺序

1. `AGENTS.md`
2. `docs/项目使用与设计逻辑.md`
3. `docs/方案设计.md`
4. `Source/ToyTowerDefense/Private/TTDResearchSubsystem.cpp`
5. `Source/ToyTowerDefense/Private/UI/TTDResearchLabWidget.cpp`
6. `Source/ToyTowerDefense/Public/TTDDeveloperSettings.h`
7. `Source/ToyTowerDefense/Public/TTDGameplayMessages.h`
8. `Source/ToyTowerDefense/Public/TTDObjectPoolWorldSubsystem.h`

## Battle System Notes

The first playable battle loop is implemented around `UTTDBattleWorldSubsystem`.

Key runtime files:

| File | Purpose |
| --- | --- |
| `Source/ToyTowerDefense/Public/TTDBattleTypes.h` | Battle DataTable row structs and runtime stat/modifier types |
| `Source/ToyTowerDefense/Public/TTDBattleWorldSubsystem.h` | World-level battle entry point and runtime state |
| `Source/ToyTowerDefense/Public/Battle/TTDBattleCastleActor.h` | Castle target to defend |
| `Source/ToyTowerDefense/Public/Battle/TTDBattleEnemyActor.h` | Poolable enemy actor, targets nearest friendly target |
| `Source/ToyTowerDefense/Public/Battle/TTDBattleBuildingActor.h` | Buildable friendly target and attacker |
| `Source/ToyTowerDefense/Public/Battle/TTDBattleProjectileActor.h` | Poolable projectile for projectile attack buildings |
| `Source/ToyTowerDefense/Public/Battle/TTDBattleSpawnPointActor.h` | Placed enemy spawn point with `SpawnGroup` |
| `Source/ToyTowerDefense/Public/Battle/TTDBuildSlotActor.h` | Placed build slot with `SlotId`, grid coordinate, height level, and height effect id |
| `Source/ToyTowerDefense/Public/UI/TTDBattleHUDWidget.h` | Prototype C++ battle HUD |

Battle DataTable tags:

| GameplayTag | Row Struct |
| --- | --- |
| `TTD.DataTable.Battle.Levels` | `FTTDBattleLevelDefinition` |
| `TTD.DataTable.Battle.Waves` | `FTTDWaveDefinition` |
| `TTD.DataTable.Battle.Enemies` | `FTTDEnemyDefinition` |
| `TTD.DataTable.Battle.Buildings` | `FTTDBuildingDefinition` |
| `TTD.DataTable.Battle.ToyBoxRewards` | `FTTDToyBoxRewardDefinition` |
| `TTD.DataTable.Battle.HeightEffects` | `FTTDBattleHeightEffectDefinition` |

Battle flow:

`Main Menu -> ShowBattleLevel -> UTTDBattleWorldSubsystem::StartDefaultBattle -> spawn castle -> schedule waves -> spawn enemies -> build on placed slots -> victory/defeat`

Important behavior:

- Progress is `1 - (UnspawnedWeight + AliveEnemyWeight) / TotalLevelEnemyWeight`.
- Diagram ids are build permission and are not consumed.
- Part costs are consumed when building placement succeeds.
- Height never blocks building. Height effects are optional configured modifiers applied to building runtime stats.
- Enemy/building/projectile actors use the object pool subsystem when possible; unconfigured poolable classes are temporary objects per object-pool rules.
- Prototype C++ HUD supports selecting a building and clicking a build slot. `ATTDMenuPlayerController` also exposes `BeginDragBattleBuilding` and `DropSelectedBattleBuildingOnSlot` for Blueprint drag/drop UI.

Battle messages:

| Channel | Purpose |
| --- | --- |
| `TTD.Message.Battle.State.Changed` | Battle state changes |
| `TTD.Message.Battle.Progress.Changed` | Progress/remaining weight changes |
| `TTD.Message.Battle.Currency.Changed` | Currency changes |
| `TTD.Message.Battle.Inventory.Changed` | Diagram, part, and toy box inventory changes |
| `TTD.Message.Battle.Building.Placed` | Building placement succeeds |
| `TTD.Message.Battle.Ended` | Battle reaches victory or defeat |

Current battle test:

- `ToyTowerDefense.Battle.WorldSubsystem`
