# 01_led_1task — 1タスクLED点滅プログラム

## 何のサンプルか

TOPPERS/ASP の **最も基本的な構成**。1個のタスクが無限ループでLED1を点滅させるだけのプログラム。

学習ポイント：

- タスクの記述形式（`void task(intptr_t exinf)`）
- `CRE_TSK` 静的APIによるタスク生成
- `TA_ACT` 属性で生成と同時にタスクを起動できる
- `kernel.h` と `kernel_cfg.h` のインクルード

## ファイル構成

| ファイル | 役割 |
|---|---|
| `led_1task.h` | ヘッダ（環境依存：除外） |
| `led_1task.c` | タスク本体 |
| `asp_prog.cfg` | コンフィギュレーションファイル |

ヘッダファイル（`*.h`）の内容（タスク優先度・スタックサイズ・関数プロトタイプ宣言など）は環境（ターゲット・ビルド構成）ごとに異なるため，本サンプル群では説明対象外とする。共通の記述様式（`#ifndef TOPPERS_MACRO_ONLY` ガード）については `../README.md` を参照。

## なぜそう書くか

### `led_1task.c`

```c
#include <kernel.h>            /* TOPPERS/ASP 標準ヘッダ */
#include "kernel_cfg.h"        /* コンフィギュレータ生成（オブジェクトIDのマクロ） */
#include "device.h"            /* LED/スイッチ操作関数 */
#include "led_1task.h"         /* このプログラム固有のヘッダ */
```

- `kernel.h` でカーネルAPIの宣言を取り込む。
- `kernel_cfg.h` には `LED1_TASK` などのオブジェクトIDマクロが定義されている（コンフィギュレータがビルド時に生成）。
- `device.h` には `led_init`, `led_out`, `LED1` 等のドライバ関数・マクロが定義。

タスク本体：

```c
void
led1_task(intptr_t exinf){
    led_init(0);              /* LED初期化 */

    for (;;) {                /* タスクは無限ループにする */
        led_out(LED1);        /* LED1を点灯 */
        busy_wait();
        led_out(0x00);        /* LED1を消灯 */
        busy_wait();
    }
}
```

- タスク関数のシグネチャは `void name(intptr_t exinf)` で固定。
- 引数 `exinf` には `CRE_TSK` で指定した拡張情報が渡される（このサンプルでは未使用）。
- **無限ループ必須**。リターンするとタスクは休止状態になり，再度起動するまで実行されない。
- `busy_wait()` は `for(i=0;i<DELAY_LOOP;i++);` の単純なビジーウェイト。CPUを占有するため次のサンプル `04_led_2tasks` 以降は `dly_tsk()` を使う。

### `asp_prog.cfg`

```
INCLUDE("target_timer.cfg");
INCLUDE("syssvc/syslog.cfg");
INCLUDE("syssvc/banner.cfg");
INCLUDE("syssvc/serial.cfg");
INCLUDE("syssvc/logtask.cfg");

#include "led_1task.h"

CRE_TSK(LED1_TASK, { TA_ACT, 0, led1_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
```

- 冒頭の `INCLUDE("...")` はコンフィギュレータに対するインクルード。サポートモジュール（タイマ・syslog・シリアル等）の `.cfg` を取り込む。
- `#include "led_1task.h"` でアプリ固有の `DEFAULT_PRIORITY`, `STACK_SIZE` 等のマクロを参照する。
- `CRE_TSK(tskid, { tskatr, exinf, task, itskpri, stksz, stk })` でタスクを生成。
  - `LED1_TASK` — タスク識別名（`kernel_cfg.h` にマクロとして出力される）
  - `TA_ACT` — 生成と同時に起動（実行できる状態にする）。指定しないと休止状態で生成される
  - `0` — 拡張情報（タスク本体に `intptr_t exinf` として渡される）
  - `led1_task` — タスクのメインルーチンの先頭番地
  - `DEFAULT_PRIORITY` — 起動時優先度（数字が小さいほど高優先度）
  - `STACK_SIZE` — スタックサイズ（バイト）
  - `NULL` — スタック領域。`NULL` ならカーネルが確保

## LLM向け要点

- **タスクは無限ループ**で書く。途中でreturnしない。
- **`TA_ACT`** を指定すると生成と同時にタスクが実行できる状態になる。指定しなければ休止状態で生成される（`act_tsk` で起動）。
- タスク優先度は **値が小さいほど高優先度**。ASPでは1〜TMAX_TPRIの範囲。
