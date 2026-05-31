# ToyTowerDefense

`ToyTowerDefense` 是一个 UE5 C++ 塔防合作项目。当前版本包含开始菜单、研究所原型、GameplayMessageRouter 消息系统、全局对象池，以及一个可手动验证的守护城堡战斗闭环。

## 环境要求

- Unreal Engine 5，当前本地工程使用 `E:\UE\SourceEngine`
- Windows + Visual Studio C++ 工具链
- Git

## 打开项目

双击打开：

```text
ToyTowerDefense.uproject
```

默认地图配置在 `Config/DefaultEngine.ini`，当前最小验证地图为：

```text
/Game/_Main/NewMap
```

## 当前可验证内容

- 开始界面和主菜单
- 研究所界面、图鉴、玩具盒制作队列
- DeveloperSettings + GameplayTag -> DataTable 配置入口
- Lyra GameplayMessageRouter 消息插件
- 基于 `UWorldSubsystem` 的对象池
- 守护城堡战斗原型：
  - 四边刷怪
  - 中心城堡
  - 普通建造格和高地效果格
  - 简单炮塔、投射物炮塔、敌人、投射物

进入方式：

```text
Play -> 开始 -> 关卡挑战
```

## 生成最小验证资源

如需重新生成验证地图、DataTable、材质和 Blueprint 壳，可以在关闭编辑器后运行：

```powershell
E:\UE\SourceEngine\Engine\Binaries\Win64\UnrealEditor.exe E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -ExecutePythonScript=E:\Github\ToyTower\ToyTowerDefense\Content\Python\setup_minimal_validation.py
```

## 构建

```powershell
E:\UE\SourceEngine\Engine\Build\BatchFiles\Build.bat ToyTowerDefenseEditor Win64 Development -Project=E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -WaitMutex -NoHotReloadFromIDE
```

## 自动化测试

```powershell
E:\UE\SourceEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe E:\Github\ToyTower\ToyTowerDefense\ToyTowerDefense.uproject -unattended -nop4 -nosplash -NullRHI -NoSound -ExecCmds="Automation RunTests ToyTowerDefense; Quit" -TestExit="Automation Test Queue Empty"
```

## Git 协作说明

仓库会保留源码、配置、文档、插件源码和 UE 内容资产，例如 `.uasset`、`.umap`。

以下内容不入库：

- `.vs/`、`.idea/`
- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`
- `.sln`
- 本地日志、缓存、临时文件

更多项目结构和 Agent 接手说明见 `AGENTS.md`。
