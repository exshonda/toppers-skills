# 13_int_mask — 割込み優先度マスク（chg_ipm）

## 何のサンプルか

`12_cpu_lock` を拡張。スライドスイッチ SW2/SW3 の組合せで **割込み優先度マスク** を変えて，どの割込みが受け付けられるかを制御する。

| SW1 | SW2 | SW3 | 動作 |
|---|---|---|---|
| ON | - | - | CPUロック状態（PUSH1も PUSH2 も入らない） |
| OFF | OFF | OFF | マスク=0 → PUSH1(-5)もPUSH2(-6)も入る |
| OFF | ON | OFF | マスク=-5 → PUSH1(-5)はマスク，PUSH2(-6)もマスク |
| OFF | OFF | ON | マスク=-6 → PUSH1(-5)は入る，PUSH2(-6)はマスク |

学習ポイント：

- **`chg_ipm(intpri)`** で割込み優先度マスクを変更
- マスク以下の優先度の割込みは受け付けられない（マスク値より高い優先度のみ受付）
- **`sns_loc()`** でCPUロック中かどうかをチェック
- 割込み優先度マスクは「タスクと割込みハンドラ間の細かい排他制御」に使う

## 前サンプルとの差分

| 項目 | 12_cpu_lock | 13_int_mask |
|---|---|---|
| 割込み | PUSH1のみ | PUSH1 と PUSH2 |
| 排他制御 | CPUロックのみ | CPUロック + 割込み優先度マスク |
| 動作 | SW1のみ | SW1（CPUロック）+ SW2/SW3（マスク制御） |

## なぜそう書くか

### スイッチ監視タスク

```c
void
switch_task(intptr_t exinf){
    unsigned char data;
    unsigned char pdata;

    pdata = switch_slide_sense();

    for(;;){
        data = switch_slide_sense();

        /* SW1 で CPUロック制御 */
        if (((data & SW1) == SW1) && (pdata & SW1) != SW1) {
            loc_cpu();
        }
        if (((data & SW1) != SW1) && (pdata & SW1) == SW1) {
            unl_cpu();
        }

        /* CPUロック中でないときだけ優先度マスクを変更 */
        if (sns_loc() != true) {
            if (((data & SW2) == SW2) && ((data & SW3) != SW3)) {
                chg_ipm(-5);
            } else if (((data & SW2) != SW2) && ((data & SW3) == SW3)) {
                chg_ipm(-6);
            } else {
                chg_ipm(0);    /* マスクなし */
            }
        }
        pdata = data;
    }
}
```

ポイント：

- `sns_loc()`（`〔TI〕` 系API）で CPUロック中かを確認。CPUロック中は `chg_ipm` も呼べないため。
- `chg_ipm(intpri)` でマスク値を設定。
- `chg_ipm(0)` でマスク解除（`TIPM_ENAALL`=0）。

### 割込み優先度マスクの仕組み

割込み優先度マスクは **「マスクと同じか低い優先度の割込みをマスクする」**。

ASPでは値が小さいほど高優先度なので：

- マスク=0 → 全割込みを受け付ける
- マスク=-5 → 優先度-5以下（-5, -6, -7, …）の割込みをマスク。-1〜-4は受付
- マスク=-6 → -6以下をマスク。-1〜-5は受付

このサンプルでは PUSH1=-5, PUSH2=-6 なので：

- マスク=-5 → PUSH1も PUSH2も受け付けない
- マスク=-6 → PUSH1は受け付ける（-5 > -6）/PUSH2は受け付けない（-6 ≤ -6）

### CPUロックとの違い

| | CPUロック | 割込み優先度マスク |
|---|---|---|
| 効果 | カーネル管理の**全**割込みを禁止 | マスク以下の優先度のみ禁止 |
| API | `loc_cpu`/`unl_cpu` | `chg_ipm` |
| サービスコール制限 | 厳しい（`E_CTX`多発） | 待ちに入るAPIのみ `E_CTX` |
| 影響範囲 | 全割込み | マスクで指定した優先度以下の割込み |

## 待ちに入るAPIの制限

割込み優先度マスクが0以外の状態（全解除でない状態）では，**広義の待ち状態に遷移する可能性のあるサービスコール** を呼べない（`E_CTX`）。例：`wai_sem`, `tslp_tsk`, `dly_tsk`, `wai_flg`。

この制約はCPUロック状態と同じ。`pol_*` 系のポーリングAPIは呼べる。

### 割込みハンドラ

```c
void
push1_handler(void){
    syslog(LOG_NOTICE, "push1_int : start!");
    if ((led_data & LED1) == LED1) {
        led_data &= ~LED1;
    } else {
        led_data |= LED1;
    }
    led_out(led_data);
    push1_int_clear();
}

void
push2_handler(void){
    syslog(LOG_NOTICE, "push2_int : start!");
    if ((led_data & LED2) == LED2) {
        led_data &= ~LED2;
    } else {
        led_data |= LED2;
    }
    led_out(led_data);
    push2_int_clear();
}
```

PUSH1 → LED1反転，PUSH2 → LED2反転のシンプルなハンドラ。

## 構成

```
ATT_INI({ TA_NULL, 0, led_init });
ATT_INI({ TA_NULL, 0, switch_slide_init });
ATT_INI({ TA_NULL, 0, push1_int_init });
ATT_INI({ TA_NULL, 0, push2_int_init });

CFG_INT(INTNO_PUSH1, { (TA_ENAINT), PUSH1_INT_LVL });   /* PUSH1=-5 */
DEF_INH(INHNO_PUSH1, { TA_NULL, push1_handler });
CFG_INT(INTNO_PUSH2, { (TA_ENAINT), PUSH2_INT_LVL });   /* PUSH2=-6 */
DEF_INH(INHNO_PUSH2, { TA_NULL, push2_handler });
```

## LLM向け要点

- 細かい割込み制御には **割込み優先度マスク**を使う。
- マスク値は **負値**。値より低い優先度（より小さい絶対値の数値より大きい値）の割込みのみが受け付けられる。
- `chg_ipm(0)` でマスク解除。
- 割込み優先度マスクが0以外の間も **待ちに入るAPIは禁止**。
- CPUロックとは独立した状態。両者は併用可能だがCPUロック中は `chg_ipm` 呼出不可。
- `sns_loc()` でCPUロック中かをチェック，`get_ipm()` で現在のマスク値を取得。
