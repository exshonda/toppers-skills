# TOPPERS/ASP クイックルール

LLMがTOPPERS/ASPカーネル向けのアプリケーションコードと `.cfg` を**素早く正しく**生成するための簡潔なリファレンス。詳細版（`TOPPERS-ASP_RTOS仕様.md`, `TOPPERS-ASP_API仕様.md`, `TOPPERS-ASP_静的API_API仕様.md`, `TOPPERS-ASP_静的API_エラー.md`）と併読する。

## 重要: ASPはμITRON 4.0ベースだが完全互換ではない

ASPカーネルは μITRON 4.0仕様をベースとしているが，**主要な点で仕様が異なる**。μITRON 4.0準拠の文献やコードをそのまま流用するとビルドエラーになる。詳細は `TOPPERS-ASP_RTOS仕様.md` の §1.1 を参照。要点：

### 静的生成のみ（最重要）

**ASP標準パッケージではすべてのカーネルオブジェクトを `.cfg` の静的APIで生成する**。動的生成API（`acre_*`, `del_*`, `AID_*`）は**未サポート**。

| 機能 | 使う（静的） | 使えない（動的） |
|---|---|---|
| タスク・セマフォ・イベントフラグ・データキュー・ミューテックス・固定長メモリプール・周期ハンドラ・アラームハンドラ・ISR | `CRE_*`, `ATT_ISR` 等 | ~~`acre_*`~~, ~~`del_*`~~ |

### 廃止された値=0属性（μITRON互換用に `itron.h` のみ）

| 廃止 | 元の意味 | ASPでの正解 |
|---|---|---|
| `TA_TFIFO`(=0x00) | 待ち行列FIFO順 | 何も指定しない（属性ゼロは `TA_NULL`） |
| `TA_MFIFO`(=0x00) | メールボックスのメッセージFIFO順 | 何も指定しない |
| `TA_HLNG`(=0x00) | 高級言語インタフェース | 何も指定しない |
| `TA_WSGL`(=0x00) | 単一タスクのみ待ち | 何も指定しない（複数待ちは `TA_WMUL`） |

### 廃止／変更されたサービスコール

- `frsm_tsk` → `rsm_tsk` で代替（`itron.h` でマクロ定義）
- `set_tim` → 廃止（代替なし）
- `isig_tim` → 廃止（カーネル内部処理化）
- `TMAX_SUSCNT` → 1に固定（`itron.h` に定義移動）
- `ext_tsk` 非タスクコンテキスト呼出 → `E_CTX`
- `sta_cyc` 後最初の起動 → 起動位相基準（μITRONは起動周期基準）

### データ型の見直し（C99準拠）

- `B`, `H`, `W`, `UB`, `UH`, `UW` → `int8_t`, `int16_t`, `int32_t` 等（μITRON型は `itron.h`）
- 処理単位エントリ番地 `FP` → `TASK`, `TEXRTN`, `CYCHDR`, `ALMHDR` 等
- スタック領域 → `STK_T`，固定長メモリプール領域 → `MPF_T`

### ASP独自の追加機能（μITRON 4.0にない）

- 性能評価用時刻: `get_utm`
- カーネル制御: `ext_ker`, `sns_ker`
- 自タスク情報: `get_inf`
- 割込み制御: `chg_ipm`, `get_ipm`, `dis_int`, `ena_int`, `CFG_INT`, `ATT_ISR`
- 同期通信再初期化: `ini_sem`, `ini_flg`, `ini_dtq`, `ini_mbx`, `ini_mpf`
- 優先度データキュー: `CRE_PDQ`, `snd_pdq`, `rcv_pdq` 系一式
- CPU例外ハンドラ用: `xsns_dpn`, `xsns_xpn`
- 静的API: `ATT_TER`, `DEF_ICS`

### ヘッダ構成

- `t_services.h` / `s_services.h` は廃止
- アプリは `kernel.h`（カーネル機能）または `sil.h`（SIL機能）をインクルード
- 共通部のみは `t_stddef.h`
- μITRON 4.0互換型・定数が必要なときのみ `itron.h` を**明示的に**インクルード

## 0. FreeRTOS / Zephyr との対応表 — 差分注意点

TOPPERS/ASPは μITRON4.0 系の実装仕様であり，FreeRTOS や Zephyr とは概念が一部異なる。**安易にAPIを置き換えないこと**。以下は機能対応表 + ASPで特に注意すべき差分。

### 0.1 機能対応表

| 機能 | TOPPERS/ASP | FreeRTOS | Zephyr |
|---|---|---|---|
| タスク生成 | `CRE_TSK`（静的のみ） | `xTaskCreate` / `xTaskCreateStatic` | `K_THREAD_DEFINE` / `k_thread_create` |
| タスク起動 | `act_tsk(tskid)` | （生成と同時） | （生成と同時） |
| タスク終了 | `ext_tsk()` / return | `vTaskDelete(NULL)` | `k_thread_abort()` |
| 自タスクスリープ | `slp_tsk` / `tslp_tsk(tmout)` | `vTaskSuspend(NULL)` / `ulTaskNotifyTake` | `k_sleep()` |
| 別タスク起床 | `wup_tsk(tskid)` | `xTaskNotifyGive` | `k_wakeup()` |
| 遅延 | `dly_tsk(ms)` | `vTaskDelay(ticks)` | `k_msleep(ms)` |
| 優先度変更 | `chg_pri(tskid, pri)` | `vTaskPrioritySet` | `k_thread_priority_set` |
| バイナリセマフォ | `CRE_SEM(...{...,0,1})` + `wai_sem`/`sig_sem` | `xSemaphoreCreateBinary` | `K_SEM_DEFINE(...,0,1)` |
| カウンティングセマフォ | `CRE_SEM(...{...,init,max})` | `xSemaphoreCreateCounting` | `K_SEM_DEFINE` |
| ミューテックス | `CRE_MTX` + `loc_mtx`/`unl_mtx` | `xSemaphoreCreateMutex` | `K_MUTEX_DEFINE` + `k_mutex_lock`/`unlock` |
| イベントフラグ（複数ビット） | `CRE_FLG` + `set_flg`/`wai_flg` | `xEventGroupCreate` | `K_EVENT_DEFINE` |
| メッセージキュー | `CRE_DTQ` + `snd_dtq`/`rcv_dtq` | `xQueueCreate` | `K_MSGQ_DEFINE` |
| 優先度メッセージキュー | `CRE_PDQ` | （直接対応なし） | （直接対応なし） |
| メールボックス（ポインタ送受信） | `CRE_MBX` + `snd_mbx`/`rcv_mbx` | `xQueueCreate(1, ...)` | `K_MBOX_DEFINE` |
| 固定長メモリプール | `CRE_MPF` + `get_mpf`/`rel_mpf` | `pvPortMalloc` 系 | `K_MEM_SLAB_DEFINE` |
| 周期タイマ | `CRE_CYC(...{TA_STA,...})` | `xTimerCreate` | `K_TIMER_DEFINE` |
| 一発タイマ | `CRE_ALM` + `sta_alm` | `xTimerCreate(...,pdFALSE,...)` | `k_timer_start(timer, expiry, K_NO_WAIT)` |
| ISR登録 | `CFG_INT` + `ATT_ISR` | `IRQ_CONNECT`相当（ターゲット依存） | `IRQ_CONNECT` |
| ISRからのタスク通知 | `iset_flg` / `isig_sem` / `ipsnd_dtq` / `iact_tsk` / `iwup_tsk` | `xQueueSendFromISR` / `xSemaphoreGiveFromISR` | （通常APIから呼出可：内部判定） |
| クリティカルセクション | `loc_cpu`/`unl_cpu` または `dis_dsp`/`ena_dsp` | `taskENTER_CRITICAL`/`taskEXIT_CRITICAL` | `irq_lock`/`irq_unlock` |
| 起動 | カーネルが自動的に最高優先度タスクを起動 | `vTaskStartScheduler()` | （main_thread が起動） |
| 終了 | `ext_ker()` | （通常無し） | `sys_reboot` |

### 0.2 FreeRTOS/Zephyr ユーザが間違えやすい差分

1. **オブジェクトはすべて `.cfg` に静的記述**。FreeRTOSの `xTaskCreate` 相当はASPでは `CRE_TSK` のみ。動的生成（`acre_tsk`）はASP標準パッケージにはない。
2. **優先度は値が小さいほど高優先度**。FreeRTOS（値が大きいほど高）と逆。Zephyr（小さいほど高，協調スレッドは負）は同じ。
3. **時間単位はミリ秒（`SYSTIM`/`RELTIM`/`TMO` すべて ms）**。FreeRTOSのtick数とは異なる。`pdMS_TO_TICKS` のような変換は不要。
4. **ISR専用APIは `i` 接頭辞**。FreeRTOSの `*FromISR` / Zephyrの内部判定とは違って，**呼ぶ側が明示的に使い分ける**必要がある。間違えると `E_CTX`。
5. **割込み登録は2段階**: `CFG_INT`（属性設定）→ `ATT_ISR`（ハンドラ登録）。FreeRTOSのように1ステップで登録できない。
6. **スタックサイズはバイト指定**。FreeRTOS（`StackType_t`単位）と違ってバイト数。
7. **`E_OK = 0`**。FreeRTOSの `pdPASS = 1` / `pdFAIL = 0` とは逆の慣習。**負値が常にエラー**で正値はOK。
8. **`CRE_DTQ` のデータは `intptr_t`（整数1個分）**。FreeRTOSのキューのように任意サイズの構造体を直接送るには別途バッファ管理が必要。任意サイズの送信は `CRE_MBX` でメッセージへのポインタを送るのが定石。
9. **`dis_dsp` ≠ `taskENTER_CRITICAL`**。`dis_dsp` は割込みは入る（ディスパッチのみ禁止），FreeRTOSの `taskENTER_CRITICAL` は割込みもマスクする（実装依存）。完全な排他は `loc_cpu` を使う。
10. **タスクのメインルーチンから return しても問題ない**（カーネルが終了処理する）。FreeRTOSのように return 禁止ではない。だが明示的に `ext_tsk()` を書くのが推奨。
11. **`wup_tsk` の起床要求はキューイングされる**（最大 `TMAX_WUPCNT`）。`xTaskNotifyGive` のシグナル意味論と似ているが，ASPは複数回の `wup_tsk` で `slp_tsk` の再呼出時にも残る。
12. **イベントフラグのモード**: FreeRTOSの `xEventGroupWaitBits` のように `xClearOnExit` 引数で個別にクリアできない。ASPでは生成時に `TA_CLR` 属性で「待ち解除時に全クリア」を指定するか，`clr_flg` で明示クリアする。
13. **割込み優先度は負値**。FreeRTOSやZephyrとは符号が逆。`-1` が最高，`TMIN_INTPRI`（負の最小値）まで。

## 1. 必読のグローバルルール

LLMがコード生成するときに必ず守ること：

### 1.1 ファイル構成

- `*.c`/`*.h`: アプリケーションのソース・ヘッダ
- `*.cfg`: システムコンフィギュレーションファイル（**コンフィギュレータが処理してビルドに渡す**）
- アプリ固有ヘッダ（例: `myapp.h`）には `.cfg` と `*.c` の両方からインクルードする宣言・定数を集める

### 1.2 必須インクルード

```c
/* アプリケーション側 */
#include <kernel.h>      /* カーネルAPI（先頭で必ずインクルード） */
#include <t_syslog.h>    /* syslog 用，必要なら */
#include "kernel_cfg.h"  /* オブジェクト識別名のID定義 */
#include "myapp.h"       /* アプリ固有定数 */
```

```c
/* .cfg 側 */
#include <kernel.h>
#include "myapp.h"       /* タスク優先度・スタックサイズ等のマクロを参照する場合 */
```

### 1.3 命名規則（推奨）

| 種類 | 形式 | 例 |
|---|---|---|
| タスク識別名 | `xxx_TASK` または `TASK_xxx` | `MAIN_TASK` |
| タスク関数名 | `xxx_task` | `main_task` |
| セマフォ識別名 | `SEM_xxx` | `SEM_BUTTON` |
| イベントフラグ識別名 | `FLG_xxx` | `FLG_EVENT` |
| データキュー識別名 | `DTQ_xxx` | `DTQ_LOG` |
| ミューテックス識別名 | `MTX_xxx` | `MTX_LCD` |
| 周期ハンドラ関数名 | `xxx_cyclic_handler` | `heartbeat_handler` |
| ISR関数名 | `xxx_isr` | `button_isr` |

### 1.4 コンテキスト判定の鉄則

| 呼び出し元 | 使うAPI |
|---|---|
| タスク・タスク例外処理ルーチン | `i` 接頭辞 **なし** のAPI（例: `act_tsk`, `sig_sem`, `set_flg`, `psnd_dtq`） |
| 割込みハンドラ・ISR・周期ハンドラ・アラームハンドラ・CPU例外ハンドラ | `i` 接頭辞 **付き** のAPI（例: `iact_tsk`, `isig_sem`, `iset_flg`, `ipsnd_dtq`） |

`〔TI〕` と書かれたAPI（`sns_*`, `loc_cpu`/`iloc_cpu`，`get_utm` 等）は両方から呼べるが，i付きはコンテキスト・センシティブな状態（割込み制御等）の操作で必要なら使う。

**待ち状態に入るAPIは絶対にISR/周期ハンドラから呼ばない**：`wai_sem`, `tslp_tsk`, `wai_flg`, `loc_mtx`, `rcv_dtq`, `dly_tsk` 等。`E_CTX` で必ず失敗する。

## 2. よく使うパターン集

### 2.1 タスクの作成と起動

```c
/* myapp.h */
#define MAIN_TASK_PRI  5
#define STACK_SIZE     1024
extern void main_task(intptr_t exinf);
```

```c
/* myapp.cfg */
#include <kernel.h>
#include "myapp.h"

DEF_ICS({ 1024, NULL });        /* 非タスクコンテキスト用スタック */

CRE_TSK(MAIN_TASK,
        { TA_ACT, 0, main_task, MAIN_TASK_PRI, STACK_SIZE, NULL });
```

```c
/* myapp.c */
#include <kernel.h>
#include "kernel_cfg.h"
#include "myapp.h"

void main_task(intptr_t exinf)
{
    while (1) {
        /* 周期実行 */
        dly_tsk(100);
    }
    ext_tsk();   /* 通常到達しない */
}
```

### 2.2 セマフォによるイベント通知（ISR → タスク）

```c
/* myapp.cfg */
CFG_INT(INTNO_BUTTON,
        { TA_ENAINT | TA_EDGE, BUTTON_INT_PRI });
ATT_ISR({ TA_NULL, 0, INTNO_BUTTON, button_isr, 1 });

CRE_SEM(SEM_BUTTON, { TA_NULL, 0, 1 });    /* バイナリセマフォ */

CRE_TSK(BUTTON_TASK,
        { TA_ACT, 0, button_task, 5, 1024, NULL });
```

```c
/* myapp.c */
void button_isr(intptr_t exinf)
{
    /* デバイスフラグクリア */
    button_clear_irq();
    isig_sem(SEM_BUTTON);   /* タスクへ通知 */
}

void button_task(intptr_t exinf)
{
    while (1) {
        wai_sem(SEM_BUTTON);
        /* ボタン処理 */
        process_button();
    }
}
```

### 2.3 イベントフラグによる多イベント通知

```c
/* myapp.h */
#define BIT_BUTTON_PRESSED  0x01U
#define BIT_TIMEOUT         0x02U
#define BIT_ERROR           0x04U
```

```c
/* myapp.cfg */
CRE_FLG(FLG_EVENT, { TA_WMUL, 0 });
```

```c
/* myapp.c */
void event_task(intptr_t exinf)
{
    FLGPTN flgptn;
    while (1) {
        wai_flg(FLG_EVENT,
                BIT_BUTTON_PRESSED | BIT_TIMEOUT | BIT_ERROR,
                TWF_ORW,
                &flgptn);
        if (flgptn & BIT_BUTTON_PRESSED) {
            handle_button();
            clr_flg(FLG_EVENT, ~BIT_BUTTON_PRESSED);
        }
        if (flgptn & BIT_TIMEOUT) {
            handle_timeout();
            clr_flg(FLG_EVENT, ~BIT_TIMEOUT);
        }
        if (flgptn & BIT_ERROR) {
            handle_error();
            clr_flg(FLG_EVENT, ~BIT_ERROR);
        }
    }
}

/* イベント発生側（ISR等） */
void some_isr(intptr_t exinf)
{
    iset_flg(FLG_EVENT, BIT_BUTTON_PRESSED);
}
```

### 2.4 ミューテックスによる排他制御

```c
/* myapp.cfg */
CRE_MTX(MTX_LCD, { TA_CEILING, LCD_CEIL_PRI });
```

```c
/* myapp.c */
void update_lcd(const char *msg)
{
    ER ercd = loc_mtx(MTX_LCD);
    if (ercd == E_OK) {
        lcd_write(msg);
        unl_mtx(MTX_LCD);
    }
}
```

`TA_CEILING` 属性で優先度上限プロトコルを使うのが推奨（優先度逆転の抑制）。`LCD_CEIL_PRI` はLCD排他を取りうるタスクの中で最高優先度の値以下を指定。

### 2.5 データキューによるメッセージパッシング

```c
/* myapp.cfg */
CRE_DTQ(DTQ_LOG, { TA_NULL, 16, NULL });   /* 容量16要素 */

CRE_TSK(LOG_TASK, { TA_ACT, 0, log_task, 10, 1024, NULL });
```

```c
/* myapp.c */
void some_task(intptr_t exinf)
{
    intptr_t msg = (intptr_t)0xDEADBEEF;
    psnd_dtq(DTQ_LOG, msg);   /* ノンブロッキング送信（ポーリング） */
}

void log_task(intptr_t exinf)
{
    while (1) {
        intptr_t data;
        rcv_dtq(DTQ_LOG, &data);   /* 永久待ち */
        process_log(data);
    }
}
```

### 2.6 周期ハンドラ

```c
/* myapp.cfg */
CRE_CYC(CYC_HEARTBEAT,
        { TA_STA, 0, heartbeat_handler, 1000, 1 });   /* 1秒周期 */
```

```c
/* myapp.c */
void heartbeat_handler(intptr_t exinf)
{
    /* 非タスクコンテキストで実行 */
    iset_flg(FLG_EVENT, BIT_HEARTBEAT);
}
```

### 2.7 アラームハンドラ（一発タイマ）

```c
/* myapp.cfg */
CRE_ALM(ALM_TIMEOUT, { TA_NULL, 0, timeout_handler });
```

```c
/* myapp.c */
void start_timeout(RELTIM ms)
{
    sta_alm(ALM_TIMEOUT, ms);   /* ms 後に timeout_handler が呼ばれる */
}

void cancel_timeout(void)
{
    stp_alm(ALM_TIMEOUT);
}

void timeout_handler(intptr_t exinf)
{
    iset_flg(FLG_EVENT, BIT_TIMEOUT);
}
```

### 2.8 ISRからタスクを起動するパターン

```c
/* myapp.cfg */
CRE_TSK(WORKER_TASK,
        { TA_NULL, 0, worker_task, 5, 1024, NULL });   /* 起動なしで生成 */
ATT_ISR({ TA_NULL, 0, INTNO_X, trigger_isr, 1 });
```

```c
/* myapp.c */
void trigger_isr(intptr_t exinf)
{
    iact_tsk(WORKER_TASK);   /* タスク起動要求 */
}

void worker_task(intptr_t exinf)
{
    /* 起動ごとに1回実行されて休止状態に戻る */
    process_event();
    /* return か ext_tsk() */
}
```

### 2.9 固定長メモリプールでブロック確保

```c
/* myapp.cfg */
CRE_MPF(MPF_PACKET,
        { TA_NULL, 8, sizeof(packet_t), NULL, NULL });
```

```c
/* myapp.c */
void allocate_packet(void)
{
    packet_t *p;
    ER ercd = pget_mpf(MPF_PACKET, (void **)&p);   /* ポーリング取得 */
    if (ercd == E_OK) {
        memset(p, 0, sizeof(*p));
        /* p を使う */
        rel_mpf(MPF_PACKET, p);   /* 必ず返却 */
    }
}
```

### 2.10 初期化／終了処理ルーチン

```c
/* myapp.cfg */
ATT_INI({ TA_NULL, 0, app_init });
ATT_TER({ TA_NULL, 0, app_term });
```

```c
/* myapp.c */
void app_init(intptr_t exinf)
{
    /* デバイス・ペリフェラル初期化 */
    /* サービスコールはほぼ呼べない（カーネル動作前） */
    init_uart();
    init_gpio();
}

void app_term(intptr_t exinf)
{
    /* 後始末（カーネル動作終了後） */
    deinit_gpio();
    deinit_uart();
}
```

## 3. NGパターン集（LLMが避けるべきコード）

### 3.1 ISRから待ちに入るAPI

```c
/* ❌ 絶対NG: ISRからは待ちに入れない */
void bad_isr(intptr_t exinf)
{
    wai_sem(SEM_X);   /* E_CTX */
    tslp_tsk(100);    /* E_CTX */
    rcv_dtq(DTQ_X, &data);   /* E_CTX */
}

/* ✅ OK: i接頭辞付きの即時APIを使う */
void good_isr(intptr_t exinf)
{
    isig_sem(SEM_X);
    iwup_tsk(SOME_TASK);
    ipsnd_dtq(DTQ_X, data);   /* ポーリング送信 */
}
```

### 3.2 `i`接頭辞を間違える

```c
/* ❌ NG: タスクから i 付きを呼ぶ */
void task(intptr_t exinf) {
    iact_tsk(OTHER_TASK);    /* E_CTX: タスクから iact_tsk は呼べない */
}

/* ✅ OK */
void task(intptr_t exinf) {
    act_tsk(OTHER_TASK);
}
```

### 3.3 CPUロック中に許されないAPI

```c
/* ❌ NG */
loc_cpu();
sig_sem(SEM_X);   /* E_CTX */
unl_cpu();

/* ✅ OK: CPUロック中は最小限の操作のみ */
loc_cpu();
/* 短いクリティカルセクション */
unl_cpu();
sig_sem(SEM_X);
```

### 3.4 動的生成APIをアプリに含める

```c
/* ❌ NG: ASP標準パッケージでは acre_*, del_* は使えない */
ER_ID tskid = acre_tsk(&tcb);

/* ✅ OK: .cfg で静的生成 */
/* myapp.cfg: CRE_TSK(MY_TASK, {...}); */
act_tsk(MY_TASK);
```

### 3.5 サポート外の属性

```c
/* ❌ NG: TA_PHS は ASP では未サポート */
CRE_CYC(CYC_X, { TA_STA | TA_PHS, 0, h, 1000, 0 });

/* ✅ OK */
CRE_CYC(CYC_X, { TA_STA, 0, h, 1000, 1 });
```

### 3.6 ミューテックスのアンロック忘れ

```c
/* ❌ NG: 早期リターンでアンロック忘れ */
void bad_func(void) {
    loc_mtx(MTX_X);
    if (some_error) return;   /* MTX_X がロックされたまま */
    do_something();
    unl_mtx(MTX_X);
}

/* ✅ OK: すべての出口で確実にアンロック */
void good_func(void) {
    ER ercd = loc_mtx(MTX_X);
    if (ercd != E_OK) return;
    if (some_error) {
        unl_mtx(MTX_X);
        return;
    }
    do_something();
    unl_mtx(MTX_X);
}
```

### 3.7 優先度を逆向きに

```c
/* ❌ NG: 「数字を大きくすれば優先度が上がる」と勘違い */
#define HIGH_PRIORITY  20   /* 実は低優先度 */
#define LOW_PRIORITY   5    /* 実は高優先度 */

/* ✅ OK: 値が小さいほど高優先度 */
#define HIGH_PRIORITY  1
#define MEDIUM_PRIORITY 5
#define LOW_PRIORITY   10
```

### 3.8 時間単位の混同

```c
/* ❌ NG: tick数を渡そうとする */
dly_tsk(pdMS_TO_TICKS(100));   /* マクロも値も意味が違う */

/* ✅ OK: ミリ秒で直接指定 */
dly_tsk(100);   /* 100 ms */
twai_sem(SEM_X, 500);   /* 500 ms タイムアウト */
```

### 3.9 dly_tsk(0)

```c
/* ❌ NG: 0は意味が異なる（次のタイムティックまで待ち） */
dly_tsk(0);

/* ✅ OK: 1 ms（少なくとも1ティック） */
dly_tsk(1);
```

### 3.10 タスク本体で無限ループせず return

```c
/* ❌ NG: 1度しか実行されない（戻ったら休止状態） */
void task_loop_missing(intptr_t exinf) {
    process_once();
    /* return → 休止状態。再起動するまで動かない */
}

/* ✅ OK: 永続実行するタスクはループ */
void task(intptr_t exinf) {
    while (1) {
        wai_sem(SEM_TRIGGER);
        process_event();
    }
}
```

## 4. クイックチェックリスト（コード生成時の最終確認）

LLMがTOPPERS/ASPコードを出力する前に必ず確認：

- [ ] `kernel.h` を最初にインクルードしているか
- [ ] オブジェクト生成は `.cfg` の静的APIか（動的生成APIを書いていないか）
- [ ] 動的生成・保護機能・マルチプロセッサ系のAPIを使っていないか
- [ ] ISR・周期ハンドラ・アラームハンドラから待ちに入るAPIを呼んでいないか
- [ ] ISR・周期ハンドラ・アラームハンドラから呼ぶAPIに `i` 接頭辞が付いているか
- [ ] `CFG_INT` → `ATT_ISR` の順で記述しているか
- [ ] `DEF_ICS` を1回だけ書いているか
- [ ] タスクのメインルーチンが無限ループまたは `ext_tsk()` で終了しているか
- [ ] ミューテックスはすべての出口で `unl_mtx` しているか
- [ ] 優先度の値が小さいほど高優先度であることを正しく扱っているか
- [ ] 時間単位がミリ秒であることを正しく扱っているか
- [ ] `TA_PHS`（CRE_CYC）等のASP未サポート属性を使っていないか
- [ ] エラーコードを `ER` 型で受け，必要なら判定しているか
- [ ] アプリのスタックサイズが妥当か（ターゲット定義の最小値以上）

## 5. 困ったときの参照ガイド

- 概念・状態モデル・初期化／終了：`TOPPERS-ASP_RTOS仕様.md`
- C言語サービスコールの引数・返値・コンテキスト：`TOPPERS-ASP_API仕様.md`
- `.cfg` の静的API書式・パラメータ：`TOPPERS-ASP_静的API_API仕様.md`
- エラーコードの意味と発生条件：`TOPPERS-ASP_静的API_エラー.md`
- ターゲット依存（最小スタックサイズ・割込み番号・FPU属性等）：そのターゲット依存部仕様書

## 6. ベンダー間ローカライズ早見表（FreeRTOS/Zephyr → ASP）

LLMがFreeRTOS/Zephyrの既存コードをTOPPERS/ASPに移植する際の置き換えガイド。

### 6.1 タスク

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `xTaskCreate(fn, "name", stk, arg, pri, &h)` | `K_THREAD_DEFINE(name, stk, fn, arg, ..., pri, ...)` | `.cfg` で `CRE_TSK(NAME, { TA_ACT, arg, fn, pri, stk, NULL });` |
| `vTaskDelay(pdMS_TO_TICKS(ms))` | `k_msleep(ms)` | `dly_tsk(ms)` |
| `vTaskDelete(NULL)` | `k_thread_abort(k_current_get())` | `ext_tsk()` |
| `vTaskSuspend(h)` | `k_thread_suspend` | `sus_tsk(TASKID)` |
| `vTaskResume(h)` | `k_thread_resume` | `rsm_tsk(TASKID)` |
| `vTaskPrioritySet(h, p)` | `k_thread_priority_set(t, p)` | `chg_pri(TASKID, p)` |

### 6.2 セマフォ

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `s = xSemaphoreCreateBinary()` | `K_SEM_DEFINE(s, 0, 1)` | `CRE_SEM(SEM_NAME, { TA_NULL, 0, 1 });` |
| `xSemaphoreGive(s)` | `k_sem_give(&s)` | `sig_sem(SEM_NAME)` |
| `xSemaphoreTake(s, portMAX_DELAY)` | `k_sem_take(&s, K_FOREVER)` | `wai_sem(SEM_NAME)` |
| `xSemaphoreTake(s, 0)` | `k_sem_take(&s, K_NO_WAIT)` | `pol_sem(SEM_NAME)` |
| `xSemaphoreTake(s, ms)` | `k_sem_take(&s, K_MSEC(ms))` | `twai_sem(SEM_NAME, ms)` |
| `xSemaphoreGiveFromISR(s, NULL)` | `k_sem_give(&s)` | `isig_sem(SEM_NAME)` |

### 6.3 ミューテックス

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `m = xSemaphoreCreateMutex()` | `K_MUTEX_DEFINE(m)` | `CRE_MTX(MTX_NAME, { TA_NULL, 0 });` |
| `xSemaphoreTake(m, ...)` | `k_mutex_lock(&m, K_FOREVER)` | `loc_mtx(MTX_NAME)` |
| `xSemaphoreGive(m)` | `k_mutex_unlock(&m)` | `unl_mtx(MTX_NAME)` |

### 6.4 イベントフラグ

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `g = xEventGroupCreate()` | `K_EVENT_DEFINE(e)` | `CRE_FLG(FLG_NAME, { TA_WMUL, 0 });` |
| `xEventGroupSetBits(g, b)` | `k_event_post(&e, b)` | `set_flg(FLG_NAME, b)` |
| `xEventGroupClearBits(g, b)` | `k_event_set_masked(&e, 0, b)` | `clr_flg(FLG_NAME, ~b)` |
| `xEventGroupWaitBits(g, b, pdTRUE, pdTRUE, ...)` | `k_event_wait_all(&e, b, false, ...)` | `wai_flg(FLG_NAME, b, TWF_ANDW, &p)` |
| `xEventGroupSetBitsFromISR` | （内部判定） | `iset_flg(FLG_NAME, b)` |

### 6.5 メッセージキュー

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `q = xQueueCreate(n, sizeof(t))` | `K_MSGQ_DEFINE(q, sizeof(t), n, 4)` | `CRE_DTQ(DTQ_NAME, { TA_NULL, n, NULL });` ※ データは intptr_t 1個まで |
| `xQueueSend(q, &v, 0)` | `k_msgq_put(&q, &v, K_NO_WAIT)` | `psnd_dtq(DTQ_NAME, (intptr_t)v)` |
| `xQueueReceive(q, &v, ...)` | `k_msgq_get(&q, &v, K_FOREVER)` | `rcv_dtq(DTQ_NAME, &data)` |
| `xQueueSendFromISR(q, &v, NULL)` | k_msgq_put（内部判定） | `ipsnd_dtq(DTQ_NAME, v)` |

任意サイズの構造体を送るときは：
- ポインタを送る → `CRE_MBX`（メールボックス）でメッセージ構造体先頭が `T_MSG`
- バイト列を送る → 別途バッファプール+データキューで管理

### 6.6 タイマ

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `xTimerCreate(... pdTRUE ...)` で周期 | `K_TIMER_DEFINE` + `k_timer_start(&t, expiry, period)` | `CRE_CYC(CYC_NAME, { TA_STA, 0, fn, period_ms, phase_ms });` |
| `xTimerCreate(... pdFALSE ...)` でワンショット | `k_timer_start(&t, expiry, K_NO_WAIT)` | `CRE_ALM(ALM_NAME, { TA_NULL, 0, fn });` + `sta_alm(ALM_NAME, ms)` |
| `xTimerStart` | `k_timer_start` | `sta_cyc`/`sta_alm` |
| `xTimerStop` | `k_timer_stop` | `stp_cyc`/`stp_alm` |

### 6.7 ISRと割込み

| FreeRTOS | Zephyr | TOPPERS/ASP |
|---|---|---|
| `IRQ_CONNECT(...)` 相当 | `IRQ_CONNECT(irq, pri, isr, arg, flags)` | `.cfg` で `CFG_INT` + `ATT_ISR` |
| `taskENTER_CRITICAL` | `irq_lock()` | `loc_cpu()` |
| `taskEXIT_CRITICAL` | `irq_unlock(key)` | `unl_cpu()` |

## 7. まとめ — TOPPERS/ASP特有の流儀

1. **静的構成主義**: すべてのリソースは `.cfg` に書く。実行時に増えない。
2. **コンテキスト明示**: ISR用APIは `i` で始まる別名。ユーザが選ぶ。
3. **優先度は小さいほど強い**: μITRON系の慣習。`PRI_HIGH < PRI_LOW`。
4. **時間はミリ秒**: 単位変換の悩みなし。`dly_tsk(100)` で100ms待ち。
5. **エラーは負値**: 正常は `E_OK=0` または正値。`if (ercd < 0)` でエラー判定。
6. **ID番号はマクロ**: `kernel_cfg.h` 経由で参照。数値リテラルは書かない。
7. **生成と削除は静的**: ASP標準では `del_*` も `acre_*` も無い。
8. **保護なし・MP対応なし**: ASPはシンプルなフラットRTOS。HRP2/FMP系のAPIは混ぜない。
