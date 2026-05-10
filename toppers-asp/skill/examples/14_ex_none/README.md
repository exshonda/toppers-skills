# 14_ex_none — 排他制御なし（競合状態の観察）

## 何のサンプルか

**意図的に排他制御を行わず**，2タスクから共有変数 `count` をインクリメントすると競合状態でカウントが取りこぼされることを観察する。次サンプル `15_ex_sem` で正しく排他制御する。

学習ポイント：

- 共有変数を複数タスクから書き込むときの**競合状態（race condition）**
- 周期ハンドラから **`irot_rdq`** でレディキューの優先順位を回転させて2タスクをラウンドロビン的に動かす
- ローカル変数（`local_count`）は競合しない → 比較で問題が見える化

## 前サンプルとの差分

| 項目 | 06_led_2tasks_share | 14_ex_none |
|---|---|---|
| 主目的 | 動作するマルチタスク | **問題のあるマルチタスク** |
| 共有変数 | `led_data`（ビット単位） | `count`（整数全体） |
| 切替方法 | `dly_tsk` | `irot_rdq` を周期ハンドラから |

## なぜそう書くか

### busy_wait_inc（時間のかかるインクリメント）

```c
#define DELAY_LOOP 0x40000L

int
busy_wait_inc(int e){
    volatile int i;
    e++;
    for(i = 0; i < DELAY_LOOP; i++);   /* インクリメント後にビジーループ */
    return e;
}
```

**インクリメントとReturnの間にビジーループ** → タスクが切り替わるタイミングでこの関数の中にいる確率が高くなり，競合が顕在化しやすい。

### タスク

```c
int count = 0;          /* 共有変数 */

void
task(intptr_t exinf){
    int local_count = 0;

    for (;;) {
        count = busy_wait_inc(count);              /* 共有変数を更新 */
        local_count = busy_wait_inc(local_count);  /* ローカル変数を更新 */
        syslog(LOG_NOTICE, "Task %d : count = %d, local_count = %d",
               exinf, count, local_count);
    }
}
```

`exinf` は1または2（タスク識別用）。`count` は2タスクで共有，`local_count` は各タスクごとに独立。

### 周期ハンドラで強制切替

```c
void
rot_cyc_handler(intptr_t exinf){
    SVC_PERROR(irot_rdq(DEFAULT_PRIORITY));
}
```

- `irot_rdq(pri)`: 指定優先度のレディキュー先頭タスクを末尾に回す（同一優先度内のラウンドロビン）。
- 1ms周期で呼ぶ（`CRE_CYC` の `cyctim=1`）。
- これにより task1 と task2 が頻繁に切り替わる。

### コンフィギュレーション

```
CRE_TSK(TASK1, { TA_ACT, 1, task, DEFAULT_PRIORITY, STACK_SIZE, NULL });
CRE_TSK(TASK2, { TA_ACT, 2, task, DEFAULT_PRIORITY, STACK_SIZE, NULL });

CRE_CYC(ROT_CYC_HANDLER, { TA_STA, 0, rot_cyc_handler, ROT_INTERVAL, 0 });
```

両タスクとも同じ関数 `task` を指定し，`exinf` で区別（コード共有パターン）。

## 観察される現象

実行すると syslog でこのような出力が出る（イメージ）：

```
Task 1 : count = 5, local_count = 4
Task 2 : count = 9, local_count = 4
Task 1 : count = 11, local_count = 5
...
```

- 期待値: `count` は2タスクで共有しているので各タスクの `local_count` の **2倍** で増えてほしい。
- 実際: 競合により `count` の増分が `local_count` の増分と同程度になる。

### なぜ競合するか

```c
count = busy_wait_inc(count);
```

これは「`count` を引数として渡し，戻り値を `count` に代入」と展開される：

1. `count` の値を読み込み（一時変数 t1 に）
2. `busy_wait_inc(t1)` を実行（中でビジーループ） ← この間にタスク切替が起こる
3. 別タスクが同じ手順で `count` を更新
4. 元のタスクが復帰し，**古い値+1** を `count` に書き戻す

→ 別タスクの更新が**上書きされて消える**。

## 解決策

次のサンプル `15_ex_sem` で **セマフォ** を使って排他制御する。

他にも：

- ミューテックス（`loc_mtx`/`unl_mtx`）— 推奨（優先度逆転を防ぐ）
- ディスパッチ禁止状態（`dis_dsp`/`ena_dsp`）— タスク間の簡易排他
- CPUロック（`loc_cpu`/`unl_cpu`）— 割込みハンドラとの排他必要時のみ

## LLM向け要点

- 複数タスクから書き込む共有変数は**必ず排他制御**（このサンプルが反面教師）。
- `irot_rdq(pri)` で同一優先度のタスクをラウンドロビン的に切り替えられる（疑似マルチコア環境シミュレーション用）。
- 競合状態は**運が悪いとき**にしか顕在化しないことが多いので，明示的に切替を強制するテストが有効。
- **読込→操作→書込** の3ステップ操作はアトミックでない。共有変数アクセスは細心の注意。
