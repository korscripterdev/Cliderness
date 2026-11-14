# 変更履歴

[English](changelogs.md) · [한국어](changelogs.ko.md) · **日本語** · [简体中文](changelogs.zh-CN.md) · [繁體中文](changelogs.zh-TW.md) · [Español](changelogs.es.md) · [Français](changelogs.fr.md) · [Deutsch](changelogs.de.md) · [Português](changelogs.pt.md) · [Русский](changelogs.ru.md) · [Italiano](changelogs.it.md)

Cliderness(制作: korscripterdev)の主な変更点です。

## [0.1.0] — 2026-11-14

最初のプレイ可能な基盤(ロードマップのフェーズ1〜3完了、4〜6と10は一部)。

### 追加
- Windowsコンソール、POSIX端末、MS-DOS/FreeDOS、テスト用ヘッドレスのプラットフォーム層を持つISO C90エンジン。
- ダーティセル出力と全角文字に対応したCP437 / VGA 16色フレームバッファ。
- シードによる整数ワールド生成、38種のバイオーム、洞窟、鉱石、海、安全なチャンク保存。
- 移動、採掘、設置、道具と耐久度、スキル、生存値、体温、死亡と復活。
- インベントリ、ホットバー、浮力のあるドロップアイテム。
- ClideTime、18種の天気、空の観測。71個のコマンドを持つClideConsole。
- 英語・韓国語45項目のヘルプ、11言語のUI、WAV出力できる手続き型DSP。
- CMake、Make、Open Watcom、DJGPPのビルド、GitHub Actions、293,610件のテストチェック。

### 既知の制限
- クラフト、生物、戦闘、流れる水、船、古代次元は未実装。
- Windows/Linux/macOSのリアルタイムPCM音声なし(無音で動作)。
- DOS版はまだコンパイル・実行していません(ローカルにツールチェーンなし)。
- ヘルプは英語と韓国語のみ。他の言語では英語のヘルプを表示します。
