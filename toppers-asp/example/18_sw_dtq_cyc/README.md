# 18_sw_dtq_cyc — データキューによる通知（周期ハンドラから）

## 何のサンプルか

`16_sw_flg_cyc` をデータキュー方式に書き換えたもの。

- 周期ハンドラがスイッチ変化を検出すると，**データキュー** にイベントコードを送信（`ipsnd_dtq`）
- タスクが **`rcv_dtq`** で受信し，LEDに反映

学習ポイント：

- **`CRE_DTQ`** でデータキュー生成
- **`ipsnd_dtq`**（周期ハンドラ→キュー）/ **`psnd_dtq`**（タスク→キュー）/ **`snd_dtq`**（タスク→キュー，ブロック可）
- **`rcv_dtq`** でデータ受信（待ち）
- イベントフラグとデータキューの違い: **イベントを順序通りに処理**できる

## 前サンプルとの差分

| 項目 | 16_sw_flg_cyc | 18_sw_dtq_cyc |
|---|---|---|
| 通知機構 | イベントフラグ | データキュー |
| 順序保存 | × （ビットOR） | ○ （FIFO） |
| 同一イベント連発 | 1回として扱われる | キューに積まれる |
| 静的API | `CRE_FLG` | `CRE_DTQ` |

## なぜそう書くか

### 周期ハンドラ（送信側）

```c
void
sw_cyc_handler(intptr_t exinf){
    unsigned char tmp;

    tmp = switch_slide_sense();

    if ((sw_state & SW1) != (tmp & SW1)) {
        if((tmp & SW1) == SW1){
            SVC_PERROR(ipsnd_dtq(SW_DTQ, SW1_ON));      /* キューに送信 */
        } else {
            SVC_PERROR(ipsnd_dtq(SW_DTQ, SW1_OFF));
        }
    }
    /* SW2, SW3, SW4 同様 */

    sw_state = tmp;
}
```

- **`ipsnd_dtq(dtqid, data)`**: 非タスクコンテキストからキューに送信（ポーリング = 待ちに入らない）。
- 第2引数 `data` は `intptr_t` 型。1要素分のデータ。
- キューが満杯なら `E_TMOUT` が返る（送信失敗）。

### タスク（受信側）

```c
void
led_task(intptr_t exinf){
    intptr_t sw_data;

    for (;;) {
        rcv_dtq(SW_DTQ, &sw_data);                  /* データ受信（待ち） */
        syslog(LOG_NOTICE, "led_task : recive data = 0x%x", sw_data);

        if ((sw_data & SW1_ON)  == SW1_ON)  led_data |= LED1;
        if ((sw_data & SW1_OFF) == SW1_OFF) led_data &= ~LED1;
        /* ... 同様 */

        led_out(led_data);
    }
}
```

- **`rcv_dtq(dtqid, *p_data)`**: データ受信（永久待ち）。
- ポーリング受信は `prcv_dtq`，タイムアウト付きは `trcv_dtq`。

### コンフィギュレーション

```
CRE_DTQ(SW_DTQ, { TA_NULL, 4, NULL });
```

`CRE_DTQ(dtqid, { dtqatr, dtqcnt, dtqmb })`:
- `TA_NULL`: 送信待ち行列はFIFO順（タスク優先度順なら `TA_TPRI`）
- `dtqcnt=4`: キュー容量4要素
- `dtqmb=NULL`: 管理領域はカーネルが確保

## イベントフラグとデータキューの使い分け

| 観点 | イベントフラグ | データキュー |
|---|---|---|
| 通知種別 | ビットでイベント種別 | `intptr_t` の任意値 |
| 順序保存 | × | ○ |
| 同一イベント連発時 | 1回のセットで集約される | 個別に積まれる |
| 受信タスクの動作 | 「これらのいずれかが起きるまで待つ」 | 「次のメッセージを取り出す」 |
| 容量 | 32ビット程度の状態 | 設定可能（要素数） |
| データ転送 | 不可（フラグのみ） | 可能（`intptr_t`） |

例：
- 「ボタンが押されたか」→ イベントフラグ向き
- 「ボタン押下回数が知りたい」→ データキュー向き
- 「センサ値を送る」→ データキュー（または優先度データキュー）

## データキュー API一覧

### 送信

| API | コンテキスト | ブロック動作 |
|---|---|---|
| `snd_dtq(id, data)` | タスク | キュー満杯時はブロック |
| `psnd_dtq(id, data)` | タスク | キュー満杯時は `E_TMOUT` |
| `ipsnd_dtq(id, data)` | 非タスクコンテキスト | キュー満杯時は `E_TMOUT` |
| `tsnd_dtq(id, data, tmout)` | タスク | 指定時間で `E_TMOUT` |
| `fsnd_dtq(id, data)` | タスク | 強制送信（最古を上書き） |
| `ifsnd_dtq(id, data)` | 非タスクコンテキスト | 強制送信 |

### 受信

| API | コンテキスト | ブロック動作 |
|---|---|---|
| `rcv_dtq(id, *p_data)` | タスク | データなし時はブロック |
| `prcv_dtq(id, *p_data)` | タスク | データなし時は `E_TMOUT` |
| `trcv_dtq(id, *p_data, tmout)` | タスク | 指定時間で `E_TMOUT` |

割込みハンドラ・周期ハンドラから受信は不可（待ちに入る可能性があるため）。

## LLM向け要点

- **データを送りたい**ときは `CRE_DTQ` + `snd_dtq`/`rcv_dtq`。
- 周期ハンドラ・割込みハンドラからは **`ipsnd_dtq`**（i接頭辞付き）。タスクからは `psnd_dtq`（ノンブロッキング）または `snd_dtq`（ブロック可）。
- 受信は基本 **`rcv_dtq`**（タスクのみ）。割込み内では受信不可。
- 1要素は `intptr_t` 1個分。複雑な構造体を送るには `CRE_MBX`（メールボックス）でポインタを送るほうが便利。
- 容量を超えるとデフォルトでは `E_TMOUT`。古いものを上書きしたいなら `fsnd_dtq`/`ifsnd_dtq`。
