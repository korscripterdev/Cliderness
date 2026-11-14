# Cliderness

[English](README.md) · [한국어](README.ko.md) · **日本語** · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*オープンワールド・サバイバル・サンドボックス* — 制作: **korscripterdev**

Clidernessは、80×25のグリッドにVGA 16色のCP437文字だけで描かれる自由なサバイバル・サンドボックスです。ひとつのISO C90エンジンでMS-DOS、FreeDOS、Windows、Linux、macOS、その他のUnixに対応します。ストーリーもエンディングもありません。探検し、掘り、建て、土地を作り変えて生き延びてください。

## 0.1.0で動作する機能

* シードによる決定的な3Dワールド(16ビットでも64ビットでも同一)、38種のバイオーム、洞窟、鉱石、海、サンゴ礁。変更したチャンクだけを保存。
* 歩行、ダッシュ、しゃがみ、水泳、登り、落下。採掘、設置、下へ掘る、上へ積む。耐久度のある道具。
* 体力、スタミナ、空腹、渇き、疲労、体温。36スロットのインベントリとホットバー、水に浮く/沈むアイテム。
* 季節と月相のあるゲーム時間、18種の天気、空の観測(太陽、月、星座、雲、鳥、流れ星、オーロラ、日食・月食)。
* 実際に動作する**71**個のコマンド(権限、Tab補完、履歴、建築のUndo/Redo)。
* 45項目・5レベルのオフラインヘルプ、11言語のUI。F1 / Escでゲームは完全に停止します。安全な保存(一時ファイル+CRC検証)。

未実装: クラフト、生物と戦闘、流れる水、船、古代次元、リアルタイムPCM音声。正確な状況は[ROADMAP.md](ROADMAP.md)を参照。

## ビルド

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
```

`build/cliderness`を実行してください。`--help`でオプション、`--version`でバージョンと制作者を表示します。

## 操作 (初期設定)

`WASD` 移動 · `Shift` ダッシュ · `Ctrl`/`X` しゃがむ · `F` 掘る · `R` 置く · `>`/`<` 下/上 · `E` 拾う · `U` 使う · `Tab` インベントリ · `C` キャラクター · `右Shift`/`V` 空 · `F1`/`?` ヘルプ · `Esc` 一時停止 · `/` コンソール。すべてのキーは設定で変更できます。

## ドキュメント

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [変更履歴](changelogs.ja.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · [LICENSE](LICENSE)を参照。
