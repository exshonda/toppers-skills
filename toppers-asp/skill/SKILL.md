---
name: toppers-asp
description: TOPPERS/ASPカーネル向けのアプリケーションプログラム（タスク・割込みハンドラ・周期ハンドラ・セマフォ・イベントフラグ・データキュー・ミューテックス・固定長メモリプール等）の作成・編集・レビュー時に必ず使うこと。`.cfg`（システムコンフィギュレーションファイル）を扱うとき，`CRE_TSK`/`CRE_SEM`/`CRE_FLG`/`CRE_DTQ`/`CRE_CYC`/`CRE_ALM`/`ATT_ISR`/`DEF_INH`/`ATT_INI` などの静的APIを書くとき，`act_tsk`/`wai_sem`/`set_flg`/`snd_dtq`/`loc_mtx`/`dly_tsk`/`slp_tsk` などのサービスコールを使うとき，`kernel.h`/`t_syslog.h`/`t_stdlib.h` をインクルードするとき，`syslog(LOG_NOTICE, ...)` や `SVC_PERROR()` を使うとき，ITRON仕様準拠のRTOSコードを書くときも対象。ユーザが「TOPPERS」「ASP」「ITRON」「RTOS」「リアルタイムOS」「組込み」「STM32 + ASP」「タスクを作って」「割込みを登録して」「周期実行したい」と言ったときに発動すること。FreeRTOSやZephyrからの移植，μITRON系コードのレビューもこのskillの対象。
---

# TOPPERS/ASP カーネル開発支援

TOPPERS/ASPカーネル（ITRON系リアルタイムOS）向けアプリケーションプログラムを書くためのスタイルガイドとサンプル集。

## 適用範囲

- **対象**: TOPPERS/ASPカーネル（Release 1.7.1ベース）の標準パッケージ
- **対象外**: FMP（マルチプロセッサ）／HRP2（メモリ保護）／HRMP／SSP／ASP Safety／動的生成機能拡張パッケージ／オーバランハンドラ拡張パッケージ／制約タスク拡張パッケージ
- **オブジェクト生成**: **静的API（`.cfg` ファイル）のみ**。**動的生成系（`acre_*`, `del_*`）は標準パッケージでは未サポート**で，アプリケーションコードからオブジェクトを生成・削除することはできない。すべてのタスク・セマフォ・イベントフラグ・データキュー・ミューテックス・固定長メモリプール・周期ハンドラ・アラームハンドラ・割込みサービスルーチン・割込みハンドラ・CPU例外ハンドラ・初期化ルーチン・終了処理ルーチンは `.cfg` 上の `CRE_*` / `DEF_*` / `ATT_*` / `CFG_INT` で**コンフィギュレータがビルド時に登録する**。

## μITRON 4.0 との関係

ASPカーネルは **μITRON 4.0仕様をベースとしているが，完全互換ではない**。μITRON 4.0準拠のコードをそのまま流用するとビルドエラーや実行時エラーになることがある。代表的な差分は次の通り（詳細は `references/rtos-spec.md` の §1.1）：

- **値=0 の属性は廃止**: `TA_TFIFO`, `TA_MFIFO`, `TA_HLNG`, `TA_WSGL` → 何も指定しない（`TA_NULL`）
- **データ型がC99準拠**: `B`/`H`/`W` → `int8_t`/`int16_t`/`int32_t`，`FP` → `TASK`/`TEXRTN`/`CYCHDR`/`ALMHDR` 等
- **動的生成API は標準では未サポート**: `acre_*`, `del_*` は使えない
- **廃止されたAPI**: `frsm_tsk`（→ `rsm_tsk`），`set_tim`（廃止），`isig_tim`（廃止）
- **仕様変更**: `ext_tsk` 非タスクコンテキスト呼出は `E_CTX`，`sta_cyc` 後最初の起動時刻が「起動位相」基準に変更
- **管理領域の分離**: `CRE_MPF` に `mpfmb` パラメータ追加，`CRE_DTQ` の最終パラメータが `dtqmb` に変更
- **ASP独自の追加**: `get_utm`, `ext_ker`, `sns_ker`, `get_inf`, `xsns_dpn`/`xsns_xpn`, `chg_ipm`/`get_ipm`, `ini_*` 系，`CRE_PDQ` 系，`ATT_TER`, `DEF_ICS`, `CFG_INT`, `ATT_ISR`
- **ヘッダ構成**: `t_services.h`/`s_services.h` 廃止 → `kernel.h`/`sil.h`／μITRON互換が必要なら `itron.h`

LLMが μITRON 4.0準拠の文献やWebの記事を参照してコードを書くと，これらの差分でビルドエラーになる。**ASP仕様（本skill のreferences）に従うこと**。

## 初手で必ずやること

タスクを開始したら，**まず該当するサンプルを `examples/` から特定して `README.md` を読む**こと。サンプルは `examples/01_led_1task/` から `examples/19_sw_dtq_tsk/` まで19本あり，前のサンプルとの差分で説明されている。

サンプル選択の早見表：

| やりたいこと | 参照サンプル |
|---|---|
| タスクを1個作る | `01_led_1task` |
| `printf` 相当でデバッグ出力 | `02_led_1task_syslog` |
| 複数タスクを協調動作 | `04_led_2tasks`（`dly_tsk`使用） |
| 起動時に1回だけ実行する初期化 | `05_led_2tasks_init`（`ATT_INI`） |
| 同じ関数で複数タスクを区別 | `06_led_2tasks_share`（`exinf`） |
| 一定周期で何かする | `07_led_cyc`（周期ハンドラ） |
| 外部割込みを処理 | `08_sw_int`, `09_sw_int2`（`CFG_INT`+`DEF_INH`+`ATT_ISR`） |
| 割込みからタスクを起動 | `10_sw_int_api`（`iact_tsk`） |
| 割込みでタスクを起こす | `11_sw_int_api2`（`slp_tsk`+`iwup_tsk`） |
| クリティカルセクション | `12_cpu_lock`（`loc_cpu`） |
| 細かい割込み制御 | `13_int_mask`（`chg_ipm`） |
| 共有変数の排他制御 | `15_ex_sem`（セマフォ） |
| 多種イベントの通知 | `16_sw_flg_cyc`／`17_sw_flg_tsk`（イベントフラグ） |
| データのタスク間転送 | `18_sw_dtq_cyc`／`19_sw_dtq_tsk`（データキュー） |

`14_ex_none` と `03_led_2tasks_for` は**意図的に動かない反例**で，それぞれ競合状態とFCFSの罠を観察するためのもの。

詳しくは `examples/README.md` を参照。

## 仕様参照（references/）

サンプルだけで足りない場合は `references/` の各ファイルを引く：

| ファイル | 引くタイミング |
|---|---|
| `references/rtos-spec.md` | タスク状態・スケジューリング規則・割込みモデル・初期化／終了の順序など，**RTOSの基本概念**を確認したいとき。**§1.1 にμITRON 4.0との差分一覧**あり（移植時に必須） |
| `references/api-spec.md` | サービスコール（C言語API）の**正確なシグネチャ・コンテキスト・エラーコード**を確認したいとき |
| `references/static-api-spec.md` | `.cfg` の静的APIの**書式とパラメータ**を確認したいとき |
| `references/error-codes.md` | `E_CTX`／`E_PAR`／`E_RLWAI`／`E_QOVR` などの**エラーが何で発生するか**を調べたいとき |
| `references/quick-rules.md` | FreeRTOS/Zephyr経験者向けの**対応表**と頻出パターン早見表 |

## コード生成の鉄則

### 鉄則1: 静的生成のみ（動的生成APIは未サポート）

**ASP標準パッケージでは，すべてのカーネルオブジェクトを `asp_prog.cfg` の静的APIで生成する**。実行時にオブジェクトを生成・削除する API は**存在しない**：

| 機能 | 静的API（`.cfg` に記述） | 動的API（**ASP未サポート**） |
|---|---|---|
| タスク | `CRE_TSK` | ~~`acre_tsk`~~, ~~`del_tsk`~~ |
| セマフォ | `CRE_SEM` | ~~`acre_sem`~~, ~~`del_sem`~~ |
| イベントフラグ | `CRE_FLG` | ~~`acre_flg`~~, ~~`del_flg`~~ |
| データキュー | `CRE_DTQ` | ~~`acre_dtq`~~, ~~`del_dtq`~~ |
| 優先度データキュー | `CRE_PDQ` | ~~`acre_pdq`~~, ~~`del_pdq`~~ |
| メールボックス | `CRE_MBX` | ~~`acre_mbx`~~, ~~`del_mbx`~~ |
| ミューテックス | `CRE_MTX` | ~~`acre_mtx`~~, ~~`del_mtx`~~ |
| 固定長メモリプール | `CRE_MPF` | ~~`acre_mpf`~~, ~~`del_mpf`~~ |
| 周期ハンドラ | `CRE_CYC` | ~~`acre_cyc`~~, ~~`del_cyc`~~ |
| アラームハンドラ | `CRE_ALM` | ~~`acre_alm`~~, ~~`del_alm`~~ |
| 割込みサービスルーチン | `ATT_ISR` | ~~`acre_isr`~~, ~~`del_isr`~~（CRE_ISR も未サポート） |
| 割込みハンドラ | `DEF_INH` | （動的版なし） |
| CPU例外ハンドラ | `DEF_EXC` | （動的版なし） |
| 初期化／終了処理 | `ATT_INI` / `ATT_TER` | （動的版なし） |
| 割込み要求ライン属性 | `CFG_INT` | （動的版なし） |
| 非タスクコンテキスト用スタック | `DEF_ICS` | （動的版なし） |

ユーザが `acre_*` / `del_*` / `AID_*` を要求したら**「ASP標準パッケージでは未サポート」と明示的に通告し，`.cfg` に静的記述する形に置き換える**。「ランタイムにタスクを増やしたい」「動的にセマフォを作りたい」といった要求は ASP では実現できないので，必要なオブジェクトを `.cfg` に予め宣言する設計に変更してもらう。

### 鉄則2: コンテキストごとの `i` 接頭辞

呼び出し元によってAPI名が変わる：

| 呼出元 | 使うAPI | 例 |
|---|---|---|
| タスク・タスク例外処理ルーチン | `i` 接頭辞**なし** | `act_tsk`, `sig_sem`, `set_flg`, `psnd_dtq`, `wup_tsk`, `loc_cpu` |
| 割込みハンドラ・ISR・周期ハンドラ・アラームハンドラ・CPU例外ハンドラ | `i` 接頭辞**付き** | `iact_tsk`, `isig_sem`, `iset_flg`, `ipsnd_dtq`, `iwup_tsk`, `iloc_cpu` |

両方から呼べる `〔TI〕` 系（`sns_*`, `xsns_dpn`, `xsns_xpn`, `syslog`, `get_utm`）は使い分け不要。

**待ちに入るAPI（`wai_sem`, `tslp_tsk`, `dly_tsk`, `wai_flg`, `rcv_dtq`, `loc_mtx`）は割込みハンドラ・周期ハンドラから絶対に呼ばない**（`E_CTX` で必ず失敗）。

### 鉄則3: 値が小さいほど高優先度

- タスク優先度: 1（最高）から `TMAX_TPRI` まで，**値が小さいほど高優先度**（FreeRTOSと逆）
- 割込み優先度: -1（最高）から `TMIN_INTPRI` まで，**負値**で表現

### 鉄則4: 時間はミリ秒

`SYSTIM`/`RELTIM`/`TMO` すべてミリ秒単位。`pdMS_TO_TICKS()` のような変換は不要。`dly_tsk(100)` で100ms。

`dly_tsk(0)` は0msではなく「次のタイムティックまで待つ」なので **`dly_tsk(1)` 以上**を使う。

### 鉄則5: エラーコードは負値

サービスコールの戻り値は `ER` 型（符号付き整数）。`E_OK`(=0) または正値で正常，負値はエラー。判定は `if (ercd < 0)`。

割込みハンドラ・周期ハンドラからのAPI呼出は **`SVC_PERROR()` でラップ**してエラーをログ出力する：

```c
#include <t_stdlib.h>

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)  svc_perror(__FILE__, __LINE__, #expr, (expr))

/* 使い方 */
SVC_PERROR(iact_tsk(WORKER_TASK));
```

## 必須テンプレート

### Cソースの先頭

```c
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>      /* syslog を使う場合 */
#include <t_stdlib.h>      /* SVC_PERROR を使う場合 */
#include "device.h"        /* LED/スイッチ操作関数（ターゲット依存） */
#include "<このプログラム固有>.h"
```

### `asp_prog.cfg` の先頭

```
INCLUDE("target_timer.cfg");
INCLUDE("syssvc/syslog.cfg");
INCLUDE("syssvc/banner.cfg");
INCLUDE("syssvc/serial.cfg");
INCLUDE("syssvc/logtask.cfg");

#include "<アプリヘッダ>.h"
#include "device.h"

/* ここから静的API */
```

### タスクの記述形式

```c
void my_task(intptr_t exinf)
{
    /* 初期化処理（必要なら） */

    for (;;) {                 /* 永続タスクは無限ループ */
        wai_sem(SEM_TRIGGER);  /* 待ちに入るAPIで他タスクに切替 */
        /* 本処理 */
    }
    /* return しても可（カーネルが自動的に ext_tsk() 同等処理） */
}
```

### 割込みハンドラの登録手順

```
ATT_INI({ TA_NULL, 0, push1_int_init });   /* 1. 端子を割込み機能に切替 */
CFG_INT(INTNO_PUSH1, { (TA_ENAINT), -5 }); /* 2. 割込み要求ラインの属性設定 */
DEF_INH(INHNO_PUSH1, { TA_NULL, push1_handler }); /* 3. ハンドラ登録 */
```

ハンドラ本体（**`void(void)` 形式**）：

```c
void push1_handler(void)
{
    push1_int_clear();          /* デバイスフラグクリア（必須） */
    SVC_PERROR(iset_flg(FLG_BUTTON, BIT_PRESSED));
}
```

## 命名規則

| 対象 | 形式 | 例 |
|---|---|---|
| タスク識別名 | 全大文字＋アンダースコア | `MAIN_TASK` |
| タスク関数名 | 小文字＋`_task` | `main_task` |
| セマフォ識別名 | `SEM_xxx` | `SEM_BUTTON` |
| イベントフラグ識別名 | `FLG_xxx` | `FLG_EVENT` |
| データキュー識別名 | `DTQ_xxx` | `DTQ_LOG` |
| ミューテックス識別名 | `MTX_xxx` | `MTX_LCD` |
| 周期ハンドラ識別名 | `xxx_CYC_HANDLER` | `LED3_CYC_HANDLER` |
| ISR関数名 | 小文字＋`_isr` | `button_isr` |
| 割込み優先度マクロ | `xxx_INT_LVL` | `PUSH1_INT_LVL` |

## アンチパターン

| やってはいけない | 正しいやり方 |
|---|---|
| ISRから `wai_sem`, `dly_tsk`, `tslp_tsk`, `wai_flg`, `rcv_dtq`, `loc_mtx` 呼出 | `i` 接頭辞付きAPI（`isig_sem`, `iwup_tsk`, `iset_flg`, `ipsnd_dtq`）でタスクへ通知 |
| タスクから `iact_tsk`, `isig_sem` 呼出 | `act_tsk`, `sig_sem`（`i` なし） |
| マルチタスクで `for(...);` のビジーウェイト | `dly_tsk(ms)` で待ち状態へ |
| `dly_tsk(0)` | `dly_tsk(1)` 以上 |
| 共有変数の無排他アクセス | `wai_sem`/`sig_sem` または `loc_mtx`/`unl_mtx` |
| 割込みハンドラ末尾でフラグクリア忘れ | 必ず `*_int_clear()` を呼ぶ |
| `loc_cpu`/`unl_cpu` のネスト | ペアでネストせず使用 |
| CPUロック中に `wai_*`, `sig_sem`, `chg_pri` 等を呼ぶ | CPUロック中は `loc_cpu`/`unl_cpu`/`dis_int`/`ena_int`/`sns_*`/`ext_*` のみ |
| `CFG_INT` なしで `ATT_ISR` | 必ず `CFG_INT` → `ATT_ISR` の順 |
| 1回 `return` するだけのタスク（無限ループなし） | 永続タスクは `for(;;)` |
| 優先度を「数字が大きいほど高」と扱う | 値が小さいほど高優先度 |
| `pdMS_TO_TICKS(ms)` のような変換 | TOPPERSは元々ミリ秒単位 |
| ヘッダの `extern` を `#ifndef TOPPERS_MACRO_ONLY` で囲まない | 必ず囲む（コンフィギュレータが `.cfg` 解析時にエラーになるため） |

## 範囲外API（提案・生成しない）

これらが必要に見えてもユーザに**「ASP標準では未サポート」と通告**して代替案を提示すること：

- 動的生成系: `acre_*`, `del_*`（`acre_tsk`, `acre_sem`, ...）
- 保護機能関連: `att_mem`, `att_pma`, `prb_mem`, `def_svc`, `cal_svc`, `sac_*`, `get_did`, `dis_wai`, `ena_wai`
- マルチプロセッサ: `loc_spn`, `iloc_spn`, `mact_tsk`, `mig_tsk`, `msta_cyc`, `msta_alm`, `mrot_rdq`, `get_pid`
- メッセージバッファ: `snd_mbf`, `rcv_mbf`, `CRE_MBF` →メールボックスで代替
- オーバランハンドラ: `def_ovr`, `sta_ovr`, `DEF_OVR`
- 制約タスク属性 `TA_RSTR`
- 周期ハンドラの `TA_PHS` 属性（ASP未サポート）
- 静的API: `AID_*`, `SAC_*`, `CRE_ISR`（ASPは `ATT_ISR` のみ）, `LMT_DOM`, `DEF_STK`, `ATT_REG`, `DEF_SRG`, `ATT_SEC`, `ATT_MOD`, `ATT_MEM`, `ATT_PMA`
- 保護ドメインの囲み（`DOMAIN`, `KERNEL_DOMAIN`），クラスの囲み（`CLASS`）

## ヘッダファイルについて

各サンプルの `*.h` の**内容（マクロ値・関数プロトタイプ）はターゲット環境ごとに異なる**ため，値（優先度・スタックサイズ・割込み番号）は**勝手に決めない**。書式パターンのみ参考にする：

```c
#define DEFAULT_PRIORITY    8       /* 値はターゲットによる */
#define STACK_SIZE          4096    /* 値はターゲットによる */

#ifndef TOPPERS_MACRO_ONLY
extern void my_task(intptr_t exinf);   /* C言語の宣言は必ず囲む */
#endif /* TOPPERS_MACRO_ONLY */
```

理由: ヘッダは `.cfg` からも `#include` される。コンフィギュレータは `TOPPERS_MACRO_ONLY` を定義した状態で `.cfg` を処理し，マクロ定義以外のC言語記述があるとエラーになる。

## 完了前のセルフチェック

C ソースまたは `.cfg` を生成・編集したら以下を確認する：

1. **範囲外API**を含めていないか
2. コンテキストごとの **`i` 接頭辞**は正しいか
3. **タスクは無限ループ**または明示的 `ext_tsk()` で終わるか
4. **`CFG_INT` → `ATT_ISR` の順**で記述されているか
5. **`#include "kernel_cfg.h"`** がCソースの冒頭にあるか
6. **`asp_prog.cfg` の `INCLUDE` 行5本**が揃っているか
7. **オブジェクト識別名は全大文字＋アンダースコア**か
8. **割込み優先度は負値**で，`TMIN_INTPRI..-1` の範囲か
9. **時間引数はミリ秒**として正しいか
10. **割込みハンドラ内でデバイスフラグクリア**しているか

## ユーザへの応答方針

- 範囲外API要求には**事前に明示的に通告**してから代替案を提示
- ターゲット依存事項（割込み番号の具体値，最小スタックサイズ等）は**勝手に決めず**「ターゲット仕様書を参照」と注記
- 既存サンプルをテンプレートとして再利用，新規構造を一から作らない
- 説明・コメント・識別名はユーザの言語に合わせる（日本語または英語）
- コードコメントは**「なぜ」を書く**。「何」は識別名から自明にする
