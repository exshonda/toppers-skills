# AGENTS.md — TOPPERS/ASP アプリケーション開発エージェント指示書

このファイルは，このリポジトリで TOPPERS/ASPカーネル 向けのアプリケーションプログラムを LLM コーディングエージェント（Claude Code, Cursor, Aider, Codex, GitHub Copilot Coding Agent など）に書かせるときに従うべきルール・参照すべきリソース・避けるべきパターンをまとめた指示書である。

エージェントは **新しいタスクを開始する前に必ず本ファイル全体を読む** こと。

---

## 1. プロジェクトの前提

### 1.1 対象RTOS

- **TOPPERS/ASPカーネル**（Release 1.7.1相当）専用。
- ASP 標準パッケージのみサポート。**FMP（マルチプロセッサ版）／HRP2（メモリ保護版）／HRMP／SSP／ASP Safety／動的生成機能拡張パッケージ／オーバランハンドラ拡張パッケージ／制約タスク拡張パッケージは対象外**。
- **すべてのカーネルオブジェクトは静的API（`.cfg` ファイル）で生成する**。**動的生成API（`acre_*`, `del_*`, `AID_*`）はASP標準パッケージでは未サポート**。タスク・セマフォ・イベントフラグ・データキュー・ミューテックス・固定長メモリプール・周期ハンドラ・アラームハンドラ・ISR・割込みハンドラ・CPU例外ハンドラ・初期化／終了処理ルーチンはすべて `.cfg` 中の `CRE_*` / `DEF_*` / `ATT_*` / `CFG_INT` で**コンフィギュレータがビルド時に登録**する。
- 保護ドメイン（`DOMAIN`/`KERNEL_DOMAIN`）・クラス（`CLASS`）の囲みは使わない。
- アクセス許可ベクタ系API（`SAC_*`, `sac_*`）は使わない。

### 1.1.1 ASPはμITRON 4.0ベースだが完全互換ではない

ASPカーネルは μITRON 4.0仕様をベースとしているが**主要な点で仕様が異なる**。LLMが μITRON 4.0準拠の文献・Web記事・既存コードを参考にすると以下の理由でビルドエラーや誤動作になる。詳細は `doc/TOPPERS-ASP_RTOS仕様.md` の §1.1（および skill版 `references/rtos-spec.md` §1.1）参照：

- **値=0 の属性は廃止**: `TA_TFIFO`, `TA_MFIFO`, `TA_HLNG`, `TA_WSGL` → 明示しない（属性ゼロは `TA_NULL`）
- **データ型がC99準拠**: `B`/`H`/`W` → `int8_t`/`int16_t`/`int32_t`，`FP` → `TASK`/`TEXRTN`/`CYCHDR`/`ALMHDR` 等
- **動的生成APIは標準では未サポート**: `acre_*`, `del_*`, `AID_*` は使えない
- **廃止されたAPI**: `frsm_tsk`（→ `rsm_tsk`），`set_tim`（廃止），`isig_tim`（廃止），`TMAX_SUSCNT`（1固定，`itron.h` 移動）
- **仕様変更**: `ext_tsk` 非タスクコンテキスト呼出は `E_CTX`，`sta_cyc` 後の最初の起動時刻は「起動位相」基準
- **管理領域の分離**: `CRE_MPF` に `mpfmb` パラメータ追加，`CRE_DTQ` の最終パラメータが `dtqmb` に変更
- **ASP独自の追加**: `get_utm`, `ext_ker`, `sns_ker`, `get_inf`, `xsns_dpn`/`xsns_xpn`, `chg_ipm`/`get_ipm`, `ini_*` 系, `CRE_PDQ` 系, `ATT_TER`, `DEF_ICS`, `CFG_INT`, `ATT_ISR`
- **ヘッダ構成**: `t_services.h`/`s_services.h` 廃止 → `kernel.h`/`sil.h`（μITRON互換型・定数が必要な場合のみ `itron.h` を明示的にインクルード）

### 1.2 想定ターゲット

教材は STM32F401（NUCLEO/Discovery系）向けだが，本指示書は**ターゲット非依存**の TOPPERS/ASP仕様部分にフォーカスする。ターゲット定義事項（割込み番号・最小スタックサイズ・FPU属性等）は，対象ターゲットのターゲット依存部仕様書を参照させる。

### 1.3 言語と規格

- C言語（C90 または C99 準拠，フリースタンディング環境）。
- 16ビットおよび32ビット整数型，ポインタを格納できる整数型がある前提。

---

## 2. リポジトリ構成

```
toppers-asp-llm/
├── AGENTS.md                    # 本ファイル
├── AGENTS_SIMPLE.md             # 短縮版
├── doc/                         # スタイルガイド（マニュアル）
│                                # 原典: https://www.toppers.jp/docs/tech/ngki_spec-171.pdf
│   ├── TOPPERS-ASP_RTOS仕様.md           # 概念・状態モデル・初期化／終了（§1.1にμITRON 4.0差分）
│   ├── TOPPERS-ASP_API仕様.md            # サービスコール（C言語API）
│   ├── TOPPERS-ASP_静的API_API仕様.md    # .cfg 静的API
│   ├── TOPPERS-ASP_静的API_エラー.md     # エラーコード一覧
│   └── TOPPERS-ASP_クイックルール.md     # FreeRTOS/Zephyr 対応表＋頻出パターン
├── example/                     # サンプル19本（テンプレート）
│   ├── README.md                # サンプル全体の索引
│   ├── 01_led_1task/            # 最小: 1タスク
│   ├── 02_led_1task_syslog/     # syslogデバッグ
│   ├── 03_led_2tasks_for/       # FCFS反例
│   ├── 04_led_2tasks/           # dly_tsk
│   ├── 05_led_2tasks_init/      # ATT_INI
│   ├── 06_led_2tasks_share/     # exinfでコード共有
│   ├── 07_led_cyc/              # 周期ハンドラ
│   ├── 08_sw_int/               # 割込みハンドラ
│   ├── 09_sw_int2/              # 複数割込み
│   ├── 10_sw_int_api/           # iact_tsk, SVC_PERROR
│   ├── 11_sw_int_api2/          # slp_tsk + iwup_tsk
│   ├── 12_cpu_lock/             # CPUロック
│   ├── 13_int_mask/             # 割込み優先度マスク
│   ├── 14_ex_none/              # 競合状態の観察（反例）
│   ├── 15_ex_sem/               # セマフォ排他
│   ├── 16_sw_flg_cyc/           # イベントフラグ（CYC送信）
│   ├── 17_sw_flg_tsk/           # イベントフラグ（TSK送信）
│   ├── 18_sw_dtq_cyc/           # データキュー（CYC送信）
│   └── 19_sw_dtq_tsk/           # データキュー（TSK送信）
└── skill/                       # Claude Code 等のskill形式
    ├── SKILL.md                 # frontmatter + 概要 + 使用ガイド
    ├── references/              # マニュアル（doc/と同内容を短い英名で）
    │   ├── rtos-spec.md
    │   ├── api-spec.md
    │   ├── static-api-spec.md
    │   ├── error-codes.md
    │   └── quick-rules.md
    └── examples/                # サンプル（example/と同内容）
        ├── README.md
        └── 01_led_1task/ ... 19_sw_dtq_tsk/
```

各サンプルディレクトリには `README.md`／`*.c`／`*.h`／`asp_prog.cfg` の4ファイルが含まれる。`Makefile`・`.launch` などビルド固有ファイルは含まない（参照したい場合は `material/` 配下の元教材を見ること）。

---

## 3. エージェントの動作規範

### 3.1 タスク開始時のチェックリスト

1. ユーザの要求を読み，**該当するサンプルディレクトリ**（`example/NN_*/`）を特定する。例：「割込みからセマフォを `signal` したい」→ `15_ex_sem` ＋ `08_sw_int` を参照。
2. 該当サンプルの `README.md` を読み，使うAPI・なぜそう書くかを理解する。
3. 不明なAPIは `doc/TOPPERS-ASP_API仕様.md` で引き，制約（呼出可能コンテキスト・エラーコード）を確認する。
4. `.cfg` の書き方は `doc/TOPPERS-ASP_静的API_API仕様.md` を引く。
5. 使うかも知れない API を提示する前に，それが**ASPの標準パッケージ**でサポートされているか必ず確認する（4.4節参照）。

### 3.2 コード生成の原則

- **既存サンプルをテンプレートとして再利用**する。新規構造をゼロから作らない。
- すべてのカーネルオブジェクトは `asp_prog.cfg` の静的APIで生成する。
- タスク・ハンドラの記述形式（シグネチャ）は仕様準拠：
  - タスク本体: `void name(intptr_t exinf)`
  - 周期ハンドラ／アラームハンドラ／タスク例外処理ルーチン／ISR: `void name(intptr_t exinf)`
  - 割込みハンドラ（`DEF_INH` で登録）: `void name(void)`
  - CPU例外ハンドラ: `void name(void *p_excinf)`
  - 初期化／終了処理ルーチン: `void name(intptr_t exinf)`
- C ソースの先頭は固定の順序：
  ```c
  #include <kernel.h>
  #include "kernel_cfg.h"
  #include <t_syslog.h>      /* syslog を使う場合 */
  #include <t_stdlib.h>      /* SVC_PERROR を使う場合 */
  #include "device.h"
  #include "<このプログラム固有>.h"
  ```
- `asp_prog.cfg` の冒頭は固定パターン：
  ```
  INCLUDE("target_timer.cfg");
  INCLUDE("syssvc/syslog.cfg");
  INCLUDE("syssvc/banner.cfg");
  INCLUDE("syssvc/serial.cfg");
  INCLUDE("syssvc/logtask.cfg");
  ```

### 3.3 命名規則

| 対象 | 形式 | 例 |
|---|---|---|
| タスク識別名 | 全大文字＋アンダースコア | `MAIN_TASK`, `BUTTON_TASK` |
| タスク関数名 | 小文字＋`_task` | `main_task`, `button_task` |
| セマフォ識別名 | `SEM_xxx` | `SEM_BUTTON` |
| イベントフラグ識別名 | `FLG_xxx` | `FLG_EVENT` |
| データキュー識別名 | `DTQ_xxx` | `DTQ_LOG` |
| ミューテックス識別名 | `MTX_xxx` | `MTX_LCD` |
| 周期ハンドラ識別名 | `CYC_xxx` または `xxx_CYC_HANDLER` | `CYC_HEARTBEAT` |
| 周期ハンドラ関数名 | 小文字＋`_cyc_handler` または `_cychdr` | `heartbeat_cyc_handler` |
| アラームハンドラ識別名 | `ALM_xxx` | `ALM_TIMEOUT` |
| ISR関数名 | 小文字＋`_isr` | `button_isr` |
| 割込み優先度マクロ | 大文字＋`_INT_LVL` | `BUTTON_INT_LVL` |

### 3.4 コンテキスト判定の鉄則

呼出元コンテキストを意識し，APIの **`i` 接頭辞** を使い分ける：

| 呼出元 | 使うAPI | 例 |
|---|---|---|
| タスク・タスク例外処理ルーチン | **`i` 接頭辞なし** | `act_tsk`, `sig_sem`, `set_flg`, `psnd_dtq`, `wup_tsk`, `loc_cpu` |
| 割込みハンドラ・ISR・周期ハンドラ・アラームハンドラ・CPU例外ハンドラ | **`i` 接頭辞付き** | `iact_tsk`, `isig_sem`, `iset_flg`, `ipsnd_dtq`, `iwup_tsk`, `iloc_cpu` |

両方から呼べる `〔TI〕` 系（`sns_*`, `get_utm`, `xsns_dpn`, `xsns_xpn`, `syslog`）は使い分け不要。

### 3.5 エラー処理

- 割込みハンドラ・周期ハンドラ・アラームハンドラから API を呼ぶときは **`SVC_PERROR()` でラップ** する：

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

  /* 使用例 */
  SVC_PERROR(iact_tsk(WORKER_TASK));
  ```

- タスクからのAPI呼出は，戻り値を `ER` 型で受けて `< 0` 判定でエラーチェックする。
- タイムアウト付きAPIでは `E_TMOUT` ／ `E_RLWAI` ／ `E_DLT` の3種類のエラーをそれぞれ適切に処理する（`doc/TOPPERS-ASP_静的API_エラー.md` 参照）。

---

## 4. やってはいけないこと（NG リスト）

### 4.1 範囲外API

以下のAPIは **ASP標準パッケージでは未サポート**。提案・生成しない：

- 動的生成系: `acre_tsk`, `acre_sem`, `acre_flg`, `acre_dtq`, `acre_pdq`, `acre_mbx`, `acre_mtx`, `acre_mpf`, `acre_cyc`, `acre_alm`, `acre_isr`, `del_*` 全般
- 保護機能関連: `att_mem`, `att_pma`, `det_mem`, `prb_mem`, `ref_mem`, `def_svc`, `cal_svc`, `sac_*` 全般, `get_did`, `dis_wai`, `idis_wai`, `ena_wai`, `iena_wai`
- マルチプロセッサ: `loc_spn`, `iloc_spn`, `try_spn`, `itry_spn`, `unl_spn`, `iunl_spn`, `ref_spn`, `mact_tsk`, `imact_tsk`, `mig_tsk`, `msta_cyc`, `msta_alm`, `imsta_alm`, `mrot_rdq`, `imrot_rdq`, `get_pid`, `iget_pid`
- メッセージバッファ: `snd_mbf`, `psnd_mbf`, `tsnd_mbf`, `rcv_mbf`, `prcv_mbf`, `trcv_mbf`, `ini_mbf`, `ref_mbf`, `CRE_MBF`
- オーバランハンドラ: `def_ovr`, `sta_ovr`, `ista_ovr`, `stp_ovr`, `istp_ovr`, `ref_ovr`, `DEF_OVR`

ユーザがこれらを要求した場合は，「ASP標準パッケージでは未サポート」である旨を明示し，**サポートされる代替案**を提示する（例：`acre_tsk` → `CRE_TSK`，メッセージバッファ → メールボックス＋メモリプール，スピンロック → セマフォ）。

### 4.2 範囲外の静的API

`AID_*`（ID自動割付）／`SAC_*`（アクセス許可ベクタ）／`CRE_ISR`（ASPでは `ATT_ISR` のみ）／`CRE_MBF`／`CRE_SPN`／`DEF_SVC`／`DEF_EPR`／`LMT_DOM`／`DEF_STK`／`ATT_REG`／`DEF_SRG`／`ATT_SEC`／`ATA_SEC`／`LNK_SEC`／`ATT_MOD`／`ATA_MOD`／`ATT_MEM`／`ATA_MEM`／`ATT_PMA`／`ATA_PMA` は使わない。

### 4.3 サポート外の属性・機能

- **`TA_PHS`**（周期ハンドラ）: ASPでは未サポート。`TA_STA` と `cycphs` で対応。
- **`TA_RSTR`**（制約タスク）: ASP標準パッケージでは未サポート。
- **保護ドメインの囲み**（`DOMAIN`, `KERNEL_DOMAIN`）／**クラスの囲み**（`CLASS`）：使わない。

### 4.4 アンチパターン

| やってはいけない | 理由 | 正しいやり方 |
|---|---|---|
| ISRから `wai_sem`, `dly_tsk`, `tslp_tsk`, `loc_mtx`, `rcv_dtq`, `wai_flg` 等を呼ぶ | 待ちに入るAPIは `E_CTX` で必ず失敗 | `i` 接頭辞付きAPI（`isig_sem`, `iwup_tsk`, `iset_flg`, `ipsnd_dtq` 等）でタスクへ通知 |
| タスクから `iact_tsk`, `isig_sem` 等を呼ぶ | `E_CTX` で失敗 | `i` 接頭辞なしAPI（`act_tsk`, `sig_sem` 等） |
| マルチタスクで `for(i=0;i<N;i++)` のビジーウェイトで時間調整 | 同一優先度なら片方しか動かない（`03_led_2tasks_for` の反例） | `dly_tsk(ms)` で待ち状態に入れる |
| `dly_tsk(0)` | 仕様上「次のタイムティックまで待つ」（=0msではない） | `dly_tsk(1)` 以上を使う |
| 共有変数を排他制御なしで複数タスクから書く | 競合状態（`14_ex_none` の反例） | `wai_sem`/`sig_sem`，`loc_mtx`/`unl_mtx` |
| 割込みハンドラ内でデバイスフラグクリア忘れ | 同じ割込みが繰り返し発生 | ハンドラの最後に `*_int_clear()` を呼ぶ |
| `loc_cpu()`/`unl_cpu()` のネスト | 未定義動作 | ペアで，ネストせずに使う |
| CPUロック中に `wai_sem`, `chg_pri`, `chg_ipm`, `dis_dsp`, `act_tsk` 等 | `E_CTX` で失敗 | `loc_cpu`/`unl_cpu`/`dis_int`/`ena_int`/`sns_*`/`ext_*` のみ呼出可 |
| `CFG_INT` なしで `ATT_ISR` を書く | `E_OBJ` でビルドエラー | 必ず `CFG_INT` → `ATT_ISR` の順 |
| タスクのメインルーチンを1回で `return` させ無限ループにしない | 1度実行したら休止状態。再起動するまで動かない | 永続タスクは `for(;;)` の中で `wai_*` 系API |
| 優先度を「数字が大きいほど高優先度」と扱う | TOPPERS は逆（FreeRTOSと逆） | **値が小さいほど高優先度**。割込み優先度は負値 |
| `pdMS_TO_TICKS(ms)` 相当の変換 | TOPPERS は元々ミリ秒単位 | `dly_tsk(100)` で100ms |
| ヘッダの `extern` 宣言を `#ifndef TOPPERS_MACRO_ONLY` で囲まない | コンフィギュレータが `.cfg` 解析時にエラー | `#ifndef TOPPERS_MACRO_ONLY ... #endif` で囲む |

---

## 5. ヘッダファイルの扱い

各サンプルの `*.h` ファイル（例：`led_1task.h`）の**内容（マクロ値・関数プロトタイプ宣言）はターゲット環境ごとに異なる**ため，`example/` 内のヘッダ実体は参考値である。

エージェントは：

- ヘッダファイルの**書式パターン**は参考にする：
  ```c
  #define DEFAULT_PRIORITY    8       /* 値はターゲット環境による */
  #define STACK_SIZE          4096    /* 値はターゲット環境による */

  #ifndef TOPPERS_MACRO_ONLY
  extern void my_task(intptr_t exinf);
  #endif /* TOPPERS_MACRO_ONLY */
  ```
- 具体値（優先度・スタックサイズ・割込み番号・割込み優先度等）は **ターゲット依存部仕様** または既存プロジェクトのヘッダを参照させる。
- 自動生成時に値を勝手に決め打ちしない。ユーザに確認するか，「ターゲットの仕様書を参照すること」とコメントで明示する。

---

## 6. ファイル変更時の自己レビュー

エージェントが C ソースまたは `.cfg` を生成・編集した後，以下を確認する：

1. **ASP範囲外APIを含めていないか**（4.1, 4.2 参照）。
2. **コンテキストごとの `i` 接頭辞**は正しいか（3.4 参照）。
3. **タスクは無限ループまたは明示的 `ext_tsk()`**で終わっているか。
4. **CFG_INT → ATT_ISR の順**で記述されているか。
5. **`DEF_ICS` を1回だけ**書いているか（複数記述すると `E_OBJ`）。
6. **`#include "kernel_cfg.h"`** がCソースの冒頭にあるか。
7. **`asp_prog.cfg` 冒頭の `INCLUDE` 行5本**が揃っているか。
8. **オブジェクト識別名は全大文字＋アンダースコア**か。
9. **割込み優先度は負値**で，`TMIN_INTPRI..-1` の範囲内か。
10. **時間引数はミリ秒**として正しいか（`dly_tsk(100)` で100ms）。

---

## 7. ユーザへの応答ガイドライン

- 提案するAPI・属性が ASP 範囲外の場合は，**事前に明示的に通告**してから代替案を提示する：
  > **注意**: `acre_tsk` はASP標準パッケージでは未サポートです。`.cfg` に静的記述する `CRE_TSK` を使います。
- ターゲット依存事項（具体的な割込み番号，最小スタック値など）は**勝手に決めない**。「ターゲット仕様書を参照」と注記する。
- ビルド系ファイル（Makefile, リンカスクリプト, `.launch`）は本リポジトリにはない。ユーザがビルド設定を求めた場合は「`material/` 配下の元教材または対象ターゲットのビルド構成テンプレートを参照」と案内する。
- 説明・コメント・識別名は日本語でも英語でも可だが，**ユーザの言語に合わせる**。
- コードコメントは「なぜ」を書く。「何」は識別名から自明にする。

---

## 8. 困ったときに引くドキュメント

| 状況 | 参照先 |
|---|---|
| RTOSの基本概念（タスク状態，スケジューリング規則，割込みモデル） | `doc/TOPPERS-ASP_RTOS仕様.md` |
| サービスコール（C言語API）の正確なシグネチャ・コンテキスト・エラー | `doc/TOPPERS-ASP_API仕様.md` |
| `.cfg` の静的APIの書式・パラメータ | `doc/TOPPERS-ASP_静的API_API仕様.md` |
| エラーコード（`E_CTX`, `E_PAR`, `E_RLWAI` 等）の発生条件 | `doc/TOPPERS-ASP_静的API_エラー.md` |
| FreeRTOS/Zephyr 経験者向けの違いまとめ | `doc/TOPPERS-ASP_クイックルール.md` |
| 「こういうことをしたい」のサンプル探し | `example/README.md`（インデックス） |
| 上記で解決しない場合の原典 | TOPPERS新世代カーネル統合仕様書 Release 1.7.1 — <https://www.toppers.jp/docs/tech/ngki_spec-171.pdf> |

---

## 9. 典型タスクのエージェント手順テンプレート

### 9.1 「タスクを1つ追加してほしい」

1. `example/01_led_1task/` を雛形にコピー。
2. `*.c` のタスク関数本体を要求に合わせて修正。
3. `asp_prog.cfg` の `CRE_TSK` でタスクを追加。
4. ヘッダの関数プロトタイプを `#ifndef TOPPERS_MACRO_ONLY` で囲んで追加。
5. 自己レビュー（6章）。

### 9.2 「割込みハンドラを追加してほしい」

1. `example/08_sw_int/` を雛形に参照。割込みのみなら `08`，APIを呼ぶなら `10_sw_int_api`，起床通知なら `11_sw_int_api2`。
2. C ソースに `void xxx_handler(void)` を実装。デバイスフラグクリアを忘れずに。
3. `.cfg` に `ATT_INI`（端子初期化）→ `CFG_INT`（属性）→ `DEF_INH`（ハンドラ登録）の3点セットを記述。
4. ハンドラ内で API を呼ぶなら **`i` 接頭辞付き** + **`SVC_PERROR()`** でラップ。
5. 自己レビュー。

### 9.3 「タスク間でデータを送りたい」

- 単一値の通知（イベント発生のみ） → セマフォ（`15_ex_sem`）または起床要求（`11_sw_int_api2`）。
- 複数イベントの種別を区別 → イベントフラグ（`16_sw_flg_cyc` / `17_sw_flg_tsk`）。
- データの受け渡し（`intptr_t` 1個分） → データキュー（`18_sw_dtq_cyc` / `19_sw_dtq_tsk`）。
- 構造体の受け渡し → メールボックス（ポインタ送受信）。本サンプル群には未掲載のため `doc/TOPPERS-ASP_API仕様.md` 4.5節を参照。

### 9.4 「共有変数を排他制御したい」

- 単純なタスク間排他 → セマフォ（`15_ex_sem` 参照），または優先度逆転対策が必要なら **ミューテックス**（`doc/TOPPERS-ASP_API仕様.md` 4.6節）。
- 割込みハンドラとの排他 → CPUロック（`12_cpu_lock`）または割込み優先度マスク（`13_int_mask`）。
- 影響範囲・割込み応答性のトレードオフは `doc/TOPPERS-ASP_クイックルール.md` の比較表を参照。

### 9.5 「定期実行したい」

- 周期ハンドラ（`07_led_cyc`）が第一選択。
- 周期内で待ちに入る必要があるなら，タスク化して `dly_tsk(ms)` で周期を作る（`04_led_2tasks` 参照）。

### 9.6 「初期化処理を追加したい」

- カーネル動作前に行う初期化 → `ATT_INI` で登録（`05_led_2tasks_init` 参照）。
- 終了処理は `ATT_TER` で登録（記述順とは**逆順**で実行されることに注意）。

---

## 10. メンテナンスメモ

このAGENTS.mdが古くなった兆候：

- TOPPERSプロジェクトが新しい仕様（Release 1.7.1 以降）を出している。
- `example/` に新しいサンプルが追加されている／既存の構成が変わっている。
- 教材 `material/` の内容が更新されている。

これらが起きた場合，本AGENTS.mdの該当節（特に2章のリポジトリ構成，4章の範囲外API一覧）を更新すること。
