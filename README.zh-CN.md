# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · **简体中文** · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*开放世界生存沙盒* — 制作：**korscripterdev**

Cliderness 是一款完全由 CP437 字符和 VGA 16 色绘制在 80×25 网格上的自由生存沙盒。一个 ISO C90 引擎同时支持 MS-DOS、FreeDOS、Windows、Linux、macOS 和其他 Unix 系统。没有剧情，也没有结局：探索、挖掘、建造、改造大地并生存下去。

## 1.1.0 已实现的功能

* 基于种子的确定性 3D 世界（16 位与 64 位完全一致），46 种生物群系、洞穴、矿石、海洋、河流和地下湖；只保存被修改的区块。
* 行走、冲刺、潜行、游泳、挖掘、放置、有耐久度的工具；生命、体力、饥饿、口渴、疲劳、体温，以及伤病。
* 合成（87 个配方）、工作台、熔炉、营火、铁砧、祭坛，农业与植树，箱子、床、装备与背包、修理。
* ClideFluid：体积守恒的流动水与熔岩、水坝、雨水积水、水桶；废弃矿井、遗迹、远古城市等结构。
* 30 种生物（动物、怪物、NPC），目标驱动 AI、视觉与听觉、昼夜作息；近战与弓箭战斗、护甲、战利品、交易、4 个难度。
* 船只：木筏、划艇、帆船、商船；船桨与受风向影响的帆、货舱、浮力与超载、3 个船体损伤阶段、进水、沉没。
* 远古维度：传送门、8 种生物群系（含虚空裂隙）、远古矿石、植物与生物、远古祭坛。
* 季节与月相、21 种天气、天空观测，程序化音效（Windows、Linux、macOS 实时 PCM，DOS 使用 PC 喇叭）。
* 1.1.0 新增：生物 A* 寻路、随游戏成长的属性、钓鱼与隐蔽、风力漂流与雇佣船员、远古天空的第二个月亮和环带星球、Linux 混音串流音频、全部 11 种语言的帮助与内容名称。
* **240** 个命令、63 个主题 5 个级别的离线帮助、11 种界面语言、安全存档（临时文件 + CRC）。

限制：DOS 可执行文件已由 CI 完成编译和链接（Open Watcom，16 位与 DOS/4GW），但尚未在真机或模拟器上运行。Linux 和 macOS 的实时音频尚未在真实设备上验证。详见 [ROADMAP.md](ROADMAP.md)。

## 构建

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

运行 `build/cliderness`（会在可执行文件旁或当前目录中寻找 `data/`）。`--help` 显示选项，`--version` 显示版本与作者，`--play-sfx rain` 检查音频设备。

## 操作（默认）

`WASD` 移动 · `Shift` 冲刺 · `Ctrl`/`X` 潜行 · `F` 攻击/挖掘 · `R` 放置 · `>`/`<` 下/上 · `E` 拾取/登船/交易 · `U` 使用 · `K` 合成 · `Tab` 物品栏 · `C` 角色 · `右 Shift`/`V` 天空 · `F1`/`?` 帮助 · `Esc` 暂停 · `/` 控制台。所有按键都可在设置中修改。

## 文档

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [更新日志](changelogs.zh-CN.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · 见 [LICENSE](LICENSE).
