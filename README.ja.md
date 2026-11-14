# Cliderness

[English](README.md) · [한국어](README.ko.md) · **日本語** · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*オープンワールド・サバイバル・サンドボックス* — 制作: **korscripterdev**

Clidernessは、80×25のグリッドにVGA 16色のCP437文字だけで描かれる自由なサバイバル・サンドボックスです。ひとつのISO C90エンジンでMS-DOS、FreeDOS、Windows、Linux、macOS、その他のUnixに対応します。ストーリーもエンディングもありません。探検し、掘り、建て、土地を作り変えて生き延びてください。

## 1.1.0で動作する機能

* シードによる決定的な3Dワールド(16ビットでも64ビットでも同一)、46種のバイオーム、洞窟、鉱石、海、川、地下湖。変更したチャンクだけを保存。
* 歩行、ダッシュ、しゃがみ、水泳、採掘、設置、耐久度のある道具。体力、スタミナ、空腹、渇き、疲労、体温、ケガと病気。
* クラフト(87レシピ)、作業台・かまど・焚き火・金床・祭壇、農業と植林、チェスト、ベッド、装備とバッグ、修理。
* ClideFluid: 体積が保存される流れる水と溶岩、ダム、雨の水たまり、バケツ。廃坑・遺跡・古代都市などの構造物。
* 30種の生物(動物、モンスター、NPC)、目標駆動AI、視覚と聴覚、昼夜の行動。近接と弓の戦闘、防具、戦利品、取引、4段階の難易度。
* 船: いかだ、手漕ぎボート、帆船、商船。オールと風を受ける帆、船倉、浮力と積みすぎ、船体損傷3段階、浸水、沈没。
* 古代次元: ポータル、8種のバイオーム(虚空の裂け目)、古代の鉱石・植物・生物、古代の祭壇。
* 季節と月相、21種の天気、空の観測、手続き型サウンド(Windows・Linux・macOSでリアルタイムPCM、DOSはPCスピーカー)。
* 1.1.0の新機能: 生物のA*経路探索、プレイで伸びる能力値、釣りと隠密、風による漂流と船員の雇用、古代の空の第二の月と環のある惑星、Linuxでのミキシング音声ストリーミング、全11言語のヘルプとコンテンツ名。
* **240**個のコマンド、63項目・5レベルのオフラインヘルプ、11言語のUI、安全な保存(一時ファイル+CRC)。

制限: DOS実行ファイルはCIでビルドとリンクまで確認済み(Open Watcom、16ビットとDOS/4GW)ですが、実機・エミュレーターでの動作は未確認です。LinuxとmacOSのリアルタイム音声は実機で未確認です。詳細は[ROADMAP.md](ROADMAP.md)。

## ビルド

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test
sh builds/dos/build_watcom.sh 16
python tools/package.py            # ZIP / tar.gz (CPack)
```

`build/cliderness`を実行してください(`data/`を実行ファイルの隣か現在のフォルダーで探します)。`--help`でオプション、`--version`でバージョンと制作者、`--play-sfx rain`で音声デバイスを確認できます。

## 操作 (初期設定)

`WASD` 移動 · `Shift` ダッシュ · `Ctrl`/`X` しゃがむ · `F` 攻撃/掘る · `R` 置く · `>`/`<` 下/上 · `E` 拾う/乗る/取引 · `U` 使う · `K` クラフト · `Tab` インベントリ · `C` キャラクター · `右Shift`/`V` 空 · `F1`/`?` ヘルプ · `Esc` 一時停止 · `/` コンソール。すべてのキーは設定で変更できます。

## ドキュメント

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [ROADMAP.md](ROADMAP.md) · [変更履歴](changelogs.ja.md) · [commands](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · 参照 [LICENSE](LICENSE).
