# 03_led_2tasks_for — 2タスク(forループ版)：FCFSスケジューリングの罠

## 何のサンプルか

**意図的に「動かない」サンプル**。LED1点滅タスクとLED3点滅タスクを2つ作るが，両者ともビジーループ（`for` で時間を作る）を使っているため，**片方のLEDしか点滅しない**。

学習ポイント：

- 同一優先度のタスクは **FCFS（First Come First Served）** でスケジューリングされる
- タスクが**待ち状態に入らない**と他のタスクに切り替わらない
- 同一優先度内では，先にレディキューに入ったタスクのみが実行される
- → 解決策は次サンプル `04_led_2tasks` で `dly_tsk` を使う

## 前サンプルとの差分

| 項目 | 02_led_1task_syslog | 03_led_2tasks_for |
|---|---|---|
| タスク数 | 1 | 2 |
| 共有変数 | なし | `unsigned int led_data` |
| LED出力 | `led_out(LED1)` 直接 | `led_data |= LED1; led_out(led_data);` |
| 静的API | `CRE_TSK` ×1 | `CRE_TSK` ×2 |

## なぜそう書くか

```c
unsigned int led_data = 0;   /* 2タスクで共有する変数 */

void
led1_task(intptr_t exinf){
    led_init(0);
    for (;;) {
        led_data |= LED1;     /* led_dataの該当ビットだけON */
        led_out(led_data);
        busy_wait();
        led_data &= ~LED1;    /* led_dataの該当ビットだけOFF */
        led_out(led_data);
        busy_wait();
    }
}

void
led3_task(intptr_t exinf){
    for (;;) {
        led_data |= LED3;
        led_out(led_data);
        busy_wait();
        led_data &= ~LED3;
        led_out(led_data);
        busy_wait();
    }
}
```

- LEDは1つの GPIO レジスタで制御するため，他のLEDの状態を保持するために共有変数 `led_data` を導入。
- ただし2タスクから書き込むため**競合状態（race condition）** が起こりうる（後の `14_ex_none` で問題化する）。

```
CRE_TSK(LED1_TASK, { TA_ACT, 0, led1_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
CRE_TSK(LED3_TASK, { TA_ACT, 0, led3_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
```

両タスクとも `DEFAULT_PRIORITY=8` で同一優先度。`TA_ACT` で生成と同時に起動。

## なぜ片方しか動かないか

ASPのスケジューリング規則：

- 実行できるタスクの中で**最も優先順位の高い**タスクが実行される。
- 同じ優先度のタスクの間では，**先に実行できる状態になったタスクが高い優先順位**を持つ（FCFS）。
- 両タスクが `TA_ACT` で同時に登録されるが，`.cfg` の記述順で先に来た `LED1_TASK` が先に実行できる状態になる → `LED1_TASK` が実行状態となる。
- `LED1_TASK` は `for` ループでCPUを占有し続け，**待ち状態に入らない**ため，`LED3_TASK` が実行可能状態のまま。
- 結果，LED1のみ点滅して LED3 は永久にスキップされる。

## 解決策

タスクを **待ち状態に入れる** APIを使うと，他のタスクに切り替わる：

- `dly_tsk(ms)` — 指定ミリ秒待機 → `04_led_2tasks` で使用
- `wai_sem(...)` — セマフォ待ち → `15_ex_sem` で使用
- `wai_flg(...)` — イベントフラグ待ち → `16_sw_flg_cyc` 以降で使用
- `slp_tsk()` — 起床待ち → `11_sw_int_api2` で使用

または同一優先度内で強制的に切り替える `irot_rdq()` を使うパターンもある（`14_ex_none` で使用）。

## LLM向け要点

- 同一優先度の複数タスクを`for`ループだけで動かすと **片方しか動かない**。これはバグではなく仕様（FCFS）。
- マルチタスクで動作させたいなら **必ず待ち状態に入るAPI**を呼ぶ。
- `dly_tsk(0)` ではなく `dly_tsk(1)` 以上を使う（次のタイムティックまで待つ仕様）。
