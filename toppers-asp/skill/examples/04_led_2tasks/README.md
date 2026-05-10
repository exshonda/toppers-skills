# 04_led_2tasks — 2タスク（dly_tsk版）：正しいマルチタスク

## 何のサンプルか

`03_led_2tasks_for` の不具合（片方しか動かない）を `dly_tsk()` で修正した版。LED1を1秒間隔，LED3を0.25秒間隔で独立に点滅させる。

学習ポイント：

- **`dly_tsk(ms)`** で自タスクを待ち状態にする（ミリ秒単位）
- 待ち状態に入ると他のタスクが実行可能になる
- 複数タスクで時間制御を独立に行うときの定石

## 前サンプルとの差分

| 項目 | 03_led_2tasks_for | 04_led_2tasks |
|---|---|---|
| 時間生成 | `busy_wait()`（CPU占有） | `dly_tsk(ms)`（待ち状態） |
| LED1周期 | for ループ依存 | 1000 ms |
| LED3周期 | （実行されない） | 250 ms |
| マルチタスク動作 | × | ○ |

## なぜそう書くか

```c
#define LED1_INTERVAL 1000    /* 1000 ms */
#define LED3_INTERVAL 250     /* 250 ms */

void
led1_task(intptr_t exinf){
    led_init(0);
    for (;;) {
        led_data |= LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);   /* 1秒待機 → 他のタスクに切替 */
        led_data &= ~LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
    }
}

void
led3_task(intptr_t exinf){
    for (;;) {
        led_data |= LED3;
        led_out(led_data);
        dly_tsk(LED3_INTERVAL);
        led_data &= ~LED3;
        led_out(led_data);
        dly_tsk(LED3_INTERVAL);
    }
}
```

- `dly_tsk(ms)` を呼ぶとタスクは広義の待ち状態に遷移し，その間に他のタスクが実行される。
- 指定時間が経過するとタスクは実行できる状態に戻る。

## なぜ `dly_tsk` か

- 単位が**ミリ秒**で直感的。
- 自タスクのみ待ち状態にする（他のタスクに影響しない）。
- 同等のことを `tslp_tsk(ms)` でも実現できるが，`dly_tsk` は `wup_tsk` で起床できない（時間経過のみで起床する）違いがあり，純粋な遅延用途には `dly_tsk` が適切。

## なぜ排他制御していないのか

`led_data` を2タスクから書き込んでいるが，このサンプルでは排他制御していない。これは：

- LED1とLED3はビットが異なる（衝突しない）
- 各タスクは `led_data |= LED1`（または `&= ~LED1`）で **自分のビットのみ**を操作
- 仮に競合してもLED表示が一瞬おかしくなる程度で実害が小さい

ただし**厳密には Read-Modify-Write の競合**が起こりうる（タスク切り替えがビット操作中に入った場合）。本格的に排他制御する例は `15_ex_sem` を参照。

## LLM向け要点

- マルチタスク環境では **`dly_tsk` で時間を作る**のが基本。`busy_wait` は禁止。
- `dly_tsk(0)` は0msを意味しない（次のタイムティックまで待つ）。`dly_tsk(1)` 以上を使う。
- `dly_tsk` の引数の単位は **ミリ秒**。FreeRTOSの `vTaskDelay` の `pdMS_TO_TICKS` のような変換は不要。
- 共有変数の排他制御は次のサンプル群（`14`-`15`）で扱う。
