# 16_sw_flg_cyc — イベントフラグによる通知（周期ハンドラから）

## 何のサンプルか

スライドスイッチの状態変化を **イベントフラグ** でタスクに通知するプログラム。

- **周期ハンドラ** が10ms毎にスイッチをスキャン → 変化があれば `iset_flg` でビットをセット
- **タスク** は `wai_flg` でビットがセットされるのを待つ → 起こされたらLEDに反映

学習ポイント：

- **`CRE_FLG`** でイベントフラグを生成（`TA_CLR` 属性で待ち解除時自動クリア）
- **`iset_flg`** で割込みハンドラ・周期ハンドラからビットをセット（i接頭辞付き）
- **`wai_flg`** でタスクから複数ビットの待ち合わせ（OR/AND指定）
- **`TWF_ORW`** でいずれかのビットが立てば起床

## 前サンプルとの差分

| 項目 | 07_led_cyc | 16_sw_flg_cyc |
|---|---|---|
| 周期ハンドラの動作 | LED3反転 | スイッチをスキャンして変化を検出 |
| 通知方法 | 直接LEDを叩く | イベントフラグ経由 |
| イベント受信側 | なし | `led_task`（イベントフラグ待ち） |
| 静的API | `CRE_CYC` | `CRE_CYC` + `CRE_FLG` |

## なぜそう書くか

### イベントビットの定義

```c
#define SW1_ON   0x01
#define SW1_OFF  0x02
#define SW2_ON   0x04
#define SW2_OFF  0x08
#define SW3_ON   0x10
#define SW3_OFF  0x20
#define SW4_ON   0x40
#define SW4_OFF  0x80
#define SW_EV_MASK 0xff
```

8ビットのフラグで4つのスイッチのON/OFFイベントを表現。`SW_EV_MASK` は全ビットマスク（待ち合わせ用）。

### 初期化ルーチン

```c
void
state_init(intptr_t exinf){
    led_data = 0;
    sw_state = switch_slide_sense();

    /* 起動時のSW状態をLEDに反映 */
    if((sw_state & SW1) == SW1) led_data |= LED1;
    if((sw_state & SW2) == SW2) led_data |= LED2;
    if((sw_state & SW3) == SW3) led_data |= LED3;
    if((sw_state & SW4) == SW4) led_data |= LED4;

    led_out(led_data);
}
```

カーネル動作開始前に呼ばれる（`ATT_INI`）。`led_init` の後に呼ばれる必要があるので **`.cfg` での記述順** に注意。

### 周期ハンドラ（送信側）

```c
void
sw_cyc_handler(intptr_t exinf){
    unsigned char tmp;
    FLGPTN flgptn = 0;

    tmp = switch_slide_sense();

    /* SW1 の状態変化を検出 */
    if ((sw_state & SW1) != (tmp & SW1)) {
        if((tmp & SW1) == SW1){
            flgptn |= SW1_ON;
        } else {
            flgptn |= SW1_OFF;
        }
    }
    /* SW2, SW3, SW4 も同様 */

    if (flgptn != 0) {
        SVC_PERROR(iset_flg(SW_FLG, flgptn));    /* 変化があればフラグセット */
        sw_state = tmp;
    }
}
```

- 周期ハンドラは**非タスクコンテキスト**なので **`iset_flg`** を使う（`i` 接頭辞付き）。
- 状態変化があった分だけビットをセット（`flgptn |=`）。
- `iset_flg` はビットの**論理和**でセット（既存のセット済みビットは消えない）。

### タスク（受信側）

```c
void
led_task(intptr_t exinf){
    FLGPTN flgptn;

    for (;;) {
        wai_flg(SW_FLG, SW_EV_MASK, TWF_ORW, &flgptn);
        syslog(LOG_NOTICE, "led_task : eventflag pattern = 0x%x", flgptn);

        if ((flgptn & SW1_ON)  == SW1_ON)  led_data |= LED1;
        if ((flgptn & SW1_OFF) == SW1_OFF) led_data &= ~LED1;
        if ((flgptn & SW2_ON)  == SW2_ON)  led_data |= LED2;
        if ((flgptn & SW2_OFF) == SW2_OFF) led_data &= ~LED2;
        if ((flgptn & SW3_ON)  == SW3_ON)  led_data |= LED3;
        if ((flgptn & SW3_OFF) == SW3_OFF) led_data &= ~LED3;
        if ((flgptn & SW4_ON)  == SW4_ON)  led_data |= LED4;
        if ((flgptn & SW4_OFF) == SW4_OFF) led_data &= ~LED4;

        led_out(led_data);
    }
}
```

- **`wai_flg(flgid, waiptn, wfmode, p_flgptn)`**: イベントフラグ待ち。
  - `SW_EV_MASK`: 待つビットパターン（`SW1_ON`〜`SW4_OFF` の全ビット）
  - `TWF_ORW`: いずれか1ビットでもセットされていれば起床（OR待ち）
  - `p_flgptn`: 待ち解除時のビットパターンを返す変数のポインタ
- 起床時のビットパターンを見て該当処理を行う。

### コンフィギュレーション

```
CRE_TSK(LED_TASK, { TA_ACT, 0, led_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });

ATT_INI({ TA_NULL, 0, led_init });
ATT_INI({ TA_NULL, 0, switch_slide_init });
ATT_INI({ TA_NULL, 0, state_init });           /* led_init/switch_slide_init の後 */

CRE_CYC(SW_CYC_HANDLER, { TA_STA, 0, sw_cyc_handler, SW_SCAN_INTERVAL, 1 });

CRE_FLG(SW_FLG, { TA_CLR, 0 });                /* 待ち解除時にビット自動クリア */
```

- **`TA_CLR` 属性**: タスクが待ち解除されたときにビットパターンを**全クリア**。これで「同じイベントを2度処理する」を防ぐ。
- 初期ビットパターンは0（何もセットされていない状態）。

### イベントフラグ属性まとめ

| 属性 | 意味 |
|---|---|
| 何も指定しない（`TA_NULL`） | 待ち行列はFIFO順（既定） |
| `TA_TPRI`(=0x01) | 待ち行列はタスク優先度順 |
| `TA_WMUL`(=0x02) | 複数タスクの同時待ちを許可（指定しないと2人目以降は `E_ILUSE`） |
| `TA_CLR`(=0x04) | 待ち解除時にビットパターン自動クリア |

なおμITRON 4.0 にあった `TA_TFIFO`(=0x00) はASP仕様で廃止された。FIFO順を意図する場合は属性を明示しない（属性が他にないなら `TA_NULL` を指定）。

## 待ちモード `wfmode`

| モード | 動作 |
|---|---|
| `TWF_ORW`(=0x01) | `waiptn` のいずれかのビットがセットされたら起床 |
| `TWF_ANDW`(=0x02) | `waiptn` のすべてのビットがセットされたら起床 |

## LLM向け要点

- 多種類のイベント通知は **イベントフラグ** が最適。
- 割込みハンドラ・周期ハンドラから送信は **`iset_flg`**，タスクから送信は **`set_flg`**。
- 受信は **`wai_flg`**（待ち），ノンブロッキングは `pol_flg`，タイムアウト付きは `twai_flg`。
- 「**いずれか発生**」は `TWF_ORW`，「**全部揃ったら**」は `TWF_ANDW`。
- 自動クリアしたいなら `TA_CLR` 属性。`clr_flg` で明示クリアも可。
- 1つのイベントフラグに **複数タスクの待ち** を許すには `TA_WMUL`。
