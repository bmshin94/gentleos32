# GentleOS/32 전수조사 분석 정리 (한국어)

> 작성: Claude Code (카리나 페르소나) · 요청자: bmshin94
> 작성일: 2026-10-01
> 대상 커밋: `bf27483` (Merge pull request #1 from bmshin94/feat/claude-guide)

---

## 🔗 관련 링크

| 구분 | 주소 |
|---|---|
| **이 저장소 (포크)** | https://github.com/bmshin94/gentleos32 |
| **원본 저장소 (upstream)** | https://github.com/luke8086/gentleos32 |
| **공식 웹사이트 / 웹 데모** | https://luke8086.dev/gentleos32 |
| **자매 프로젝트 (16비트)** | https://github.com/luke8086/gentleos |
| 작업 브랜치 | `claude/intelligent-goldberg-sdexy3` |

### 외부 에셋 출처
- v86 (브라우저 x86 에뮬레이터): https://github.com/copy/v86
- Icons8 (무료 라이선스): https://icons8.com/
- Mona Font: https://github.com/MonadABXY/mona-font
- The Ultimate Oldschool PC Font Pack: https://int10h.org/oldschool-pc-fonts/
- Anthony's Icon Library: https://antofthy.gitlab.io/
- Squidfingers 패턴: https://everythingisabox.com/

---

## 1. 한 줄 요약

**GentleOS/32는 빈티지 32비트 PC(i386)에서 맨 바닥(bare metal)부터 동작하는 취미용 운영체제다.**
플러그인도, 스킬도, MCP도, 라이브러리도 아니다. 윈도우/리눅스의 **자리를 대신하는 OS 본체**다.

| 항목 | 내용 |
|---|---|
| 이름 | GentleOS/32 |
| 원작자 | luke8086 |
| 라이선스 | **GPLv2** (일부 vendor 에셋은 별도 라이선스) |
| 규모 | C + 어셈블리 약 **18,674줄** |
| 타겟 | i386 32비트, RAM 4MB 수준 |
| 커밋 수 | 51개 (luke8086 49 / bmshin94 2) |
| 빌드 | Docker (alpine:3.20) 한 줄 |

---

## 2. 폴더별 전수조사 결과

| 디렉터리 | 줄 수 | 역할 |
|---|---:|---|
| `boot/` | 1,170 | 2단계 부트로더 (MBR 512B + stage2, 리얼모드→보호모드, VESA 메뉴) |
| `kernel/` | 2,520 | 인터럽트·PIC·타이머·RTC·PS/2·UART·PC스피커·VGA·VMware 백도어·메모리 |
| `lib/` | 1,407 | 자체 표준 라이브러리 (printf, string, heap, rand, time, cpu.s) |
| `gui/` | 3,138 | 윈도우 매니저 + 위젯 시스템 + **planar.c VGA 최적화 렌더러** |
| `feat/` | 277 | `card.c` — 솔리테어 계열이 공유하는 트럼프 카드 렌더러 |
| `apps/` | 8,895 | 앱 22개 (전체 코드의 절반) |
| `include/` | 1,267 | 헤더 11개 (`p_*.h`는 `cproto.pl`이 자동 생성) |
| `tools/` | 1,217 | 디스크/initrd/음악/에셋/웹HTML 생성 스크립트 |
| `assets/` | - | PBM 아이콘·패턴·스프라이트 + 클래식 MusicXML 10곡 |
| `vendor/` | - | v86(WASM 에뮬레이터), 폰트, 아이콘, 월페이퍼 |
| `misc/` | - | 링커 스크립트, GRUB 디스크, VGA 팔레트(GIMP), 웹 템플릿 |
| `doc/` | - | 실기 사진 + (이 문서) |

### 2.1 주요 파일 하이라이트

- **`boot/boot_a.s`** (14KB) — 512바이트 MBR + 2단계 로더. BIOS 인터럽트로 섹터 읽기, A20, GDT, 보호모드 점프
- **`boot/boot_c.c`** — 부팅 메뉴. VESA/VBE BIOS에 직접 질의해 사용 가능한 8bpp 선형 프레임버퍼 모드 목록 생성
- **`kernel/main.c`** — 부팅 순서: `intr → pic → mboot → uart → mem → initrd → vga → timer → rtc → mouse → keyboard → ps2 → vmware → file_init_all → gui_main`
- **`kernel/vga.c`** — VGA 레지스터(MISC/SEQ/CRTC/GC/AC)를 직접 기록해 모드 12h(640×480×16색) 설정
- **`kernel/speaker.c`** — PC 스피커(비퍼) 하나로 음악 재생
- **`lib/heap.c`** — first-fit 연결리스트 할당기. 블록마다 `magic` + `desc[16]` 라벨 → `Ctrl+Shift+M`으로 사용처별 메모리 덤프
- **`gui/planar.c`** (12KB) — 이 프로젝트의 기술적 핵심. VGA 4비트플레인 구조를 래치/비트마스크/write-mode로 가속
- **`tools/mkemu.py`** — v86 WASM + SeaBIOS + VGA BIOS + 디스크 이미지를 전부 base64로 묶어 **단일 HTML 파일** 생성

### 2.2 아키텍처: "없는 것"이 더 중요하다

`grep` 전수 확인 결과 다음이 **전혀 없다**:

| 없는 것 | 결과 |
|---|---|
| 스케줄러 / 태스크 스위치 | 앱이 **단일 이벤트 루프를 공유** (`on_tick()`에서 조금씩 일함) |
| 프로세스 / fork | 앱이 커널 함수를 직접 호출 |
| 페이징 / MMU | 단일 주소공간, 메모리 보호 없음 |
| 멀티스레드 | 없음 |
| 유저·권한 분리 | 전부 동일 권한 |
| 네트워크 스택 | 외부 통신 경로가 물리적으로 없음 |
| 쓰기 가능 파일시스템 | initrd 읽기 전용 + 선형 이름 검색(`lib/file.c`) |

→ 정확한 분류: **"OS 커널 + GUI 쉘이 합쳐진 단일 바이너리 애플리케이션"**.
의도된 설계 선택이며, 그 덕분에 **1.8만 줄로 전체를 읽을 수 있는 OS**가 되었다.

### 2.3 발견 사항: 등록되지 않은 스도쿠 앱

- `apps/sudoku.c` (10.8KB)는 완성되어 있고 `include/p_apps.h`에 선언도 있다
- `assets/icons/sudoku.pbm` 아이콘도 존재한다
- 그런데 **`gui/main.c`의 `gui_apps[]` 배열에 등록되어 있지 않아** 런처에 나타나지 않는다
- 참고: `app_logview`와 `app_launcher`는 **의도적으로** 미등록(단축키 전용)

**첫 기여 아이디어**: `gui/main.c`의 `gui_apps[]`에 `&app_sudoku,` 추가 + `apps/sudoku.c`의 `app_sudoku`에 `.icon = &icon_sudoku,` 추가

---

## 3. 설치 및 사용법

### 3.1 빌드 (Docker만 필요)

```bash
git clone https://github.com/bmshin94/gentleos32
cd gentleos32
docker compose run --rm dev make -j4
```

산출물:

| 파일 | 용도 |
|---|---|
| `gentleos32-base.img` | 순수 부팅 이미지 |
| `gentleos32-emu.img` | 에뮬레이터용 (월페이퍼 포함, 실린더 정렬 패딩) |
| `gentleos32-web.img` | 웹용 (부팅 메뉴 생략 + UART 디버그) |
| **`gentleos32-web.html`** | 브라우저에서 바로 실행되는 단일 파일 |
| `gentleos32-grub.img` | GRUB 부팅용 |

정리: `make clean` / `docker compose down --rmi all`

### 3.2 실행 방법

```bash
# A. 브라우저 (가장 쉬움)
open gentleos32-web.html

# B. QEMU
qemu-system-i386 -hda gentleos32-emu.img -m 4 -soundhw pcspk -serial stdio

# C. 실물 디스크 (⚠️ 장치명 반드시 확인, 디스크 전체를 덮어씀)
sudo dd if=gentleos32-base.img of=/dev/sdX bs=1M status=progress
```

### 3.3 조작법

| 입력 | 동작 |
|---|---|
| 부팅 중 `m` (약 0.16초 내) | 비디오 모드 / 시리얼 포트 메뉴 |
| 제목줄 드래그 | 창 이동 |
| `Ctrl+Shift+A` | 앱 런처 |
| `Ctrl+Shift+D` | 디버그 로그 뷰어 |
| `Ctrl+Shift+M` | 힙 메모리 덤프 |
| `Ctrl+Shift+S` | 시리얼 마우스 활성화 |

### 3.4 콘텐츠 추가

```bash
# 악보 변환 (MusicXML → SPK)
uv run tools/mkspk.py -i song.musicxml -o song.spk

# initrd 생성 + 디스크 이미지에 설치
uv run tools/mkinitrd.py wallpaper.png song.spk -d gentleos32-emu.img -p

# initrd만 파일로 (실물 GRUB 디스크용)
uv run tools/mkinitrd.py files... -o gentleos.rd
```

월페이퍼 3종류:

| 종류 | 조건 |
|---|---|
| 모노크롬 | 흑백, 너비가 8의 배수, 1bpp 저장 → 설정에서 색 변경 가능 |
| 컬러 | 임의 이미지, 8bpp 변환 후 타일링 |
| 픽셀아트 | **정확히 64×43px**, 640×480 모드 전용 |

GIMP에서 `misc/vga-256.gpl` / `misc/vga-16.gpl` 팔레트로 인덱스 모드 변환 권장.
SPK 제약: 단일 스태프·단일 성부, 화음 불가. 일반 음표/꾸밈음/타이/스타카토/템포만 지원.

---

## 4. 질문별 답변 요약

### Q. 플러그인? 스킬? MCP?
**전부 아니다.** 운영체제 소스코드다. 저장소의 `CLAUDE.md`는 bmshin94가 추가한 Claude Code 페르소나 파일이며 프로젝트 본체와 무관하다.

근거: `outb()` 포트 I/O, `cpu_lidt()` IDT 설치, `0xA0000` 비디오 메모리 직접 쓰기, `-ffreestanding -nostdlib`, 진입점이 `main()`이 아닌 `krn_main()`.

### Q. API 토큰 필요?
**전혀 불필요. 비용 0원.** 네트워크 스택 자체가 없는 완전 오프라인(에어갭) 시스템. 유일한 외부 I/O는 COM1 시리얼(디버그 로그 출력 / 시리얼 마우스 입력). 인터넷은 Docker 이미지 받을 때만 필요.

### Q. AI 에이전트 구축에 도움?
**직접적으로는 불가, 간접적으로는 매우 유용.**

불가 이유: 네트워크 없음, Python/Node 런타임 없음, 멀티태스킹 없음, RAM 4MB.

유용한 이유 — 구조적 대응 관계:

| GentleOS | AI 에이전트 |
|---|---|
| 이벤트 큐 (`krn_event_pop`) | 메시지/태스크 큐 |
| `gui_main()` while(1) 루프 | 에이전트 루프 (perceive→decide→act) |
| `app_st` 플러그인 등록 배열 | 툴 레지스트리 |
| 위젯 함수 포인터 디스패치 | 툴 콜 디스패처 |
| `heap_alloc(sz, "About app")` | 토큰/리소스 예산 추적 |
| `E_TOO_MANY_WINDOWS` 가드 | 리소스 제한 / 레이트 리밋 |
| "큐 비었을 때만 flush" | "툴 콜 종료 후 한 번만 커밋" |

가장 현실적인 활용: **Claude Code 등 AI 코딩 에이전트의 연습/벤치마크 대상.**
빌드 성공 여부로 검증이 명확하고, 멀티파일 수정 과제(스도쿠 등록)와 패턴 학습 과제(새 앱 추가)가 자연스럽게 나온다.

### Q. React나 PHP로 만들 수 있어?
**OS 본체는 불가. 주변 생태계는 전부 가능.**

불가 이유 (닭-달걀 문제): JS를 실행하려면 JS 엔진이 필요하고, JS 엔진을 실행하려면 OS가 필요하다. 포트 I/O, 특정 물리 주소 접근, 512바이트 부트섹터, 인터럽트 핸들러를 React/PHP로는 표현할 수 없다.

**React로 만들 수 있는 것:**
1. v86 에뮬레이터를 감싼 웹 쇼케이스 컴포넌트
2. **웹 월페이퍼 변환기** — Canvas API로 VGA 팔레트 양자화 → PBM/PPM → initrd 생성 (100% 클라이언트 사이드, 가장 실용적)
3. 웹 악보 편집기 → SPK 생성 (SPK가 평문 텍스트라 쉬움) + Web Audio 미리듣기
4. 코드베이스 인터랙티브 탐색기 (부팅 시퀀스 애니메이션, 메모리 맵, VGA 플레인 다이어그램)
5. 레트로 UI React 컴포넌트 라이브러리 (디자인만 참고, 코드는 신규 작성 → GPL 영향 없음)

**PHP로 만들 수 있는 것:**
1. 월페이퍼/음악 공유 커뮤니티 (Laravel 또는 WordPress)
2. 서버사이드 커스텀 이미지 빌드 서비스 (Laravel Queue + Docker)
   - ⚠️ **보안 주의**: 사용자 입력을 shell에 전달하면 명령 주입 위험. `escapeshellarg()` + 화이트리스트 검증 + Docker 샌드박스 필수
3. 강의 플랫폼 백엔드 (결제, 수강 관리, 진도 추적)

### Q. 유튜브 강의 영상 제작 가능?
**가능하고, 소재로서 매우 우수하다.**

강점: 브라우저에서 즉시 부팅(첫 10초 후킹), Docker만 있으면 누구나 재현, 한국어 OSDev 콘텐츠 희소, 레트로 트렌드 부합, 매 편 시각적 성취, GPLv2가 해설 콘텐츠를 막지 않음.

리스크와 대응: 시청층이 좁음 → "레트로 게임 / 브라우저에서 OS / C언어 마스터"로 넓게 포장. 어셈블리 난이도 → 실행 결과 먼저, 설명 나중 + 애니메이션. 크레딧 → 매 편 설명란에 원작자·저장소·공식 사이트·GPLv2 명시.

**커리큘럼 (총 39편 규모)**
- 시즌 0: 훅 숏폼 5편
- 시즌 1: 전체 투어 5편
- 시즌 2: 부트로더 6편
- 시즌 3: 커널 8편
- 시즌 4: GUI 7편 (`planar.c` 해부가 킬러 콘텐츠)
- 시즌 5: 실전 8편

---

## 5. 수익화 전략

### 5.1 GPLv2 규칙 (반드시 준수)

**가능:** 상업적 사용, 강의·책·영상 제작, 컨설팅·용역, 수정판 배포(소스 공개 + GPLv2 유지), 교육 키트 판매(SW 소스 공개 시), 굿즈, 독립 저작물 판매.

**불가:** 소스 비공개 수정판 판매, 라이선스 변경(MIT/상용), 원저작권 표시 제거, "내가 만들었다" 주장, GPL 코드를 독점 제품에 링크.

> **전략 원칙: 코드를 팔지 말고, 코드를 둘러싼 지식·서비스·경험을 팔 것.**

### 5.2 수익 모델 (ROI 순)

**TIER S**
1. **유료 온라인 강의** — 149,000~299,000원, 손익분기 20~30명, 연 1,000~5,000만원 목표. 한국어 OSDev 강의 희소 + 유데미로 글로벌 확장 가능
2. **유튜브 채널** — 광고/멤버십/스폰서십. 직접 수익보다 **강의 유입 채널로서의 가치가 핵심**
3. **기술 블로그 + 뉴스레터** — 비용 0, SEO 자산 복리

**TIER A**

4. **기술 서적** — 인세보다 "권위" 획득이 핵심 (강의 단가 2배, 컨설팅 3배)
5. **기업 교육 / 사내 강의** — 일 100~300만원. 임베디드·반도체·자동차 전장·방산·가전 펌웨어 대상. 단가 최고, 단 권위 선행 필요
6. **교육용 키트** — 15~30만원, 마진 40~60%, 학교/학원 B2B
7. **SaaS 커스텀 이미지 빌더** — Free/Pro($5)/Team($20). 3.x의 React+PHP 프로젝트가 바로 이것

**TIER B**

8. 1:1 멘토링 (시간당 5~15만원) · 9. 오픈소스 후원 · 10. 유료 커뮤니티 멤버십 ·
11. 굿즈/디지털 상품 · 12. React 레트로 UI 라이브러리(오픈코어) · 13. 유료 아티클 ·
14. 컨퍼런스 발표 → 용역 유입 · 15. **포트폴리오 → 이직 (연봉 +1,000~3,000만원, 가장 확실한 ROI)**

### 5.3 로드맵

| Phase | 기간 | 내용 | 투자 | 수익 |
|---|---|---|---|---|
| 1. 기반 | 1~2개월 | 빌드 성공, 스도쿠 패치, 블로그 5편, 숏폼 5편 | 0원 | 0 |
| 2. 자산화 | 3~6개월 | 유튜브 시즌1~2, React 변환기 공개, 뉴스레터 | 50~150만원 | 월 5~30만원 |
| 3. 수익화 | 7~12개월 | 유료 강의 출시, 멘토링, 기업 교육 영업, 출판 투고 | - | 월 100~500만원 |
| 4. 확장 | 1년+ | SaaS, 교육 키트, 기업 교육 반복 수주, 영어 확장 | - | 월 300~1,500만원 |

### 5.4 집중 추천

**유튜브(유입) → 유료 강의(수익) → 기업 교육(고단가)** 3단 파이프라인.

이번 주 할 일:
1. `docker compose run --rm dev make -j4` 빌드 성공시키기
2. `gentleos32-web.html` 화면 녹화 → 숏폼 1편 업로드
3. 스도쿠 등록 패치 커밋 (첫 OS 기여)

준수 사항:
- 모든 콘텐츠에 원작자 크레딧 + GPLv2 명시
- 포크 저장소 공개 유지
- "내가 만들었다"가 아니라 "분석/해설한다" 포지셔닝
- 원작자(luke8086)에게 이슈/메일로 미리 알리면 더 좋음

---

## 6. 학습 난이도별 실습 경로

| 레벨 | 소요 | 과제 |
|---|---|---|
| 🟢 1 | 10분 | `gentleos32-web.html` 열어서 앱/게임 체험 |
| 🟢 2 | 30분 | Docker로 직접 빌드 성공 |
| 🟡 3 | 1시간 | **스도쿠 런처 등록** (`gui/main.c` + `.icon` 한 줄씩) |
| 🟡 4 | 2시간 | MuseScore 악보 → `mkspk.py` → 플레이어에서 재생 |
| 🟠 5 | 반나절 | GIMP 팔레트 변환 → 내 월페이퍼 적용 |
| 🔴 6 | 며칠 | `apps/clock.c`(가장 짧음) 복사해서 새 앱 제작 |

새 앱 템플릿 구조:

```c
init_window()   // surface/window 설정, 콜백 연결
init_widgets()  // 버튼·그리드 배치
draw_window()   // 그리기
on_tick()       // 주기적 갱신 (TICK_FREQUENCY = 100Hz)
close_window()  // gui_wm_remove_window + heap_free
global app_st app_myapp = { .icon = &icon_myapp, .init = init_app };
```

---

## 7. GUI 구조와 현대 웹의 대응

| GentleOS/32 | React / 브라우저 |
|---|---|
| `surface` (오프스크린 픽셀 버퍼) | 가상 DOM |
| `window` | 컴포넌트 |
| `widget` | 하위 컴포넌트 |
| `on_pointer_up` 함수 포인터 | `onClick` 핸들러 |
| `gui_wm_render_window_region(r)` | 부분 리렌더링 |
| 이벤트 큐 + 단일 루프 | JavaScript 이벤트 루프 |
| `gui_fb_flush()` (큐 빌 때만) | 배치 렌더 커밋 |

30년 전 기술과 현대 프런트엔드의 뼈대가 동일하다는 점이 이 코드베이스의 교육적 가치다.

---

## 8. 웹 데모가 동작하는 원리

```
vendor/v86/v86.wasm       (2.1MB)  x86 CPU를 WebAssembly로 구현
vendor/v86/libv86.js      (357KB)  제어용 JS
vendor/v86/bios/*.bin              SeaBIOS + VGA BIOS
gentleos32-web.img                 OS 디스크 이미지
        ↓  tools/mkemu.py  (전부 base64 인라인)
gentleos32-web.html                단일 HTML 파일
```

브라우저에서: base64 디코드 → WASM이 가상 386 생성 → 가짜 BIOS 부팅 → 우리 부트로더 실행 → OS 기동.
`misc/emu-tpl.html`은 PC 스피커 오실레이터를 Web Audio 믹서에 연결(`node_oscillator.connect`)해 비퍼 음악까지 재생하고, 시리얼 출력을 페이지 하단 로그로 표시한다.

서버·설치·플러그인이 전혀 필요 없어, 링크 하나로 누구나 체험 가능하다 — 콘텐츠·포트폴리오·강의에서 가장 큰 무기.

---

## 9. 대화 진행 기록

| # | 요청 | 결과 |
|---|---|---|
| 1 | 전수조사 후 정체·용도·이점 분석 | 폴더 12개 / 소스 18,674줄 전수 확인. OS 본체로 확정. 스도쿠 미등록 발견 |
| 2 | 더 쉽게 상세 설명 | 집 비유, 부팅 타임라인, planar 최적화, SPK 음악 파이프라인, initrd, "없는 것들", 웹 데모 원리 |
| 3 | 질문 7개 답변 | 설치법 4가지 / OS임(플러그인·스킬·MCP 아님) / 토큰 불필요 / 에이전트엔 간접 유용 / React·PHP는 생태계 담당 / 유튜브 39편 커리큘럼 |
| 4 | 수익화 상세 | GPLv2 규칙 → 모델 15개 → 4단계 로드맵 → 3단 파이프라인 추천 |
| 5 | 정리 파일 생성 + 머지 | 이 문서 (`doc/ANALYSIS_KO.md`) |

---

## 10. 라이선스 고지

GentleOS/32는 명시되지 않은 경우를 제외하고 **GPLv2**로 배포된다 (`LICENSE` 참조).
`vendor/` 하위 에셋은 각 디렉터리의 `LICENSE*` / `README.txt`에 기재된 별도 조건을 따른다.

이 문서는 공개된 소스코드에 대한 **분석 및 해설 자료**이며, 원저작물의 저작권은 luke8086에게 있다.
