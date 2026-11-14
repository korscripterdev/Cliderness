# Cliderness

[English](README.md) · **한국어** · [日本語](README.ja.md) · [简体中文](README.zh-CN.md) · [繁體中文](README.zh-TW.md) · [Español](README.es.md) · [Français](README.fr.md) · [Deutsch](README.de.md) · [Português](README.pt.md) · [Русский](README.ru.md) · [Italiano](README.it.md)

*오픈 월드 생존 샌드박스* — 제작: **korscripterdev**

Cliderness는 80×25 격자 위에 VGA 16색 CP437 문자만으로 그려지는 자유형 생존
샌드박스입니다. 하나의 ISO C90 엔진으로 MS-DOS, FreeDOS, Windows, Linux,
macOS 및 기타 Unix를 지원합니다. 스토리도 엔딩도 없습니다. 탐험하고, 채굴하고,
건축하고, 땅을 바꾸며 살아남으세요.

## 0.5.0에서 동작하는 기능

* 시드 기반 결정적 3D 월드 (16비트와 64비트에서 동일), 바이옴 38종, 동굴,
  광석, 바다, 산호초. 바뀐 청크만 저장됩니다.
* 걷기, 달리기, 웅크리기, 수영, 오르기, 추락. 채굴, 설치, 아래로 파기, 위로
  쌓기. 내구도가 있는 도구.
* 생존 수치: 체력, 기력, 허기, 갈증, 피로, 체온.
* 36칸 인벤토리 + 핫바, 물에 뜨거나 가라앉는 드롭 아이템.
* 제작(제작법 58개), 작업대, 제련, 요리, 농사와 나무 기르기, 상자, 침대와 잠,
  장비와 가방, 수리, 부상과 질병.
* ClideFluid: 양이 보존되는 흐르는 물과 용암, 댐, 빗물 웅덩이, 양동이.
  강, 지하 호수, 폐광과 유적.
* 목표 기반 AI, 시야와 청각, 낮/밤 일과를 가진 생물 22종(동물, 몬스터, NPC).
  근접·활 전투, 방어구, 전리품, 거래, 햇빛에 타는 언데드, 난이도 4단계.
* 선박: 뗏목, 나룻배, 범선, 상선. 풍향을 반영한 노와 돛, 화물칸, 부력과 과적,
  선체 손상 3단계, 누수, 침수, 물 퍼내기, 수리, 파도, 좌초, 침몰.
* 계절과 달 위상이 있는 게임 시간, 날씨 18종, 하늘 관측 (태양, 달, 시드
  기반 별자리, 구름, 새, 별똥별, 오로라, 일식·월식).
* 명령어 콘솔: 권한, Tab 자동완성, 기록, 건축 명령 Undo/Redo를 갖춘 실제
  동작 명령어 **116개**.
* 오프라인 도움말: 59개 주제, 5단계, 검색, 즐겨찾기, 관련 문서.
* UI 11개 언어. F1 / Esc는 게임을 완전히 정지시킵니다.
* 안전한 저장 (임시 파일 + CRC 검증 + 원자적 교체).

아직 없는 것: 고대 차원, 실시간 PCM 소리.
정확한 상태는 [ROADMAP.md](ROADMAP.md)를 참고하세요.

## 빌드

```sh
cmake -S . -B build && cmake --build build && ctest --test-dir build
make && make test                     # CMake 없이 Linux / macOS / Unix
sh builds/dos/build_watcom.sh 16      # MS-DOS (Open Watcom), docs/platforms/dos.md 참고
```

`build/cliderness`를 실행하세요(실행 파일 옆이나 현재 폴더의 `data/`를
찾습니다). `cliderness --help`로 옵션을, `--version`으로 버전과 제작자를 봅니다.

## 기본 조작

`WASD` 이동 · `Shift` 달리기 · `Ctrl`/`X` 웅크리기 · `F` 채굴 · `R` 설치 ·
`>`/`<` 내려가기/올라가기 · `E` 줍기 · `U` 사용 · `Tab` 인벤토리 · `C` 캐릭터 ·
`오른쪽 Shift`/`V` 하늘 · `F1`/`?` 도움말 · `Esc` 일시정지 · `/` 콘솔.
모든 키는 설정에서 바꿀 수 있습니다.

## 문서

[DESIGN.md](DESIGN.md) · [ARCHITECTURE.md](ARCHITECTURE.md) ·
[ROADMAP.md](ROADMAP.md) · [변경 기록](changelogs.ko.md) ·
[명령어](docs/commands/commands.md) · [DOS](docs/platforms/dos.md)

© 2026 korscripterdev · GPL-3.0-or-later · [LICENSE](LICENSE) 참고.
