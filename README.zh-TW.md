# Cliderness

[English](README.md) · [한국어](README.ko.md) · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · **繁體中文** · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*開放世界生存沙盒* — 製作：**korscripterdev**

Cliderness 是一款完全以 CP437 字元和 VGA 16 色繪製於 80×25 格線上的自由生存沙盒。單一 ISO C90 引擎支援 MS-DOS、FreeDOS、Windows、Linux、macOS 與其他 Unix 系統。沒有劇情，也沒有結局：探索、挖掘、建造、改造大地並生存下去。

## 1.0.0 已實作的功能

* 以種子產生的確定性 3D 世界（16 位元與 64 位元完全一致），46 種生物群系、洞穴、礦石、海洋、河流與地下湖；只儲存被修改的區塊。
* 行走、衝刺、潛行、游泳、挖掘、放置、有耐久度的工具；生命、體力、飢餓、口渴、疲勞、體溫，以及傷病。
* 合成（87 個配方）、工作台、熔爐、營火、鐵砧、祭壇，農業與植樹，箱子、床、裝備與背包、修理。
* ClideFluid：體積守恆的流動水與熔岩、水壩、雨水積水、水桶；廢棄礦坑、遺跡、遠古城市等結構。
* 29 種生物（動物、怪物、NPC），目標驅動 AI、視覺與聽覺、晝夜作息；近戰與弓箭戰鬥、護甲、戰利品、交易、4 個難度。
* 船隻：木筏、划艇、帆船、商船；船槳與受風向影響的帆、貨艙、浮力與超載、3 個船體損傷階段、進水、沉沒。
* 遠古維度：傳送門、8 種生物群系（含虛空裂隙）、遠古礦石、植物與生物、遠古祭壇。
* 季節與月相、21 種天氣、天空觀測，程序化音效（Windows、Linux、macOS 即時 PCM，DOS 使用 PC 喇叭）。
* **240** 個指令、62 個主題 5 個等級的離線說明、11 種介面語言、安全存檔（暫存檔 + CRC）。

限制：說明與物品名稱只有英語/韓語（其他語言顯示英語）。DOS 執行檔由 CI 建置，但尚未在實機或模擬器上驗證。詳見 [ROADMAP.md](ROADMAP.md)。

## 建置

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

執行 `build/cliderness`（會在執行檔旁或目前目錄中尋找 `data/`）。`--help` 顯示選項，`--version` 顯示版本與作者，`--play-sfx rain` 檢查音訊裝置。

## 操作（預設）

`WASD` 移動 · `Shift` 衝刺 · `Ctrl`/`X` 潛行 · `F` 攻擊/挖掘 · `R` 放置 · `>`/`<` 下/上 · `E` 撿取/登船/交易 · `U` 使用 · `K` 合成 · `Tab` 物品欄 · `C` 角色 · `右 Shift`/`V` 天空 · `F1`/`?` 說明 · `Esc` 暫停 · `/` 主控台。所有按鍵都可在設定中修改。

## 文件

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [更新日誌](changelogs.zh-TW.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · 見 [LICENSE](LICENSE).
