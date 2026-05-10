# TOPPERS/ASP サンプルプログラム集

LLMが TOPPERS/ASP カーネル向けのアプリケーションプログラムを書く際のリファレンスとして利用するサンプルプログラム集。教材 `material/` 配下のプログラムから核となるコード（`*.c`, `*.h`, `asp_prog.cfg`）のみを抜き出し，理解しやすいように整理した。各サンプルには README.md を付け，**何のサンプルか／なぜそう書くか**を解説する。

## 重要な前提

- TOPPERS/ASPカーネルは **μITRON 4.0仕様をベースとしているが完全互換ではない**。サンプル中の属性指定や API呼出は ASP仕様に従っており，μITRON 4.0準拠の文献の書き方とは異なる箇所があることに注意。差分は `../doc/TOPPERS-ASP_RTOS仕様.md` §1.1 参照。
- **すべてのカーネルオブジェクトは `.cfg` の静的APIで生成**する。動的生成API（`acre_*`, `del_*`, `AID_*`）は標準パッケージでは未サポート。各サンプルの `asp_prog.cfg` を雛形として再利用すること。

## 利用するスタイルガイドとの関係

`../doc/` 配下の以下5ファイルと併読する：

- `TOPPERS-ASP_RTOS仕様.md` — RTOSの基本概念・状態モデル・初期化／終了
- `TOPPERS-ASP_API仕様.md` — サービスコール（C言語API）一覧と機能仕様
- `TOPPERS-ASP_静的API_API仕様.md` — `.cfg` に書く静的API一覧
- `TOPPERS-ASP_静的API_エラー.md` — エラーコード一覧
- `TOPPERS-ASP_クイックルール.md` — 即参照用早見表

## サンプル一覧（学習順）

各サンプルは**前のサンプルとの差分**で理解できるよう構成されている。

### 第1章: タスク基礎

| サンプル | 学ぶこと | 主なAPI |
|---|---|---|
| [01_led_1task](01_led_1task/) | 1タスクの最小プログラム。`CRE_TSK` で生成し，無限ループでLEDを点滅 | `CRE_TSK`, `led_init`, `led_out` |
| [02_led_1task_syslog](02_led_1task_syslog/) | `syslog()` によるprintfデバッグ | `syslog` |

### 第2章: マルチタスク

| サンプル | 学ぶこと | 主なAPI |
|---|---|---|
| [03_led_2tasks_for](03_led_2tasks_for/) | 2タスクをforループで動かすと **片方しか動かない** ことを学ぶ（FCFSスケジューリング） | `CRE_TSK` ×2 |
| [04_led_2tasks](04_led_2tasks/) | `dly_tsk` でタスクを待ち状態にしてスケジューリングを回す | `dly_tsk` |
| [05_led_2tasks_init](05_led_2tasks_init/) | 初期化ルーチンを `ATT_INI` で登録 | `ATT_INI` |
| [06_led_2tasks_share](06_led_2tasks_share/) | 拡張情報 `exinf` で1つの関数を複数タスクで共有 | `CRE_TSK` の exinf |

### 第3章: タイマと割込み

| サンプル | 学ぶこと | 主なAPI |
|---|---|---|
| [07_led_cyc](07_led_cyc/) | 周期ハンドラ。`CRE_CYC` で定周期処理を実装 | `CRE_CYC` |
| [08_sw_int](08_sw_int/) | 割込みハンドラ。`CFG_INT` + `DEF_INH` で外部割込みを登録 | `CFG_INT`, `DEF_INH` |
| [09_sw_int2](09_sw_int2/) | 複数の割込みハンドラ登録 | `CFG_INT`×2, `DEF_INH`×2 |
| [10_sw_int_api](10_sw_int_api/) | 割込みハンドラからのAPI呼出（`iact_tsk`）。**`i` 接頭辞**の使い分け | `iact_tsk`, `SVC_PERROR` |
| [11_sw_int_api2](11_sw_int_api2/) | 割込みからの起床通知（`iwup_tsk` + `slp_tsk`） | `iwup_tsk`, `slp_tsk` |

### 第4章: 排他制御

| サンプル | 学ぶこと | 主なAPI |
|---|---|---|
| [12_cpu_lock](12_cpu_lock/) | CPUロック状態（割込み・ディスパッチ全停止） | `loc_cpu`, `unl_cpu` |
| [13_int_mask](13_int_mask/) | 割込み優先度マスク | `chg_ipm`, `sns_loc` |
| [14_ex_none](14_ex_none/) | 排他制御なし → カウンタの不一致を観察 | `irot_rdq` |
| [15_ex_sem](15_ex_sem/) | セマフォによる排他制御 | `CRE_SEM`, `wai_sem`, `sig_sem` |

### 第5章: イベント通知

| サンプル | 学ぶこと | 主なAPI |
|---|---|---|
| [16_sw_flg_cyc](16_sw_flg_cyc/) | イベントフラグでスイッチ状態を周期ハンドラから通知 | `CRE_FLG`, `iset_flg`, `wai_flg` |
| [17_sw_flg_tsk](17_sw_flg_tsk/) | 同上をタスクから通知（`set_flg`） | `set_flg` |
| [18_sw_dtq_cyc](18_sw_dtq_cyc/) | データキューでデータ送受信（周期ハンドラ送信） | `CRE_DTQ`, `ipsnd_dtq`, `rcv_dtq` |
| [19_sw_dtq_tsk](19_sw_dtq_tsk/) | 同上をタスクから送信（`psnd_dtq`） | `psnd_dtq` |

## ファイル構成（各サンプル共通）

各サンプルディレクトリには次のファイルが含まれる：

- `README.md` — サンプルの目的・解説・前サンプルとの差分・LLM向け要点
- `*.c` — Cソースファイル（タスク本体・ハンドラ等）
- `*.h` — ヘッダ（タスク優先度・スタックサイズ・関数プロトタイプ宣言等）
- `asp_prog.cfg` — システムコンフィギュレーションファイル

`Makefile` と `*.launch` は教材本体（`material/`）にある。例文として参照したいときは元教材を見ること。

## 共通の前提

以下は `material/` 配下のすべてのサンプルに共通する：

### 共通インクルードと共通モジュール

`asp_prog.cfg` の冒頭で常に以下のサポートモジュールを `INCLUDE` する：

```
INCLUDE("target_timer.cfg");      /* タイマ */
INCLUDE("syssvc/syslog.cfg");     /* syslog */
INCLUDE("syssvc/banner.cfg");     /* 起動メッセージ */
INCLUDE("syssvc/serial.cfg");     /* シリアル */
INCLUDE("syssvc/logtask.cfg");    /* ログタスク */
```

### デバイス操作関数（`device.h` で提供）

LED・スイッチ操作は教材で用意されたデバイスドライバ関数を使う：

| 関数 | 役割 |
|---|---|
| `void led_init(intptr_t exinf)` | LED初期化（引数は0） |
| `void led_out(unsigned char led_data)` | LED出力。`LED1`〜`LED4` のビットOR |
| `void switch_slide_init(intptr_t exinf)` | スライドスイッチ初期化 |
| `unsigned char switch_slide_sense(void)` | スライドスイッチ状態取得（`SW1`〜`SW4`） |
| `void switch_push_init(intptr_t exinf)` | プッシュスイッチ初期化 |
| `unsigned char switch_push_sense(void)` | プッシュスイッチ状態取得（`PUSH1`/`PUSH2`） |
| `void push1_int_init(intptr_t exinf)` | PUSH1割込み端子設定 |
| `void push1_int_clear(void)` | PUSH1割込みフラグクリア（割込みハンドラ内で呼ぶ） |
| `void push2_int_init(intptr_t exinf)` | PUSH2割込み端子設定 |
| `void push2_int_clear(void)` | PUSH2割込みフラグクリア |

`device.h` で `INTNO_PUSH1`, `INHNO_PUSH1`, `INTNO_PUSH2`, `INHNO_PUSH2` 等も定義済み。STM32F401をターゲットとする場合，PUSH1のINHNOは `23+16=39`（EXTI9_5），PUSH2は `40+16=56`（EXTI15_10）。

### LED・スイッチのビットマスク

```c
/* LED */
#define LED1  (1 << 8)
#define LED2  (1 << 7)
#define LED3  (1 << 6)
#define LED4  (1 << 5)

/* スライドスイッチ */
#define SW1  (1 << 3)
#define SW2  (1 << 4)
#define SW3  (1 << 5)
#define SW4  (1 << 6)

/* プッシュスイッチ */
#define PUSH1 (1 << 9)
#define PUSH2 (1 << 10)
```

## ヘッダファイルでの `TOPPERS_MACRO_ONLY` ガード

各サンプルのヘッダは次のパターンを取る：

```c
#define DEFAULT_PRIORITY  8       /* タスク優先度（マクロ） */
#define STACK_SIZE        4096    /* スタックサイズ（マクロ） */

#ifndef TOPPERS_MACRO_ONLY
extern void my_task(intptr_t exinf);   /* 関数プロトタイプ宣言（C言語の宣言） */
#endif /* TOPPERS_MACRO_ONLY */
```

理由: ヘッダは `.cfg` からも `#include` される。コンフィギュレータはマクロ定義以外のC言語記述を扱えないため，C言語の宣言は `#ifndef TOPPERS_MACRO_ONLY` で囲む必要がある。コンフィギュレータは `TOPPERS_MACRO_ONLY` を定義した状態で `.cfg` を処理する。

## SVC_PERRORマクロ

割込みハンドラやタスクからAPIを呼ぶときに，戻り値（エラーコード）をログ出力するマクロ。サンプルでは次のように定義して使う：

```c
#include <t_stdlib.h>

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)  svc_perror(__FILE__, __LINE__, #expr, (expr))

/* 使用例 */
SVC_PERROR(iact_tsk(LED1_TASK));
```

## LLM向けの読み方ガイド

LLMがこれらのサンプルを参照してコードを生成するとき：

1. **ターゲットの規模に合わせて選ぶ**: 1タスクなら `01`，マルチタスクなら `04`〜`06`，割込みありなら `08`〜`11`，イベント通知なら `16`〜`19`。
2. **`asp_prog.cfg` をベースに必要な静的APIを追加**する。
3. **タスクと非タスクコンテキストの違い**を意識する: 周期ハンドラ／割込みハンドラからは `i` 接頭辞付きAPIを使う（例: `iset_flg`, `ipsnd_dtq`, `iact_tsk`）。
4. **すべてのサンプルが「静的生成のみ」**で動いている。`acre_*` を使う必要はない。
5. **共有変数のアクセスは排他制御を考慮**する。マルチタスクから書き込む場合は `wai_sem`/`sig_sem` で囲む。割込みハンドラとの排他は `loc_cpu`/`unl_cpu` か割込み優先度マスクで行う。
