# Editor 内最小手动验证指南

本文档用于在 UE Editor 里手动验证当前 `ToyTowerDefense` 已有功能是否可用。目标不是制作正式关卡，而是用最少内容跑通：

- 开始界面和主界面。
- 研究所图鉴、玩具盒制作队列、领取零件、离线进度。
- 关卡挑战入口。
- 战斗启动校验。
- 守护城堡战斗闭环。
- BuildSlot 建造。
- 高度效果器。
- 关卡内玩具盒购买和开启。
- 对象池和 `ConfigurationError` 负向验证。

## 0. 前置确认

### 0.1 构建项目

在 PowerShell 执行：

```powershell
E:\UE\SourceEngine\Engine\Build\BatchFiles\Build.bat ToyTowerDefenseEditor Win64 Development -Project=E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -WaitMutex -NoHotReloadFromIDE
```

预期：

- 编译成功。
- Editor 可以正常打开项目。

### 0.2 确认默认 GameMode

项目当前已在 `Config/DefaultEngine.ini` 配置：

```ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Engine/Maps/Templates/OpenWorld
GlobalDefaultGameMode=/Script/ToyTowerDefense.TTDGameMode
```

`ATTDGameMode` 会指定 `ATTDMenuPlayerController`。这个 PlayerController 会自动显示开始界面，所以最小验证不需要手动创建 UI Widget Blueprint。

如果你在 Editor 里打开了其他测试地图，请确认：

1. `World Settings -> GameMode Override` 为空，使用项目默认 GameMode。
2. 或者显式设置为 `TTDGameMode`。

## 1. 最小 UI 验证

直接 Play 当前地图。

### 1.1 开始界面

预期：

- 进入游戏后显示开始界面。
- 点击开始按钮进入主界面。

当前 UI 是 C++ 动态创建的 Widget：

- `UTTDStartMenuWidget`
- `UTTDMainMenuWidget`
- `UTTDResearchLabWidget`
- `UTTDBattleHUDWidget`

因此不需要配置 Widget Blueprint。

### 1.2 主界面

主界面有三个入口：

| 入口 | 当前行为 |
| --- | --- |
| 仓库 | 目前是占位，会在主界面状态文本显示占位提示 |
| 关卡挑战 | 调用 `StartDefaultBattle`，成功后进入 Battle HUD，失败时显示失败原因 |
| 研究所 | 进入研究所界面 |

先点击“仓库”。

预期：

- 不切换界面。
- 状态文本显示仓库尚未接入资源背包。

再点击“研究所”。

预期：

- 进入研究所界面。

最后先不要急着点“关卡挑战”。如果还没有配置战斗 DataTable 和场景 Actor，它会显示启动失败原因，这是正常的 P0 校验结果。

## 2. 研究所功能验证

研究所当前可以在没有正式 DataTable 资产时使用 C++ fallback 样例数据，因此是最容易先验证的功能。

### 2.1 默认解锁配置

当前 `Config/DefaultGame.ini` 中已有：

```ini
[/Script/ToyTowerDefense.TTDDeveloperSettings]
CraftingQueueMaxSlots=5
SaveSlotName=ToyTowerDefenseResearch
DefaultBattleLevelId=PrototypeBattle
+DefaultUnlockedDiagramIds=JokerTower
+DefaultUnlockedDiagramIds=BatteryBlaster
+DefaultUnlockedToyBoxIds=BasicToyBox
+DefaultUnlockedToyBoxIds=MechanicToyBox
+DefaultUnlockedToyBoxIds=EnergyToyBox
+DefaultUnlockedPartIds=Gear
+DefaultUnlockedPartIds=Spring
```

第一次进入研究所时，会基于这些默认解锁内容创建存档。

### 2.2 研发页

操作：

1. 主界面点击“研究所”。
2. 默认进入“研发”页。
3. 右侧选择一个已解锁玩具盒，例如 `BasicToyBox`。
4. 点击“加入制作队列”。

预期：

- 制作队列出现一个条目。
- 条目显示剩余时间。
- 队列最大数量来自 `CraftingQueueMaxSlots`，默认是 5。
- 如果队列满了，再加入会失败并显示失败原因。

### 2.3 领取完成玩具盒

操作：

1. 等待制作时间归零。
2. 完成后队列按钮会变为可点击。
3. 点击完成的队列项。

预期：

- 队列项被移除。
- 解锁该玩具盒可能产出的零件。
- 如果零件已经解锁，会显示没有新增零件或类似反馈。
- 图鉴里的零件相关内容会更新。

### 2.4 离线进度

操作：

1. 加入一个制作时间较长的玩具盒。
2. 关闭 PIE。
3. 等一段时间后重新 Play。
4. 进入研究所。

预期：

- 制作队列会根据上次保存时间补算离线进度。
- 达到时间的条目会显示完成。

### 2.5 图鉴页

操作：

1. 在研究所左侧点击“图鉴”。
2. 切换“图纸”和“玩具盒”分类。
3. 点击已解锁条目。

预期：

- 已解锁条目可点击。
- 未解锁条目不可点击或显示未解锁。
- 点击已解锁条目后，下方显示描述和关联零件。

### 2.6 图纸研发页

操作：

1. 在研究所点击“研发”。
2. 点击“图纸”页签。

预期：

- 当前显示占位说明。
- 图纸合成、零件消耗、解锁规则还未接入。

## 3. 战斗最小资产配置

战斗不使用 fallback 样例资产。要让“关卡挑战”真的启动，需要在 Editor 中创建最小 DataTable，并在地图中摆放 SpawnPoint 和 BuildSlot。

建议创建目录：

```text
Content/Data/ManualValidation
```

下面的命名只用于最小验证，可以按你的项目规范改名。

## 4. 创建 Battle DataTable

在 Content Browser 中右键：

```text
Miscellaneous -> Data Table
```

分别创建以下 DataTable。

| 资产名建议 | Row Struct |
| --- | --- |
| `DT_BattleLevels_Manual` | `FTTDBattleLevelDefinition` |
| `DT_BattleWaves_Manual` | `FTTDWaveDefinition` |
| `DT_BattleEnemies_Manual` | `FTTDEnemyDefinition` |
| `DT_BattleBuildings_Manual` | `FTTDBuildingDefinition` |
| `DT_BattleToyBoxRewards_Manual` | `FTTDToyBoxRewardDefinition` |
| `DT_BattleHeightEffects_Manual` | `FTTDBattleHeightEffectDefinition` |
| `DT_ObjectPool_Manual` | `FTTDObjectPoolDefinition` |

`DT_ObjectPool_Manual` 不是启动战斗的必需项。未配置对象池时，池化类会临时创建并参与战斗。要验证对象池和容量上限，再配置它。

### 4.1 HeightEffects

DataTable：`DT_BattleHeightEffects_Manual`

新增 Row：

| RowName | HeightEffectId | Modifiers |
| --- | --- | --- |
| `HighGround` | `HighGround` | 见下方 |

Modifiers 添加三项：

| Attribute | Operation | Value |
| --- | --- | --- |
| `Attack Damage` | `Add` | `5.0` |
| `Attack Range` | `Multiply` | `2.0` |
| `Attack Interval` | `Override` | `0.25` |

验证点：

- 高度不会限制建造。
- 只有配置了 `HeightEffectId` 的 BuildSlot 会改变建筑运行时属性。

### 4.2 Enemies

DataTable：`DT_BattleEnemies_Manual`

新增 Row：

| 字段 | 值 |
| --- | --- |
| RowName | `BasicEnemy` |
| EnemyId | `BasicEnemy` |
| EnemyClass | `ATTDBattleEnemyActor` |
| MaxHealth | `10.0` |
| MoveSpeed | `0.0` |
| AttackDamage | `0.0` |
| AttackRange | `100.0` |
| AttackInterval | `1.0` |
| ProgressWeight | `2.0` |
| CurrencyDrop | `3` |

说明：

- `MoveSpeed = 0.0` 是为了最小验证时敌人不乱跑，方便观察建筑攻击。
- 正式关卡可以改成正常速度。

### 4.3 Waves

DataTable：`DT_BattleWaves_Manual`

新增 Row：

| 字段 | 值 |
| --- | --- |
| RowName | `WaveA` |
| WaveId | `WaveA` |
| DelayBeforeWave | `0.0` |

`EnemyEntries` 添加一项：

| 字段 | 值 |
| --- | --- |
| EnemyId | `BasicEnemy` |
| Count | `2` |
| SpawnInterval | `0.1` |
| SpawnGroup | `Edge` |

验证点：

- 总权重应该是 `2 个敌人 * 2.0 = 4.0`。
- 每杀死一个敌人，进度应该推进 50%。

### 4.4 Buildings

DataTable：`DT_BattleBuildings_Manual`

新增两个 Row。

#### BasicTower

| 字段 | 值 |
| --- | --- |
| RowName | `BasicTower` |
| BuildingId | `BasicTower` |
| RequiredDiagramId | `BasicTower` |
| BuildingClass | `ATTDBattleBuildingActor` |
| AttackMode | `Instant Damage` |
| ProjectileClass | 留空 |

`PartCosts` 添加：

| Id | Count |
| --- | --- |
| `Gear` | `2` |

`BaseStats`：

| 字段 | 值 |
| --- | --- |
| MaxHealth | `50.0` |
| AttackDamage | `5.0` |
| AttackRange | `1000.0` |
| AttackInterval | `0.5` |
| ProjectileSpeed | `900.0` |

#### ProjectileTower

| 字段 | 值 |
| --- | --- |
| RowName | `ProjectileTower` |
| BuildingId | `ProjectileTower` |
| RequiredDiagramId | `BasicTower` |
| BuildingClass | `ATTDBattleBuildingActor` |
| AttackMode | `Projectile` |
| ProjectileClass | `ATTDBattleProjectileActor` |

`PartCosts` 添加：

| Id | Count |
| --- | --- |
| `Gear` | `1` |

`BaseStats`：

| 字段 | 值 |
| --- | --- |
| MaxHealth | `50.0` |
| AttackDamage | `10.0` |
| AttackRange | `1000.0` |
| AttackInterval | `0.01` |
| ProjectileSpeed | `3000.0` |

验证点：

- `BasicTower` 使用即时伤害。
- `ProjectileTower` 会生成投射物。
- 投射物会注册到 `UTTDBattleWorldSubsystem.ActiveProjectiles`，命中、目标失效或战斗结束时释放。

### 4.5 ToyBoxRewards

DataTable：`DT_BattleToyBoxRewards_Manual`

新增 Row：

| 字段 | 值 |
| --- | --- |
| RowName | `BasicToyBox` |
| ToyBoxId | `BasicToyBox` |
| PurchaseCost | `5` |
| RollCount | `2` |

`RewardEntries` 添加一项：

| PartId | Weight | MinQuantity | MaxQuantity |
| --- | --- | --- | --- |
| `Gear` | `1.0` | `1` | `1` |

验证点：

- 花费 5 货币购买 `BasicToyBox`。
- 开启后固定得到 `Gear x2`。

### 4.6 Levels

DataTable：`DT_BattleLevels_Manual`

新增 Row：

| 字段 | 值 |
| --- | --- |
| RowName | `PrototypeBattle` |
| LevelId | `PrototypeBattle` |
| CastleClass | `ATTDBattleCastleActor` |
| CastleTransform | Location `(0, 0, 0)` |
| CastleMaxHealth | `500.0` |
| WaveIds | `WaveA` |
| StartingCurrency | `10` |

`StartingDiagramIds` 添加：

| 值 |
| --- |
| `BasicTower` |

`StartingParts` 添加：

| Id | Count |
| --- | --- |
| `Gear` | `5` |
| `Spring` | `1` |

`StartingToyBoxes` 添加：

| Id | Count |
| --- | --- |
| `BasicToyBox` | `1` |

验证点：

- 进入战斗后 HUD 显示货币 10。
- 初始拥有 `BasicTower` 图纸权限。
- 初始零件足够建造两个塔。
- 初始拥有一个 `BasicToyBox`。

## 5. 配置 DeveloperSettings

打开：

```text
Edit -> Project Settings -> Game -> Toy Tower Defense
```

找到 `Data Tables -> Gameplay Tag Data Tables`。

添加或填写以下映射：

| GameplayTag | DataTable |
| --- | --- |
| `TTD.DataTable.Battle.Levels` | `DT_BattleLevels_Manual` |
| `TTD.DataTable.Battle.Waves` | `DT_BattleWaves_Manual` |
| `TTD.DataTable.Battle.Enemies` | `DT_BattleEnemies_Manual` |
| `TTD.DataTable.Battle.Buildings` | `DT_BattleBuildings_Manual` |
| `TTD.DataTable.Battle.ToyBoxRewards` | `DT_BattleToyBoxRewards_Manual` |
| `TTD.DataTable.Battle.HeightEffects` | `DT_BattleHeightEffects_Manual` |

设置：

| 字段 | 值 |
| --- | --- |
| DefaultBattleLevelId | `PrototypeBattle` |

研究所相关 DataTable 可以暂时不配，因为当前有 C++ fallback 数据。后续要验证纯配置路径时，再添加：

| GameplayTag | Row Struct |
| --- | --- |
| `TTD.DataTable.Research.Parts` | `FTTDPartDefinition` |
| `TTD.DataTable.Research.Diagrams` | `FTTDDiagramDefinition` |
| `TTD.DataTable.Research.ToyBoxes` | `FTTDToyBoxDefinition` |

## 6. 地图 Actor 配置

打开当前默认地图，或者创建一个新测试地图。

### 6.1 放置 SpawnPoint

在 Place Actors 或 Content Browser 中搜索：

```text
ATTDBattleSpawnPointActor
```

放到地图边界附近，例如：

```text
Location = (300, 0, 0)
```

Details 面板设置：

| 字段 | 值 |
| --- | --- |
| SpawnGroup | `Edge` |

注意：

- Wave 的 `SpawnGroup` 是 `Edge`，所以地图里至少要有一个 `SpawnGroup = Edge` 的 SpawnPoint。
- 如果 Wave 的 `SpawnGroup` 留空，则任意 SpawnPoint 都可用，但地图仍然至少要有一个 SpawnPoint。

### 6.2 放置 BuildSlot

搜索并放置：

```text
ATTDBuildSlotActor
```

建议放三个，方便验证不同建筑和高度效果。

#### PlainSlot

| 字段 | 值 |
| --- | --- |
| Location | `(0, 150, 0)` |
| SlotId | `PlainSlot` |
| GridCoordinate | `(0, 0)` |
| HeightLevel | `0` |
| HeightEffectId | 留空 |

#### HighSlot

| 字段 | 值 |
| --- | --- |
| Location | `(0, 300, 0)` |
| SlotId | `HighSlot` |
| GridCoordinate | `(1, 0)` |
| HeightLevel | `5` |
| HeightEffectId | `HighGround` |

#### ProjectileSlot

| 字段 | 值 |
| --- | --- |
| Location | `(0, 450, 0)` |
| SlotId | `ProjectileSlot` |
| GridCoordinate | `(2, 0)` |
| HeightLevel | `0` |
| HeightEffectId | 留空 |

注意：

- BuildSlot 自带 `BoxComponent`，会阻挡 `Visibility` trace。
- 建造输入依赖鼠标点击命中 BuildSlot，所以不要把 Slot 完全藏在其他会挡住 Visibility trace 的物体后面。
- 高度不限制建造。`HeightLevel` 只是数据，属性变化来自 `HeightEffectId`。

### 6.3 城堡不需要手动摆放

城堡由 `FTTDBattleLevelDefinition.CastleClass` 和 `CastleTransform` 在战斗启动时自动生成。

最小验证不要手动放 `ATTDBattleCastleActor`，避免误判。

## 7. 战斗 UI 手动验证

Play 当前地图。

### 7.1 启动战斗

操作：

1. 开始界面点击开始。
2. 主界面点击“关卡挑战”。

预期：

- 如果配置完整，进入 Battle HUD。
- HUD 显示：
  - `State: Running`
  - `Progress`
  - `Castle`
  - `Currency`
  - `Diagrams`
  - `Parts`
  - `Toy Boxes`
  - 建筑按钮列表
  - 玩具盒 Buy/Open 按钮列表

如果启动失败，主界面会显示失败原因。常见原因见本文末尾“故障排查”。

### 7.2 建造 BasicTower

操作：

1. 在 Battle HUD 右侧点击 `BasicTower`。
2. 左侧 `Selected` 应显示 `BasicTower`。
3. 点击地图中的 `PlainSlot`。

预期：

- 状态文本显示 `Building placed.`
- `Parts` 中 `Gear` 数量减少 2。
- 再次点击同一个 Slot 建造会失败，显示 Slot 已占用。

### 7.3 验证高度效果器

操作：

1. 点击 `BasicTower`。
2. 点击 `HighSlot`。

预期：

- 建造成功。
- 高度不会阻止建造。
- 该建筑运行时属性会应用 `HighGround`：
  - AttackDamage 从 5 变为 10。
  - AttackRange 从 1000 变为 2000。
  - AttackInterval 变为 0.25。

当前 HUD 不直接显示建筑运行时属性。手动观察可以通过：

- 高地塔攻击更频繁。
- 使用自动化测试验证精确数值。
- 后续可临时加 Debug UI 或日志。

### 7.4 验证敌人生成和进度

战斗启动后敌人会按 Wave 自动生成。

当前最小配置：

```text
敌人数 = 2
单个敌人 ProgressWeight = 2.0
总权重 = 4.0
```

预期：

- 初始 `Progress` 为 0%。
- 杀死第一个敌人后，`Progress` 约为 50%。
- 杀死第二个敌人后，`State` 变成 `Victory`，`Progress` 为 100%。

### 7.5 验证关卡内玩具盒

操作：

1. 点击 `Buy BasicToyBox`。
2. 观察 `Currency` 从 10 变为 5。
3. `Toy Boxes` 中 `BasicToyBox` 数量增加。
4. 点击 `Open BasicToyBox`。

预期：

- 状态文本显示开启结果。
- 因为最小配置 `RollCount = 2` 且只有 `Gear` 一种奖励，所以获得 `Gear x2`。
- `Parts` 中 Gear 数量增加。

### 7.6 验证 ProjectileTower

操作：

1. 确保还有足够 `Gear`。
2. 点击 `ProjectileTower`。
3. 点击 `ProjectileSlot`。
4. 等第二个敌人生成。

预期：

- `ProjectileTower` 建造成功。
- 建筑攻击时生成 `ATTDBattleProjectileActor`。
- 投射物命中敌人后释放。
- 战斗结束时，所有 ActiveProjectiles 会被 Subsystem 统一释放。

当前 HUD 不直接显示 ActiveProjectiles 数量。可通过自动化测试或临时日志验证。

### 7.7 当前“拖放建造”的真实状态

当前 PlayerController 有：

- `BeginDragBattleBuilding`
- `DropSelectedBattleBuildingOnSlot`

但当前 C++ HUD 的按钮实际只做“点击选择建筑”。手动验证时使用：

```text
点击建筑按钮 -> 点击地图 BuildSlot
```

不要把“可视化拖拽 UI”当作当前已完成能力。后续如果要做真正拖放，需要给建筑按钮接入 UMG drag/drop operation。

## 8. 对象池最小验证

对象池不是启动战斗的硬依赖。未配置对象池时，只要类实现 `UTTDPooledObjectInterface`，系统会临时创建对象并在释放时销毁或交给 GC。

要验证对象池，请配置 `DT_ObjectPool_Manual`。

### 8.1 正常对象池配置

DataTable：`DT_ObjectPool_Manual`

添加三行：

| RowName | ObjectClass | InitialSize | MaxSize | ExpandBy | bPrewarmOnWorldBeginPlay |
| --- | --- | --- | --- | --- | --- |
| `Enemy` | `ATTDBattleEnemyActor` | `0` | `16` | `4` | `true` |
| `Building` | `ATTDBattleBuildingActor` | `0` | `16` | `4` | `true` |
| `Projectile` | `ATTDBattleProjectileActor` | `0` | `32` | `8` | `true` |

在 Project Settings 中添加映射：

| GameplayTag | DataTable |
| --- | --- |
| `TTD.DataTable.ObjectPool.Definitions` | `DT_ObjectPool_Manual` |

预期：

- 敌人、建筑、投射物优先从对象池获取。
- 释放时隐藏、禁用碰撞、禁用 Tick。
- 再次获取时复用。

### 8.2 验证容量不足进入 ConfigurationError

把 `Enemy` 行临时改成：

| 字段 | 值 |
| --- | --- |
| InitialSize | `0` |
| MaxSize | `0` |
| ExpandBy | `1` |

重新 Play 并点击“关卡挑战”。

预期：

- 战斗可以启动，因为 Enemy class 本身合法。
- 到刷怪时间时，对象池无法提供敌人。
- 战斗进入 `Configuration Error`。
- 进度不会因为生成失败而推进。
- 不会误判 `Victory`。

验证完成后，把 `MaxSize` 改回正常值，例如 16。

## 9. 战斗启动失败验证

以下是 P0 修复后的显式失败路径。每次修改后 Play，点击“关卡挑战”，观察主界面状态文本。

### 9.1 缺默认关卡

操作：

- 把 `DefaultBattleLevelId` 改成不存在的值，例如 `MissingLevel`。

预期：

- 不进入 Battle HUD。
- 主界面显示关卡未配置原因。

### 9.2 缺 SpawnPoint

操作：

- 删除地图里的所有 `ATTDBattleSpawnPointActor`。

预期：

- 不生成城堡。
- 不进入假战斗。
- 显示需要至少一个 SpawnPoint。

### 9.3 缺 BuildSlot

操作：

- 删除地图里的所有 `ATTDBuildSlotActor`。

预期：

- 不生成城堡。
- 不进入假战斗。
- 显示需要至少一个 BuildSlot。

### 9.4 Wave 引用不存在 Enemy

操作：

- 在 `DT_BattleWaves_Manual` 中把 EnemyId 改成 `MissingEnemy`。

预期：

- 启动失败。
- 显示 Enemy 未配置。

### 9.5 Enemy class 非池化或类型错误

操作：

- 在 `DT_BattleEnemies_Manual` 中把 EnemyClass 改成普通 `Actor` 或其他不是 `ATTDBattleEnemyActor` 派生且未实现池化接口的类。

预期：

- 启动失败。
- 显示 Enemy class 必须是池化战斗敌人。

## 10. 当前功能清单和使用方式

| 功能 | 使用入口 | 当前状态 |
| --- | --- | --- |
| 开始界面 | Play 后自动显示 | 可用 |
| 主界面 | 开始按钮进入 | 可用 |
| 仓库入口 | 主界面仓库按钮 | 占位 |
| 研究所入口 | 主界面研究所按钮 | 可用 |
| 研究所图鉴 | 研究所左侧图鉴 | 可用 |
| 玩具盒制作队列 | 研究所研发页 | 可用 |
| 离线制作进度 | 关闭后重新进入 | 可用 |
| 图纸研发 | 研究所研发页图纸页签 | 占位 |
| 关卡挑战 | 主界面关卡挑战按钮 | 需要 Battle DataTable 和场景 Actor |
| 战斗启动失败 reason | 主界面状态文本 | 可用 |
| 城堡生成 | Level DataTable | 可用 |
| SpawnPoint 刷怪 | 地图 Actor + Wave SpawnGroup | 可用 |
| 权重进度条 | Battle HUD Progress | 可用 |
| BuildSlot 建造 | 点击建筑按钮，再点击 Slot | 可用 |
| 高度效果器 | BuildSlot HeightEffectId | 可用 |
| 即时伤害建筑 | Building AttackMode | 可用 |
| 投射物建筑 | ProjectileTower | 可用 |
| 关卡内玩具盒购买/开启 | Battle HUD Toy Boxes | 可用 |
| 对象池 | ObjectPool DataTable | 可用 |
| ConfigurationError | 运行中生成失败等 | 可用 |
| GameplayMessageRouter | 系统内部广播 | 可用，当前主要给 UI 和后续系统监听 |

## 11. 最小成功验收标准

完成以下步骤即可认为当前 Editor 内最小验证通过：

1. Play 后能进入开始界面。
2. 点击开始进入主界面。
3. 仓库按钮显示占位提示。
4. 研究所可以进入。
5. 研究所能加入玩具盒制作队列。
6. 制作完成后可以领取。
7. 图鉴能查看已解锁图纸和玩具盒。
8. 配好 Battle DataTable 和地图 Actor 后，关卡挑战能进入 Battle HUD。
9. HUD 显示 Running、Castle、Progress、Currency、Inventory。
10. 点击建筑按钮后能点击 BuildSlot 建造。
11. 敌人生成后建筑能攻击敌人。
12. 杀死敌人后进度推进。
13. 所有敌人死亡后进入 Victory。
14. 删除 SpawnPoint 或 BuildSlot 后，关卡挑战不会假启动，而是显示失败原因。
15. Enemy 对象池 `MaxSize = 0` 时，运行中刷怪失败进入 `ConfigurationError`，不会误判胜利。

## 12. 常见问题

### 点击“关卡挑战”没有进入战斗

看主界面状态文本。常见原因：

- `DefaultBattleLevelId` 为空或不存在。
- Battle DataTable 没有配置到 `UTTDDeveloperSettings.GameplayTagDataTables`。
- DataTable Row Struct 选错。
- Level 引用了不存在的 Wave。
- Wave 引用了不存在的 Enemy。
- 地图里没有 SpawnPoint。
- 地图里没有 BuildSlot。
- Wave 的 `SpawnGroup` 和 SpawnPoint 的 `SpawnGroup` 不一致。

### 点击地图格子无法建造

检查：

- 是否已经先点击 HUD 里的建筑按钮。
- HUD 的 `Selected` 是否显示建筑 ID。
- 点击的 Actor 是否是 `ATTDBuildSlotActor`。
- BuildSlot 是否被其他物体挡住 Visibility trace。
- BuildSlot 是否已占用。
- 是否有对应图纸权限。
- 零件是否足够。

### 建筑按钮没有出现

检查：

- `TTD.DataTable.Battle.Buildings` 是否映射到正确 DataTable。
- DataTable Row Struct 是否是 `FTTDBuildingDefinition`。
- Row 的 `BuildingId` 是否为空。为空时当前加载逻辑会用 RowName 补齐，但建议显式填写。

### 玩具盒按钮没有出现

检查：

- `TTD.DataTable.Battle.ToyBoxRewards` 是否映射正确。
- Row Struct 是否是 `FTTDToyBoxRewardDefinition`。
- Row 的 `ToyBoxId` 是否填写。

### 研究所数据不是我配置的 DataTable

研究所如果读不到正确 DataTable，会使用 C++ fallback 样例数据。检查：

- `TTD.DataTable.Research.Parts`
- `TTD.DataTable.Research.Diagrams`
- `TTD.DataTable.Research.ToyBoxes`

是否都映射到正确 Row Struct 的 DataTable。

### 已经改了默认解锁，但研究所没有变化

研究所有存档。默认解锁只在首次创建存档时生效。

处理方式：

- 删除 SaveSlot `ToyTowerDefenseResearch`。
- 或在代码/调试入口调用 `UTTDResearchSubsystem::ResetProgress()`。

### UI 文案显示异常

当前 UI 文案主要写在 C++ Widget 中。若 Editor 中显示乱码，优先检查源码文件编码和 UE 文本显示。正式项目建议后续迁移到 StringTable 或 Widget Blueprint 文本资源。

## 13. 自动化回归命令

手动验证前后建议跑：

```powershell
E:\UE\SourceEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -unattended -nop4 -nosplash -NullRHI -NoSound -ExecCmds="Automation RunTests ToyTowerDefense; Quit" -TestExit="Automation Test Queue Empty"
```

重点测试包括：

- `ToyTowerDefense.Research.CraftingQueueModel`
- `ToyTowerDefense.Settings.DeveloperSettings`
- `ToyTowerDefense.Messaging.GameplayMessageRouter`
- `ToyTowerDefense.ObjectPool.WorldSubsystem`
- `ToyTowerDefense.Battle.WorldSubsystem`
