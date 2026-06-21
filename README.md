# ToyTowerDefense

`ToyTowerDefense` 是一个基于 Unreal Engine 5.8 的 C++ 玩具塔防项目。当前版本实现了开始界面、主界面、研究所、仓库、战前配置，以及首版守护城堡战斗循环。

## 当前内容

- 研究所：图鉴、图纸研发、玩具盒制作队列、离线进度补算。
- 仓库：零件、图纸、玩具盒库存展示。
- 战前配置：选择携带图纸和真实玩具盒库存。
- 战斗：波次刷怪、金币掉落、开盒获取零件、建造建筑、升级建筑、出售建筑、胜利返回选关。
- 配置：核心玩法数据通过 `UTTDDeveloperSettings` 和 `GameplayTag -> DataTable` 管理。

## 快速打开

项目已提交 Win64 Development Editor 预编译二进制。安装 Unreal Engine 5.8 后，可先运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\Scripts\SetupDirectOpen.ps1
```

脚本会检查预编译二进制并注册本机引擎关联。完成后可以直接双击 `ToyTowerDefense.uproject` 打开项目。

## 交付文档

- `docs/策划须知.md`：当前玩法规则、验收重点和限制。
- `docs/如何配置.md`：策划配置入口、DataTable 字段和配置建议。
- `docs/免编译打开说明.md`：免编译打开项目的使用说明。

## 说明

当前 UI 仍以 C++ 原型控件为主，正式图标、拖拽交互、关卡进度保存、结算表现和更多关卡内容仍待后续补齐。
