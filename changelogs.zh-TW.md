# 更新紀錄

[English](changelogs.md) · [한국어](changelogs.ko.md) · [日本語](changelogs.ja.md) · [简体中文](changelogs.zh-CN.md) · **繁體中文** · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Cliderness（製作：korscripterdev）的主要變更。

## [0.2.0] — 2026-11-14

路線圖第 4 階段（製作與建造）及第 3 階段剩餘項目。

### 新增
- 製作畫面（K）、58 個資料驅動配方、工作台、熔爐、營火、鐵砧與燃料。
- 冶煉、烹飪、農業（鋤頭、耕地、小麥與馬鈴薯、會長成樹的樹苗）。
- 箱子、床與睡眠、裝備欄（頭、身體、腿、腳、背部）、修理。
- 狀態效果：流血、骨折、食物中毒、感冒、潮濕、飽足、休息，以及治療物品。
- 新增 15 個指令（共 86 個）。授權：GPL-3.0 或更新版本。

### 變更
- 0.1.0 之後新增的指令說明以英語顯示（介面仍完整翻譯為 11 種語言）。

## [0.1.0] — 2026-11-14

第一個可遊玩的基礎版本（路線圖第 1–3 階段完成，第 4–6 與 10 階段部分完成）。

### 新增
- ISO C90 引擎，包含 Windows 主控台、POSIX 終端機、MS-DOS/FreeDOS 以及測試用無介面平台層。
- 支援髒格輸出與全形字元的 CP437 / VGA 16 色畫面緩衝。
- 以種子產生的整數世界、38 種生態域、洞穴、礦石、海洋與安全的區塊儲存。
- 移動、挖掘、放置、工具與耐久度、技能、生存數值、體溫、死亡與重生。
- 背包、快捷列、具浮力的掉落物。
- ClideTime、18 種天氣、天空觀察；擁有 71 個指令的 ClideConsole。
- 英語與韓語 45 個主題的說明、11 種介面語言、可輸出 WAV 的程序化 DSP。
- CMake、Make、Open Watcom、DJGPP 建置，GitHub Actions，293,610 項測試檢查。

### 已知限制
- 尚無製作、生物、戰鬥、流動的水、船隻與遠古維度。
- Windows/Linux/macOS 無即時 PCM 音訊（靜音執行）。
- DOS 版本尚未編譯與執行（本機沒有工具鏈）。
- 說明僅有英語與韓語，其他語言顯示英語說明。
