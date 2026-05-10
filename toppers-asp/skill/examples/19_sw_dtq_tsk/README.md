# 19_sw_dtq_tsk — データキューによる通知（タスクから）

## 何のサンプルか

`18_sw_dtq_cyc` の周期ハンドラをタスクに置き換えた版。`sw_task` がスイッチをスキャンして `psnd_dtq` で送信する。

学習ポイント：

- タスクから送信するときは **`psnd_dtq`**（i接頭辞なし）または `snd_dtq`（ブロック可）
- 周期はタスク内の `dly_tsk` で実現

## 前サンプルとの差分

| 項目 | 18_sw_dtq_cyc | 19_sw_dtq_tsk |
|---|---|---|
| スキャン主体 | 周期ハンドラ | タスク（`SW_TASK`） |
| API（送信） | `ipsnd_dtq` | `psnd_dtq` |
| 周期 | `CRE_CYC` | `dly_tsk` |
| 静的API | `CRE_CYC` | `CRE_TSK` 追加 |

## なぜそう書くか

### スイッチスキャンタスク

```c
void
sw_task(intptr_t exinf){
    unsigned char tmp;

    for(;;){
        tmp = switch_slide_sense();

        if ((sw_state & SW1) != (tmp & SW1)) {
            SVC_PERROR(psnd_dtq(SW_DTQ, ((tmp & SW1) == SW1) ? SW1_ON : SW1_OFF));
        }
        /* SW2, SW3, SW4 も同様 */

        sw_state = tmp;
        dly_tsk(SW_SCAN_INTERVAL);
    }
}
```

- **`psnd_dtq(dtqid, data)`**: タスクからのポーリング送信（ノンブロッキング）。キュー満杯時は `E_TMOUT`。
- ブロックを許容するなら `snd_dtq`（キューが空くまで待つ）。

### LED表示タスク

`18_sw_dtq_cyc` と同じ。

### コンフィギュレーション

```
CRE_TSK(LED_TASK, { TA_ACT, 0, led_task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
CRE_TSK(SW_TASK,  { TA_ACT, 0, sw_task,  DEFAULT_PRIORITY, STACK_SIZE, NULL });

ATT_INI({ TA_NULL, 0, led_init });
ATT_INI({ TA_NULL, 0, switch_slide_init });
ATT_INI({ TA_NULL, 0, state_init });

CRE_DTQ(SW_DTQ, { TA_NULL, 4, NULL });
```

`CRE_CYC` の代わりに `CRE_TSK(SW_TASK)` を追加。

## 送信API のブロック動作の選択

| API | キュー満杯時 | 用途 |
|---|---|---|
| `snd_dtq` | キューが空くまでブロック | データを必ず送りたい（受信側が確実に追いつく前提） |
| `psnd_dtq` | 即 `E_TMOUT` | 送信失敗を許容（最新値だけ重要等） |
| `tsnd_dtq(tmout)` | 指定時間で `E_TMOUT` | 一定時間以内に送れなければ捨てる |
| `fsnd_dtq` | 最古要素を上書きして送信成功 | リアルタイムなセンサ値で最新が常に必要 |

## 仕様まとめ：周期ハンドラ vs タスク（イベント通知3手段）

`16`, `17`, `18`, `19` の差分まとめ：

|  | 周期ハンドラ送信 | タスク送信 |
|---|---|---|
| イベントフラグ | `16_sw_flg_cyc` (`iset_flg`) | `17_sw_flg_tsk` (`set_flg`) |
| データキュー | `18_sw_dtq_cyc` (`ipsnd_dtq`) | `19_sw_dtq_tsk` (`psnd_dtq`) |

イベントフラグとデータキューは目的に応じて使い分け（`18_sw_dtq_cyc/README.md` 参照）。


## LLM向け要点

- データキュー送信API（タスクから）：
  - `snd_dtq` — ブロック可
  - `psnd_dtq` — ノンブロッキング
  - `tsnd_dtq(tmout)` — タイムアウト付き
  - `fsnd_dtq` — 強制（最古上書き）
- 受信は `rcv_dtq`/`prcv_dtq`/`trcv_dtq`（タスク専用）。
- 周期処理をタスクで書く場合は `dly_tsk` を使うのが定石。
- 同じデータキューに対して**割込みハンドラ・周期ハンドラ・タスクから混在して送信**することも可能。それぞれ適切なAPI（`i` の有無）を使う。
