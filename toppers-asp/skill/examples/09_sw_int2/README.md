# 09_sw_int2 — 複数割込みハンドラの登録

## 何のサンプルか

`08_sw_int` に PUSH2 ボタンの割込みハンドラを追加し，PUSH1で LED3，PUSH2 で LED4 を反転するプログラム。複数の割込みを扱う典型例。

学習ポイント：

- 異なる割込み番号に複数のハンドラを登録できる
- 割込み優先度を割込みごとに変えられる
- それぞれに `CFG_INT`/`DEF_INH`/`ATT_INI` のセットが必要

## 前サンプルとの差分

| 項目 | 08_sw_int | 09_sw_int2 |
|---|---|---|
| 割込みハンドラ数 | 1（PUSH1） | 2（PUSH1, PUSH2） |
| 割込み優先度 | -5 | PUSH1=-5, PUSH2=-6 |

## なぜそう書くか

```c
/*
 * EXTI9_5(PUSH1) 割込みハンドラ
 */
void
push1_handler(void){
    if ((led_data & LED3) == LED3) {
        led_data &= ~LED3;
    } else {
        led_data |= LED3;
    }
    push1_int_clear();
    led_out(led_data);
    syslog(LOG_NOTICE, "push1_int : led_data 0x%x", led_data);
}

/*
 * EXTI15_10(PUSH2) 割込みハンドラ
 */
void
push2_handler(void){
    if ((led_data & LED4) == LED4) {
        led_data &= ~LED4;
    } else {
        led_data |= LED4;
    }
    push2_int_clear();
    led_out(led_data);
    syslog(LOG_NOTICE, "push2_int : led_data 0x%x", led_data);
}
```

`asp_prog.cfg`：

```
ATT_INI({ TA_NULL, 0, led_init });
ATT_INI({ TA_NULL, 0, push1_int_init });
ATT_INI({ TA_NULL, 0, push2_int_init });

CFG_INT(INTNO_PUSH1, { (TA_ENAINT), PUSH1_INT_LVL });
DEF_INH(INHNO_PUSH1, { TA_NULL, push1_handler });
CFG_INT(INTNO_PUSH2, { (TA_ENAINT), PUSH2_INT_LVL });
DEF_INH(INHNO_PUSH2, { TA_NULL, push2_handler });
```

PUSH1とPUSH2をそれぞれ別個に：
1. 初期化ルーチン（端子設定）
2. `CFG_INT`（属性・優先度）
3. `DEF_INH`（ハンドラ登録）

## 割込み優先度の使い分け

PUSH1=-5，PUSH2=-6 なので **PUSH1のほうが高優先度**（値が小さいほど高い）。

- PUSH1ハンドラ実行中にPUSH2割込みが入っても受け付けられない（割込み優先度マスクが-5になっている）。
- PUSH2ハンドラ実行中にPUSH1割込みが入ると **多重割込み**として受け付けられる（PUSH1ハンドラがネスト実行される）。

優先度を全て同じにすれば多重割込みは起こらない（多重割込みを抑制したいときの設計指針）。

## 割込みフラグクリアのタイミング

```c
push1_int_clear();
```

このサンプルではハンドラの中盤に書かれているが，**できるだけハンドラの最後に書く**ことが推奨。理由：

- 早くクリアすると，ハンドラ実行中に同じ割込みが再発生して再度ペンディング状態になる可能性がある（特にエッジトリガでない場合）。
- 処理が完了してからクリアすると，処理中に発生したエッジを正しくキャッチできる。

ただし STM32 の EXTI のような **エッジトリガ**割込みでは順序による違いはほとんどない。

## LLM向け要点

- 複数割込みは **割込みごとに `CFG_INT` + `DEF_INH` + 必要なら `ATT_INI`** を1セット書く。
- 割込み優先度は割込みごとに自由に設定可能。**多重割込みの可否**は優先度の組み合わせで決まる。
- 割込みフラグクリアは原則ハンドラの最後で。
- `INTNO`（CFG_INT用）と `INHNO`（DEF_INH用）はデバイス毎に対応した値。STM32では `device.h` で定義済み。
