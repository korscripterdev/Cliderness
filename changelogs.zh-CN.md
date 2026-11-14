# 更新日志

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · **简体中文** · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Cliderness（制作：korscripterdev）的主要变更。

## [0.1.0] — 2026-11-14

第一个可玩的基础版本（路线图第 1–3 阶段完成，第 4–6 和 10 阶段部分完成）。

### 新增
- ISO C90 引擎，包含 Windows 控制台、POSIX 终端、MS-DOS/FreeDOS 以及测试用无界面平台层。
- 支持脏单元输出和宽字符的 CP437 / VGA 16 色帧缓冲。
- 基于种子的整数世界生成、38 种生物群系、洞穴、矿石、海洋和安全的区块存储。
- 移动、挖掘、放置、工具与耐久度、技能、生存数值、体温、死亡与重生。
- 背包、快捷栏、具有浮力的掉落物。
- ClideTime、18 种天气、天空观察；拥有 71 个命令的 ClideConsole。
- 英语和韩语 45 个主题的帮助、11 种界面语言、可导出 WAV 的程序化 DSP。
- CMake、Make、Open Watcom、DJGPP 构建，GitHub Actions，293,610 项测试检查。

### 已知限制
- 尚无制作、生物、战斗、流动的水、船只和远古维度。
- Windows/Linux/macOS 无实时 PCM 音频（静音运行）。
- DOS 版本尚未编译和运行（本地没有工具链）。
- 帮助仅有英语和韩语，其他语言显示英语帮助。
