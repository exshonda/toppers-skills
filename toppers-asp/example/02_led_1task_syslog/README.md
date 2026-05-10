# 02_led_1task_syslog — syslog によるprintfデバッグ

## 何のサンプルか

`01_led_1task` に **`syslog()` 関数** を追加してデバッグログを出力する版。

学習ポイント：

- TOPPERS/ASP では `printf` の代わりに **`syslog()`** を使う
- `<t_syslog.h>` のインクルードが必要
- ログレベル指定とフォーマット文字列

## 前サンプルとの差分

| 項目 | 01_led_1task | 02_led_1task_syslog |
|---|---|---|
| インクルード | `kernel.h`, `device.h` | + `<t_syslog.h>` |
| LED点灯/消灯時 | `led_out()` のみ | `syslog()` でログ出力も追加 |

## なぜそう書くか

```c
#include <t_syslog.h>            /* syslog 関数の宣言 */
```

- `syslog()` を使うには `<t_syslog.h>` のインクルードが必須。

```c
syslog(LOG_NOTICE, "led1_task : out_put 0x%x", LED1);
led_out(LED1);
```

- `syslog(prio, format, ...)`: 第1引数はログレベル（`LOG_EMERG`, `LOG_ALERT`, `LOG_CRIT`, `LOG_ERROR`, `LOG_WARNING`, `LOG_NOTICE`, `LOG_INFO`, `LOG_DEBUG`）。
- フォーマット文字列に続く引数は **最大5個まで**（`printf` と異なる制限）。
- 出力はシリアル経由で TeraTerm 等のターミナルに届く（`asp_prog.cfg` で `syslog.cfg` と `serial.cfg` が `INCLUDE` されているため）。

`asp_prog.cfg` は `01` と同一。`syslog` を使うためのモジュールは元々取り込まれている。

## LLM向け要点

- **`printf` ではなく `syslog`** を使う。標準C関数の `printf` は ASP のサンプルプロファイルでは未提供。
- フォーマット引数は **最大5個**まで。
- マルチタスク環境ではどのタスクから出力されたか分かるよう **タスク名やタスクIDを最初に書く**のが推奨（例: `"led1_task : ..."`）。
- 割込みハンドラ・周期ハンドラ等の非タスクコンテキストからも `syslog` は呼べる（内部で `i` 接頭辞付き処理に切り替わる）。
