# 05_led_2tasks_init — 初期化ルーチン（ATT_INI）

## 何のサンプルか

`04_led_2tasks` で `led1_task` の中に書いていた `led_init(0)` を **初期化ルーチン**として独立させ，`ATT_INI` 静的APIで登録する。

学習ポイント：

- **`ATT_INI`** で初期化処理をカーネル起動前に実行する
- 複数タスクが共通で必要とするハードウェア初期化はタスク側に書かず初期化ルーチンに切り出す
- 初期化ルーチンの記述形式（`void func(intptr_t exinf)`）

## 前サンプルとの差分

| 項目 | 04_led_2tasks | 05_led_2tasks_init |
|---|---|---|
| LED初期化 | `led1_task` 内で `led_init(0)` を呼ぶ | `ATT_INI({ TA_NULL, 0, led_init })` で登録 |
| 起動順序の依存 | あり（`led1_task` が先に動かないと LED が初期化されない） | なし（カーネル起動前に初期化済み） |

## なぜそう書くか

### 問題点（`04_led_2tasks`）

`04` では `led_init(0)` を `led1_task` の冒頭で呼んでいる：

```c
void led1_task(intptr_t exinf){
    led_init(0);              /* LED初期化はここ */
    for (;;) { ... }
}

void led3_task(intptr_t exinf){
    for (;;) { ... }          /* led_init を呼んでいない */
}
```

ASPの **同一優先度タスクは記述順で先に来たほうから動く**ため，今回はたまたま `LED1_TASK` が先に登録されており動作する。しかし `.cfg` の記述順が変わったり，`led3_task` の優先度が高い設定になると，**`led_init` 前に `led_out` が呼ばれて誤動作**する可能性がある。

### 解決策：`ATT_INI` で初期化ルーチンを登録

```c
/* led_2tasks.c */
void
led1_task(intptr_t exinf){
    /* led_init は呼ばない */
    for (;;) {
        led_data |= LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
        ...
    }
}
```

```
# asp_prog.cfg
ATT_INI({ TA_NULL, 0, led_init });
```

- `ATT_INI({ATR iniatr, intptr_t exinf, INIRTN inirtn})` で初期化ルーチンを登録。
- `iniatr` は `TA_NULL` のみ。
- `exinf` は初期化ルーチンに渡される拡張情報。
- `inirtn` は初期化ルーチンの先頭番地。

### 初期化ルーチンの実行タイミング

1. カーネル自身の初期化（内部データ構造）
2. 静的APIの処理（オブジェクト登録）
3. **`ATT_INI` で登録した初期化ルーチンを `.cfg` 記述順に実行**
4. カーネル動作状態へ遷移
5. タスクの実行開始

このため，**タスクが動き始める前に LED が初期化されている**ことが保証される。

### 初期化ルーチンの記述形式

```c
void led_init(intptr_t exinf)
{
    /* exinf には ATT_INI の第2引数が渡される */
    /* カーネル動作前のため，呼び出せるサービスコールは限定的 */
    /* SILのAPIと sns_ker のみ */
}
```

このサンプルでは `led_init` は `device.h` で提供されている既存関数を使う。`ATT_INI` の第2引数 `0` がそのまま `exinf` として `led_init` に渡される。

## なぜ `device.h` を `.cfg` でインクルードするか

`asp_prog.cfg` の冒頭：

```
#include "led_2tasks.h"
#include "device.h"        /* led_init のプロトタイプ宣言を含む */
```

`ATT_INI` の第3引数（`led_init`）は一般定数式パラメータで，関数ポインタのアドレス。コンフィギュレータが `.cfg` を処理する際に C コンパイラを通して評価するため，**`led_init` の宣言が必要**。`led_init` は `device.h` で宣言されているのでこれをインクルードする。

## LLM向け要点

- ハードウェア初期化やシステム共通初期化は **`ATT_INI`** で登録する。
- タスク内に書いてしまうと，**タスク実行順序の依存性**を生む。
- 終了処理は `ATT_TER` で同様に登録する（記述順とは**逆順**で実行されることに注意）。
- 初期化ルーチンの中ではほとんどのサービスコールは呼べない（カーネル動作前のため）。
- `ATT_INI` は1つの `.cfg` に複数記述できる。記述順に実行される。
