# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · **简体中文** · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*开放世界生存沙盒* — 制作：**korscripterdev**

Cliderness 是一款完全由 CP437 字符和 VGA 16 色绘制在 80×25 网格上的自由生存沙盒。一个 ISO C90 引擎同时支持 MS-DOS、FreeDOS、Windows、Linux、macOS 和其他 Unix 系统。没有剧情，也没有结局：探索、挖掘、建造、改造大地并生存下去。

## 0.1.0 已实现的功能

* 基于种子的确定性 3D 世界（16 位与 64 位结果相同），38 种生物群系、洞穴、矿石、海洋、珊瑚礁；只保存被修改的区块。
* 行走、疾跑、潜行、游泳、攀爬、坠落；挖掘、放置、向下挖、向上搭；有耐久度的工具。
* 生命、体力、饥饿、口渴、疲劳、体温；36 格背包和快捷栏，会漂浮或下沉的掉落物。
* 带季节和月相的游戏时间，18 种天气，完整的天空观察（太阳、月亮、星座、云、鸟、流星、极光、日食和月食）。
* **71** 个真实可用的命令（权限、Tab 补全、历史记录、建造的撤销/重做）。
* 45 个主题、5 个等级的离线帮助，11 种界面语言；F1 / Esc 会完全暂停游戏；安全存档（临时文件 + CRC 校验）。

尚未实现：制作、生物与战斗、流动的水、船只、远古维度、实时 PCM 音频。准确状态见 [ROADMAP.md](ROADMAP.md)。

## 构建

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

运行 `build/cliderness`。`--help` 显示选项，`--version` 显示版本和制作者。

## 默认操作

`WASD` 移动 · `Shift` 疾跑 · `Ctrl`/`X` 潜行 · `F` 挖掘 · `R` 放置 · `>`/`<` 下/上 · `E` 拾取 · `U` 使用 · `Tab` 背包 · `C` 角色 · `右Shift`/`V` 天空 · `F1`/`?` 帮助 · `Esc` 暂停 · `/` 控制台。所有按键都可在设置中修改。

## 文档

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [更新日志](changelogs.zh-CN.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · 见 [LICENSE](LICENSE)。
