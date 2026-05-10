# 08_sw_int — 割込みハンドラ（CFG_INT + DEF_INH）

## 何のサンプルか

PUSH1 ボタンが押されたら（EXTI9_5割込み），LED3 を反転する割込みハンドラを実装する。

学習ポイント：

- **`CFG_INT`** で割込み要求ラインの属性を設定
- **`DEF_INH`** で割込みハンドラを登録
- 割込みハンドラの記述形式（`void name(void)`）
- 割込みハンドラからは `i` 接頭辞付きAPIのみ呼出可（このサンプルではAPI呼出なし）
- 割込みハンドラ内でデバイス側の割込みフラグクリアを行う必要

## 前サンプルとの差分

| 項目 | 07_led_cyc | 08_sw_int |
|---|---|---|
| 駆動源 | 周期ハンドラ（時間） | 割込みハンドラ（PUSH1ボタン） |
| 静的API | `CRE_CYC` | `CFG_INT` + `DEF_INH` |
| 初期化 | `led_init` | `led_init` + `push1_int_init`（端子を割込みに切替） |

## なぜそう書くか

### 割込みハンドラ

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
    push1_int_clear();              /* デバイスの割込みフラグクリア */
    led_out(led_data);
    syslog(LOG_NOTICE, "push1_int : led_data 0x%x", led_data);
}
```

- 割込みハンドラは **`void name(void)`** のシグネチャ（`exinf` なし）。
- 割込み処理の最後に **`push1_int_clear()`** を呼んでデバイス側の割込みフラグをクリア（これを忘れると同じ割込みが繰り返し発生する）。
- カーネル管理の割込みなので `syslog` は呼べる。

### `asp_prog.cfg`

```
ATT_INI({ TA_NULL, 0, led_init });
ATT_INI({ TA_NULL, 0, push1_int_init });    /* PUSH1端子を割込み機能に切替 */

CFG_INT(INTNO_PUSH1, { (TA_ENAINT), PUSH1_INT_LVL });
DEF_INH(INHNO_PUSH1, { TA_NULL, push1_handler });
```

3段階のセットアップ：

1. **`ATT_INI` で `push1_int_init`**：GPIO端子を割込み機能に切替（デバイス依存処理）
2. **`CFG_INT(intno, { intatr, intpri })`**：割込み要求ラインの属性設定
   - `INTNO_PUSH1` = STM32定義の割込み番号（device.h で定義済み）
   - `TA_ENAINT`(=0x01)：割込み要求禁止フラグをクリア（割込み許可状態にする）
   - `PUSH1_INT_LVL` = -5（割込み優先度。値が小さいほど高優先度）
3. **`DEF_INH(inhno, { inhatr, inthdr })`**：割込みハンドラ番号にハンドラを登録
   - `INHNO_PUSH1` = ハンドラ番号
   - `TA_NULL` = カーネル管理の割込み（標準）
   - `push1_handler` = ハンドラ関数

### CFG_INT と DEF_INH の関係

- `CFG_INT` は**割込み要求ラインの属性**を設定（割込み番号 `INTNO`）。
- `DEF_INH` は**割込みハンドラを登録**（割込みハンドラ番号 `INHNO`）。
- STM32では `INTNO` と `INHNO` は1対1対応。

`CFG_INT` を呼ばないとその割込み要求ラインは禁止フラグがセットされたまま動かない。

### 割込み優先度

- ASPでは割込み優先度は **負の値** で，`-1` が最高優先度。
- STM32では `-1` 〜 `-15` の範囲。
- `TA_NONKERNEL` 属性なしの場合は `TMIN_INTPRI` 〜 `-1` の範囲（カーネル管理の割込み）。

## 割込みハンドラから呼べるAPI

このサンプルでは API を呼んでいないが，呼ぶ場合は：

- **OK**: `iact_tsk`, `iwup_tsk`, `iset_flg`, `isig_sem`, `ipsnd_dtq`, `irot_rdq`, `iget_tid`, `iloc_cpu`/`iunl_cpu`, `syslog`（内部で安全に処理）
- **NG**: `wai_*`, `dly_tsk`, `loc_mtx` 等の待ちに入るAPI → `E_CTX`

次のサンプル `10_sw_int_api` で API 呼出の例を扱う。

## LLM向け要点

- 割込み登録の手順: **`ATT_INI` で端子初期化 → `CFG_INT` で属性 → `DEF_INH` でハンドラ登録**。
- 割込みハンドラのシグネチャは **`void name(void)`**（`exinf` なし）。
- 割込み優先度は **負値**。`-1` が最高，`-15` 等が低い。
- 割込みハンドラ内で **デバイスの割込みフラグクリアは必須**。
- 割込みハンドラから `wai_*`, `dly_tsk` 等は呼ばない（`E_CTX` エラー）。
- `TA_ENAINT` を `intatr` に指定しないと割込みが禁止のままになる。
