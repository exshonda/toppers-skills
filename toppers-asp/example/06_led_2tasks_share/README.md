# 06_led_2tasks_share — タスクのコード共有（拡張情報exinf）

## 何のサンプルか

`04_led_2tasks` では `led1_task` と `led3_task` の2関数を別々に書いていた。これを **1つの関数 `led_task` に統合**し，**拡張情報（`exinf`）** で動作を切り替える方式。

学習ポイント：

- `CRE_TSK` の第2引数 `exinf` がタスクのメインルーチンに渡される
- 同じコードを複数タスクで共有することでコードサイズが減る
- タスクごとに異なる動作（LED種類，周期）は構造体配列にまとめて参照

## 前サンプルとの差分

| 項目 | 04_led_2tasks | 06_led_2tasks_share |
|---|---|---|
| タスク関数 | `led1_task`, `led3_task` の2個 | `led_task` 1個 |
| `CRE_TSK` の `exinf` | 0（未使用） | 0 と 1（インデックス） |
| タスク固有データ | コード中にハードコード | 構造体配列 `task_info[]` |

## なぜそう書くか

```c
typedef struct{
    unsigned int led_bit;
    int          interval;
} LED_TASK_INFO;

LED_TASK_INFO task_info[] = {
    { LED1, LED1_INTERVAL },     /* exinf=0 で参照 */
    { LED3, LED3_INTERVAL }      /* exinf=1 で参照 */
};

void
led_task(intptr_t exinf){
    /* exinf をインデックスとして自タスク用の情報を取り出す */
    unsigned int led_bit  = task_info[(int)exinf].led_bit;
    int          interval = task_info[(int)exinf].interval;

    syslog(LOG_NOTICE, "led_task : exinf = %d, led_bit = 0x%x, interval = %d msec",
           exinf, led_bit, interval);

    for (;;) {
        led_data |= led_bit;
        led_out(led_data);
        dly_tsk(interval);

        led_data &= ~led_bit;
        led_out(led_data);
        dly_tsk(interval);
    }
}
```

`asp_prog.cfg`：

```
CRE_TSK(LED1_TASK, { TA_ACT, 0, led_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
CRE_TSK(LED3_TASK, { TA_ACT, 1, led_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
```

両タスクとも `led_task` 関数を指定し，`exinf` だけ `0` と `1` で区別する。

## 拡張情報の使い分け

タスクを区別する方法には2つある：

1. **タスクID**: `get_tid(&id)` で自タスクのIDを取得し，それで分岐。
2. **拡張情報 `exinf`**: `CRE_TSK` 時に好きな値を指定でき，引数で受け取る。

このサンプルでは2の方法を採用。`exinf` を構造体配列のインデックスとして使うのが定石。

`intptr_t` 型なのでポインタも格納できる。複雑な情報を渡したいときは構造体へのポインタをキャストして渡す：

```c
typedef struct { ... } TaskParam;
TaskParam params[] = { ... };
CRE_TSK(MY_TASK, { TA_ACT, (intptr_t)&params[0], my_task, ... });

void my_task(intptr_t exinf) {
    TaskParam *p = (TaskParam *)exinf;
    ...
}
```

## LLM向け要点

- 同じ振る舞いの複数タスクは **1関数に統合 + `exinf` で区別**するのが定石。
- `exinf` の型は `intptr_t`。整数として使う場合は `(int)exinf` 等のキャストが必要。
- ポインタを渡す場合は `(intptr_t)&data`，受け取り側で `(MyType *)exinf`。
- タスクの自己同定には `exinf` か `get_tid` を使う。`exinf` のほうが軽量。
