# 11_sw_int_api2 — 割込みからの起床（slp_tsk + iwup_tsk）

## 何のサンプルか

`10_sw_int_api` の `iact_tsk` 方式を **`iwup_tsk` + `slp_tsk`** 方式に変えたもの。タスクは実行できる状態で生成し，起動直後に `slp_tsk` で起床待ち状態に入る。PUSH1割込みで `iwup_tsk` が呼ばれるとタスクが起こされて4回点滅する。

学習ポイント：

- **`slp_tsk()`** で自タスクを起床待ち状態に
- **`iwup_tsk(tskid)`** で他タスクを起こす（割込みハンドラから）
- 起床要求もキューイングされる（`TMAX_WUPCNT`=1）
- 「起動」と「起床」の違い

## 前サンプルとの差分

| 項目 | 10_sw_int_api | 11_sw_int_api2 |
|---|---|---|
| タスクの生成属性 | `TA_NULL`（休止状態） | `TA_ACT`（実行できる状態） |
| 待ち方 | 起動を待つ（休止状態） | 起床を待つ（起床待ち状態） |
| API | `iact_tsk` + 終了は `return` | `iwup_tsk` + 待ちは `slp_tsk` |
| タスク本体 | 4回点滅して終了，再度起動可能 | 無限ループ：`slp_tsk` → 4回点滅 → `slp_tsk` |

## なぜそう書くか

### LED1点滅タスク

```c
void
led1_task(intptr_t exinf){
    int i;
    for (;;) {
        syslog(LOG_NOTICE, "led1_task : sleep!");
        slp_tsk();                                /* 起床待ち */
        syslog(LOG_NOTICE, "led1_task : wakeup!");

        for (i = 0; i < 4; i++) {
            if ((led_data & LED1) == LED1) {
                led_data &= ~LED1;
            } else {
                led_data |= LED1;
            }
            led_out(led_data);
            dly_tsk(LED1_INTERVAL);
        }
    }
}
```

- 無限ループ。
- ループの先頭で `slp_tsk()` で起床待ち状態に入る。
- 起床されると4回点滅。
- 再度ループの先頭に戻り `slp_tsk()` で待つ。

### PUSH1 割込みハンドラ

```c
void
push1_handler(void){
    push1_int_clear();
    syslog(LOG_NOTICE, "push1_handler : start!");
    SVC_PERROR(iwup_tsk(LED1_TASK));    /* タスクを起床 */
}
```

`iwup_tsk(tskid)` で対象タスクを起床する。タスクが `slp_tsk()` で起床待ち状態にあれば実行できる状態に遷移する。

### コンフィギュレーション

```
CRE_TSK(LED1_TASK, { TA_ACT, 0, led1_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
```

`TA_ACT` を指定して**生成と同時に実行**。タスク本体の冒頭で `slp_tsk()` を呼ぶので，すぐに起床待ち状態に入る。

## 「起動」と「起床」の違い

| 操作 | API | 状態遷移 |
|---|---|---|
| 起動（activate） | `act_tsk` / `iact_tsk` | 休止状態 → 実行できる状態 |
| 起床（wakeup） | `wup_tsk` / `iwup_tsk` | （狭義の）待ち状態 → 実行できる状態 |
| 終了 | `ext_tsk` / return | → 休止状態 |
| 起床待ち | `slp_tsk` / `tslp_tsk` | → 起床待ち状態 |

- **起動**: 休止状態（dormant）のタスクを実行できる状態にする。タスクの先頭から実行が始まる。
- **起床**: 起床待ち状態（waiting）のタスクを実行できる状態にする。**`slp_tsk` の次の文から実行が再開**される（タスクのコンテキストが保存されている）。

## 起床要求のキューイング

`iwup_tsk` の起床要求は **`TMAX_WUPCNT`=1** までキューイングされる。

例: タスクが点滅処理中（`slp_tsk` していない）に PUSH1 を3回連続で押す：

1. 1回目: 起床要求がキューイング（カウンタ=1）
2. 2回目: **`E_QOVR`** （キューイングオーバフロー）

タスクが次に `slp_tsk` を呼ぶと，キューイングされた起床要求でただちに起こされる（`slp_tsk` は待ちに入らずすぐ戻る）。

## どちらを使うか

| 用途 | 推奨API |
|---|---|
| タスクを使い捨てで実行（起動するたびに最初から） | `iact_tsk` |
| 永続タスクが特定イベントを待つ（受信ループ） | `slp_tsk` + `iwup_tsk` |
| 複数イベントを待ち分け | イベントフラグ（`16_sw_flg_cyc`） |
| データを送りたい | データキュー（`18_sw_dtq_cyc`） |

## LLM向け要点

- 永続的にイベントを待つタスクは **`slp_tsk` + `wup_tsk`/`iwup_tsk`** パターン。
- タスクは `TA_ACT` で生成し，本体の先頭で `slp_tsk()`。
- 起床要求のキューイングは **`TMAX_WUPCNT`=1** まで。
- 単純な「起こすだけ」の通知は `iwup_tsk` でよい。**複数のイベント種別を区別する**ならイベントフラグやデータキューを使う。
