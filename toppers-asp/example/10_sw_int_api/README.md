# 10_sw_int_api — 割込みハンドラからのAPI呼出（iact_tsk）

## 何のサンプルか

PUSH1 割込みハンドラから **`iact_tsk`** を呼んで `LED1_TASK` を起動するプログラム。タスクは休止状態で生成しておき，PUSH1 が押されるたびに起動される。

学習ポイント：

- 割込みハンドラから API を呼ぶときは **`i` 接頭辞付き**を使う
- **`iact_tsk`** で休止状態のタスクを起動できる
- **`SVC_PERROR`** マクロで API の戻り値をログ出力
- タスクの**起動要求はキューイング**される（最大 `TMAX_ACTCNT`=1）
- 既に動作中のタスクへの起動要求はキューに入る

## 前サンプルとの差分

| 項目 | 09_sw_int2 | 10_sw_int_api |
|---|---|---|
| LED1_TASK の生成属性 | `TA_ACT`（自動起動） | `TA_NULL`（休止状態で生成） |
| LED1_TASK の動作 | 無限ループで点滅 | 4回点滅して終了 |
| PUSH1 ハンドラの処理 | LED3 反転 | `iact_tsk(LED1_TASK)` |
| `SVC_PERROR` | 未使用 | API呼出をラップ |

## なぜそう書くか

### LED1点滅タスク（休止状態で生成，起動されると4回点滅して終了）

```c
void
led1_task(intptr_t exinf){
    int i;

    syslog(LOG_NOTICE, "led1_task : start!");

    for (i = 0; i < 4; i++) {
        if ((led_data & LED1) == LED1) {
            led_data &= ~LED1;
        } else {
            led_data |= LED1;
        }
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
    }

    syslog(LOG_NOTICE, "led1_task : done!");
}
```

- `for` ループで4回点滅。
- 関数末尾でreturn → カーネルが自動的に `ext_tsk()` を呼び休止状態に戻す。
- 次に `iact_tsk` で起動されるまで動作しない。

### PUSH1 割込みハンドラ

```c
void
push1_handler(void){
    push1_int_clear();
    syslog(LOG_NOTICE, "push1_handler : start!");
    SVC_PERROR(iact_tsk(LED1_TASK));    /* タスク起動要求 */
}
```

- **`iact_tsk(tskid)`**: 非タスクコンテキストから対象タスクに起動要求を出す。タスクが休止状態なら実行できる状態に遷移。
- `SVC_PERROR` で戻り値を確認（エラー時にログ出力）。

### コンフィギュレーション

```
CRE_TSK(LED1_TASK, { TA_NULL, 0, led1_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
```

- `TA_NULL` を指定（`TA_ACT` を指定しない）→ **休止状態**で生成。
- `iact_tsk` で起動するまで `led1_task` は呼ばれない。

### SVC_PERROR マクロ定義

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
```

- API の戻り値が負（エラー）の場合のみ `t_perror` でログ出力する。
- ファイル名・行番号も出力されデバッグに有用。
- `Inline` は TOPPERS の標準マクロ（`inline` 相当）。

## なぜ `i` 接頭辞付きなのか

割込みハンドラは**非タスクコンテキスト**で実行される。サービスコールには2種類ある：

| API | コンテキスト |
|---|---|
| `act_tsk` | タスクコンテキスト専用 |
| `iact_tsk` | 非タスクコンテキスト専用 |

タスクコンテキストから `iact_tsk` を呼ぶ／非タスクコンテキストから `act_tsk` を呼ぶと **`E_CTX` エラー** で失敗する。

両コンテキストから呼べる `〔TI〕` 系API（例: `sns_*`, `loc_cpu`/`iloc_cpu`）はある。`i` 付きと無印を**呼ぶ側が選ぶ**点が FreeRTOS の `*FromISR` と異なる。

## 起動要求のキューイング

PUSH1を**連続して押す**とどうなるか：

1. 1回目: `LED1_TASK` 起動，動作開始
2. 2回目（動作中に）: 起動要求がキューイング（キュー数=1）
3. 3回目（動作中に）: **`E_QOVR`** （キューイングオーバフロー）

ASPでは **`TMAX_ACTCNT`=1** に固定されているので，動作中タスクへの起動要求は1つしかキューイングできない。動作中のタスクが終了すると，キューイングされた起動要求でもう一度起動される。

3回目の `iact_tsk` は `SVC_PERROR` のおかげで `E_QOVR` がコンソールに出力される。


## LLM向け要点

- 割込みハンドラから API を呼ぶときは **`i` 接頭辞付き**（`iact_tsk`, `iset_flg`, `isig_sem`, `ipsnd_dtq`, `iwup_tsk`）。
- API 呼出は **`SVC_PERROR()`** でラップして戻り値をチェック。
- タスクを **休止状態で生成**したいときは `CRE_TSK` の属性に `TA_ACT` を含めない（`TA_NULL`）。
- `iact_tsk` の起動要求はキューイング数 `TMAX_ACTCNT`=1。3回以上連続呼出すると `E_QOVR`。
- タスクのメインルーチンが `return` するとカーネルが自動的に `ext_tsk()` 同等の終了処理をする。
