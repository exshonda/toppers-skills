# 12_cpu_lock — CPUロック状態（loc_cpu / unl_cpu）

## 何のサンプルか

スライドスイッチ SW1 を ON/OFF することで，**CPUロック状態への遷移／解除**を切り替える。CPUロック中はカーネル管理の割込みがすべてマスクされ，PUSH1 を押しても割込みハンドラが実行されない。

学習ポイント：

- **`loc_cpu()`** で CPUロック状態に
- **`unl_cpu()`** で CPUロック解除
- CPUロック中は割込みハンドラが実行されない（保留される）
- CPUロック解除後にペンディング中の割込みが処理される

## 前サンプルとの差分

| 項目 | 09_sw_int2 | 12_cpu_lock |
|---|---|---|
| 主タスク | LED点滅 | スライドスイッチ監視（`switch_task`） |
| 排他制御 | なし | `loc_cpu`/`unl_cpu` |
| 観察ポイント | 割込みでLED反転 | CPUロック中は割込みが入らないこと |

## なぜそう書くか

### スライドスイッチ監視タスク

```c
void
switch_task(intptr_t exinf){
    unsigned char data;
    unsigned char pdata;

    pdata = switch_slide_sense();      /* 前回のSW状態 */

    for(;;){
        data = switch_slide_sense();
        /* OFF → ON のエッジを検出 */
        if (((data & SW1) == SW1) && (pdata & SW1) != SW1) {
            loc_cpu();
        }
        /* ON → OFF のエッジを検出 */
        if (((data & SW1) != SW1) && (pdata & SW1) == SW1) {
            unl_cpu();
        }
        pdata = data;
    }
}
```

- `switch_slide_sense()` でスライドスイッチの状態を取得し，前回値と比較してエッジを検出。
- SW1 OFF → ON のエッジで `loc_cpu()` を呼ぶ。
- SW1 ON → OFF のエッジで `unl_cpu()` を呼ぶ。

### 重要な制約

CPUロック状態では**呼び出せるサービスコールが極めて限られる**：

- `loc_cpu`/`unl_cpu`/`iloc_cpu`/`iunl_cpu`
- `dis_int`/`ena_int`
- `sns_*`（センサ系）
- `xsns_dpn`/`xsns_xpn`（CPU例外ハンドラから）
- `get_utm`
- `ext_tsk`/`ext_ker`

**それ以外のAPIは `E_CTX` エラー**。`syslog` も内部的にはサービスコールを使うため，CPUロック中は呼ばないこと。

このため `switch_task` ではCPUロック状態の間 `syslog` を呼んでいない。

### loc_cpu/unl_cpu はネスト不可

ASPでは `loc_cpu` を二重に呼ぶと未定義動作になる（実装によっては失敗）。`loc_cpu` と `unl_cpu` は**ペアで，ネストせずに**使う。

### 割込みハンドラ

```c
void
push1_handler(void){
    push1_int_clear();
    syslog(LOG_NOTICE, "push1_int : start!");
}
```

特別なことはせず，PUSH1割込みが発生したことをログ出力するだけ。

CPUロック中に PUSH1 が押されても，このハンドラは実行されない。CPUロックが解除された瞬間，ペンディング中の割込みが処理されるためハンドラが呼ばれる（割込み要求は CPUロック中も保持される）。

### コンフィギュレーション

```
ATT_INI({ TA_NULL, 0, switch_slide_init });
ATT_INI({ TA_NULL, 0, push1_int_init });

CFG_INT(INTNO_PUSH1, { (TA_ENAINT), PUSH1_INT_LVL });
DEF_INH(INHNO_PUSH1, { TA_NULL, push1_handler });
```

## CPUロックの典型的な用途

このサンプルのような単純デモ以外の実用例：

1. **デバイス初期化**: 初期化シーケンスで割込みやディスパッチによる中断を許せない場合
2. **割込みハンドラとの極短い排他**: イベントフラグ・セマフォでは性能要件を満たせない場合
3. **アトミック操作の必要時**: CAS命令がない環境でリードモディファイライトを保護

CPUロックは**割込み応答性を悪化させる**ので使用は最小限に。タスク間排他には `wai_sem`/`sig_sem`（→ `15_ex_sem`）を使う方がよい。

## LLM向け要点

- CPUロックは **割込みハンドラとの排他**に使う最終手段。
- `loc_cpu()` と `unl_cpu()` は**ペアで，ネストせず**に使う。
- CPUロック中は **ほとんどのAPIが `E_CTX`** で失敗する（`loc_cpu`/`unl_cpu`/`sns_*`/`ext_*` 等のみ可）。
- CPUロック中に発生した割込みは保留され，解除後に処理される。
- **`syslog` も呼ばない**（内部でサービスコールを使うため）。
- タスク間排他には**セマフォ**を使う方が割込み応答性が落ちないので推奨。
