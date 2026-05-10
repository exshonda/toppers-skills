# 07_led_cyc — 周期ハンドラ（CRE_CYC）

## 何のサンプルか

LED1 をタスクで，LED3 を **周期ハンドラ** で点滅させるプログラム。タスク以外の処理単位として最も基本的な「周期ハンドラ」の使い方を学ぶ。

学習ポイント：

- **`CRE_CYC`** で周期ハンドラを生成
- 周期ハンドラは**非タスクコンテキスト**で実行される
- 周期ハンドラから呼べるサービスコールには制限がある（待ちに入れない・`i` 接頭辞付きAPI）
- このサンプルでは内部で待ち系APIを呼ばないので問題なし

## 前サンプルとの差分

| 項目 | 05_led_2tasks_init | 07_led_cyc |
|---|---|---|
| LED3を駆動するもの | タスク（`led3_task`） | 周期ハンドラ（`led3_cyc_handler`） |
| 静的API | `CRE_TSK` ×2 | `CRE_TSK` ×1 + `CRE_CYC` ×1 |
| 周期処理の精度 | `dly_tsk` 依存（処理時間ぶんずれる） | カーネルが周期に合わせて起動 |

## なぜそう書くか

```c
/*
 * LED3点滅周期ハンドラ
 * 注意: 非タスクコンテキストで実行されるため，
 *       待ち状態に入るAPI（dly_tsk, wai_sem等）は呼べない
 */
void
led3_cyc_handler(intptr_t exinf){
    if ((led_data & LED3) == LED3) {
        led_data &= ~LED3;       /* LED3をOFF */
    } else {
        led_data |= LED3;        /* LED3をON */
    }
    led_out(led_data);
}
```

- 周期ハンドラの記述形式は `void name(intptr_t exinf)` で，タスクと同じシグネチャ。
- ただし**無限ループにしない**。1回呼ばれて return する。
- 呼び出される間隔は `CRE_CYC` で指定する。

`asp_prog.cfg`：

```
CRE_TSK(LED1_TASK, { TA_ACT, 0, led1_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
CRE_CYC(LED3_CYC_HANDLER, { TA_STA, 0, led3_cyc_handler, LED3_INTERVAL, 0 });

ATT_INI({ TA_NULL, 0, led_init });
```

`CRE_CYC(cycid, { cycatr, exinf, cychdr, cyctim, cycphs })`：

| パラメータ | 意味 | このサンプルの値 |
|---|---|---|
| `cycid` | 周期ハンドラID（識別名） | `LED3_CYC_HANDLER` |
| `cycatr` | 属性 | `TA_STA`（生成時に動作開始） |
| `exinf` | 拡張情報 | 0 |
| `cychdr` | 周期ハンドラ関数 | `led3_cyc_handler` |
| `cyctim` | 起動周期（ミリ秒） | `LED3_INTERVAL`（500） |
| `cycphs` | 起動位相（ミリ秒） | 0 |

### 属性 `TA_STA` と `TA_PHS`

- `TA_STA`(=0x02): 生成と同時に動作開始。指定しないと「動作していない状態」で生成される（`sta_cyc` で動作開始する）。
- `TA_PHS`(=0x04): 生成時刻を基準時刻とする。**ASPでは未サポート**。

ASPでは `TA_STA` を使い，`cycphs` は0以上 `TMAX_RELTIM` 以下の値を指定する。`cycphs=0` を指定するとカーネル起動直後の最初のタイムティックで起動される（仕様により）。

## なぜ周期ハンドラを使うか

- **タスクで `dly_tsk` を使う場合**：`dly_tsk` は処理時間に対して経過時間を計算するため，処理時間が長いと周期がずれる。
- **周期ハンドラ**：カーネルが内部のタイマで周期を管理するため，周期が安定する（処理時間がずれても次回の起動時刻は固定）。

## 周期ハンドラから呼べないAPI

このサンプルでは問題ないが，一般に周期ハンドラ（非タスクコンテキスト）から：

- **呼べない**: `wai_sem`, `tslp_tsk`, `dly_tsk`, `wai_flg`, `rcv_dtq`, `loc_mtx` 等の **待ちに入る可能性があるAPI**
- **呼ぶときは `i` 接頭辞付き**: `iact_tsk`, `iset_flg`, `isig_sem`, `ipsnd_dtq`, `iwup_tsk`

タスクへの通知は次サンプル `16_sw_flg_cyc`, `18_sw_dtq_cyc` 参照。

## LLM向け要点

- 周期処理は **`CRE_CYC` で周期ハンドラ**として実装するのが推奨。
- 周期ハンドラは**非タスクコンテキスト**。待ちに入るAPIは呼べない。
- タスクへの通知は `iset_flg`/`isig_sem`/`ipsnd_dtq`/`iact_tsk` 等の `i` 接頭辞付きAPIで行う。
- ASPでは `TA_PHS` 属性は使えない。`TA_STA` を使う。
- `cyctim` は0より大きい値（0は無効）。`cycphs` は0以上。
