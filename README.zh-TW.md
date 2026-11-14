# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · **繁體中文** · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*開放世界生存沙盒* — 製作：**korscripterdev**

Cliderness 是一款完全以 CP437 字元和 VGA 16 色繪製於 80×25 格線上的自由生存沙盒。單一 ISO C90 引擎支援 MS-DOS、FreeDOS、Windows、Linux、macOS 與其他 Unix 系統。沒有劇情，也沒有結局：探索、挖掘、建造、改造大地並生存下去。

## 0.1.0 已完成的功能

* 以種子產生的確定性 3D 世界（16 位元與 64 位元結果相同），38 種生態域、洞穴、礦石、海洋、珊瑚礁；只儲存被修改的區塊。
* 行走、衝刺、潛行、游泳、攀爬、墜落；挖掘、放置、向下挖、向上堆；有耐久度的工具。
* 生命、體力、飢餓、口渴、疲勞、體溫；36 格背包與快捷列，會漂浮或下沉的掉落物。
* 有季節與月相的遊戲時間，18 種天氣，完整的天空觀察（太陽、月亮、星座、雲、鳥、流星、極光、日食與月食）。
* **71** 個實際可用的指令（權限、Tab 補全、歷史紀錄、建造的復原/重做）。
* 45 個主題、5 個等級的離線說明，11 種介面語言；F1 / Esc 會完全暫停遊戲；安全存檔（暫存檔 + CRC 驗證）。

尚未實作：製作、生物與戰鬥、流動的水、船隻、遠古維度、即時 PCM 音訊。確切狀態請見 [ROADMAP.md](ROADMAP.md)。

## 建置

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

執行 `build/cliderness`。`--help` 顯示選項，`--version` 顯示版本與製作者。

## 預設操作

`WASD` 移動 · `Shift` 衝刺 · `Ctrl`/`X` 潛行 · `F` 挖掘 · `R` 放置 · `>`/`<` 下/上 · `E` 撿取 · `U` 使用 · `Tab` 背包 · `C` 角色 · `右Shift`/`V` 天空 · `F1`/`?` 說明 · `Esc` 暫停 · `/` 主控台。所有按鍵都能在設定中變更。

## 文件

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [更新紀錄](changelogs.zh-TW.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · 請見 [LICENSE](LICENSE)。
