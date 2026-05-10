# 17_sw_flg_tsk — イベントフラグによる通知（タスクから）

## 何のサンプルか

`16_sw_flg_cyc` の周期ハンドラ部分をタスクに置き換えた版。

- **`sw_task`** が `dly_tsk(10)` で10ms周期にスイッチをスキャン
- スイッチ変化があれば **`set_flg`**（`i` 接頭辞なし）でイベントフラグをセット
- **`led_task`** が `wai_flg` でフラグ待ち

学習ポイント：

- 周期ハンドラとタスクで実装する違い
- タスクから送信するときは **`set_flg`**（`i` なし）
- タスクなら `dly_tsk` で時間を作れる（周期ハンドラはこれが使えない）

## 前サンプルとの差分

| 項目 | 16_sw_flg_cyc | 17_sw_flg_tsk |
|---|---|---|
| スキャン主体 | 周期ハンドラ | タスク（`SW_TASK`） |
| 周期実現 | `CRE_CYC` | `dly_tsk` |
| API（送信） | `iset_flg` | `set_flg` |
| 静的API | `CRE_CYC` 必要 | `CRE_CYC` 不要（`CRE_TSK` 追加） |

## なぜそう書くか

### スイッチスキャンタスク

```c
void
sw_task(intptr_t exinf){
    unsigned char tmp;
    FLGPTN flgptn;

    for (;;) {
        flgptn = 0;
        tmp = switch_slide_sense();

        if ((sw_state & SW1) != (tmp & SW1)) {
            flgptn |= ((tmp & SW1) == SW1) ? SW1_ON : SW1_OFF;
        }
        /* SW2, SW3, SW4 も同様 */

        if (flgptn != 0) {
            SVC_PERROR(set_flg(SW_FLG, flgptn));    /* iなし */
            sw_state = tmp;
        }

        dly_tsk(SW_SCAN_INTERVAL);                  /* 周期実現 */
    }
}
```

- 周期ハンドラ版（16）ではループ・dly_tsk が使えなかったが，**タスクなら自由に使える**。
- `set_flg`（i接頭辞なし）を使う。タスクから `iset_flg` を呼ぶと `E_CTX`。

### LED表示タスク

```c
void
led_task(intptr_t exinf){
    FLGPTN flgptn;
    for (;;) {
        wai_flg(SW_FLG, SW_EV_MASK, TWF_ORW, &flgptn);
        syslog(LOG_NOTICE, "led_task : eventflag pattern = 0x%x", flgptn);
        /* ... LEDに反映 */
        led_out(led_data);
    }
}
```

`16_sw_flg_cyc` と同じ。

### コンフィギュレーション

```
CRE_TSK(LED_TASK, { TA_ACT, 0, led_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
CRE_TSK(SW_TASK,  { TA_ACT, 0, sw_task,  DEFAULT_PRIORITY, STACK_SIZE, NULL });

ATT_INI({ TA_NULL, 0, led_init });
ATT_INI({ TA_NULL, 0, switch_slide_init });
ATT_INI({ TA_NULL, 0, state_init });

CRE_FLG(SW_FLG, { TA_CLR, 0 });
```

`16` で `CRE_CYC` だったところが `CRE_TSK(SW_TASK, ...)` に置き換わっている。

## 周期ハンドラ vs タスクの選択

| 観点 | 周期ハンドラ | タスク |
|---|---|---|
| 周期精度 | 高い（カーネルがタイマで管理） | 落ちる（処理時間ぶんずれる） |
| 待ちAPI使用可否 | 不可（`E_CTX`） | 可 |
| スタック使用 | 共通の非タスクコンテキスト用 | タスク専用スタック |
| 排他制御の柔軟性 | 限定的 | 自由（セマフォ等使用可） |
| コーディング | `CRE_CYC` のみ | `CRE_TSK` + 本体 |

簡単な周期処理は周期ハンドラ，複雑な処理（待ち合わせ・複数同期）はタスクが向く。

## LLM向け要点

- **タスク**から送信するときは **`set_flg`**（i接頭辞なし）。
- **割込みハンドラ・周期ハンドラ**から送信するときは **`iset_flg`**（i接頭辞付き）。
- 同じイベントフラグに対して両方からセットするケースもある（タスクと割込みハンドラから同じフラグへ）。その場合は両APIを使い分ける。
- 周期処理の精度が必要なら**周期ハンドラ**，柔軟な処理が必要なら**タスク**。
