---
name: toppers-kernel-debug
description: TOPPERS系RTOS（ASP/ASP3/FMP3/HRP3 等 ITRON系）の不具合診断・動作検証のための切り分けskill（実装非依存）。ハードフォルト／ハング／タスクがディスパッチされない／優先度逆転／スタックオーバーフロー／サービスコールが想定外の `E_*` を返す、といった症状を原因へ辿るとき、カーネルのトレース/ログ機能で実行を観測するとき、GDBのOS Awareness（タスク一覧・状態）を使うとき、機械判定（TAP等）のテスト失敗を切り分けるとき、TTSP3（TOPPERSテストスイート）で適合性を確認するとき、`E_CTX`/`E_PAR`/`E_ID`/`E_OBJ`/`E_RLWAI`/`E_QOVR` 等がなぜ出るかを調べるときに使う。ユーザが「ハードフォルト」「動かない」「ハングした」「タスクが動かない」「優先度逆転」「なぜこのエラー」「OS Awareness」「TTSP3」「テストが落ちる」と言ったときに発動。**エラーコードの網羅辞書・API仕様は別skill `toppers-asp`、トレース機能・テストランナの具体コマンドはリポジトリ固有（各リポジトリの専用skill/`docs`）。本skillは症状→原因の切り分けが中心。**
---

# TOPPERS系RTOS デバッグ・検証の切り分け（実装非依存）

ITRON系RTOS（ASP/ASP3 等）の不具合を、組込みRTOS特有の観点
（ディスパッチ・割込み・優先度・コンテキスト・スタック）で**症状から原因へ辿る**ためのskill。

> - エラーコードの意味の網羅辞書・API仕様は別skill **`toppers-asp`**（`references/error-codes.md`/`api-spec.md`）。
> - カーネル側の作業規約（禁則・移植・上流マージ）は別skill **`toppers-kernel-dev`**。
> - **トレース機能の出力形式・解析スクリプト・テストランナの具体コマンドはリポジトリ固有**。
>   各リポジトリの `AGENTS.md`／`docs/`／専用skillに従う。本skillは原則と切り分けに徹する。

---

## 1. 診断の道具（概念）

### (a) カーネルのトレース/ログで実行を観測する

TOPPERSカーネルは**実行イベントを観測する手段**を持つ（トレースログ機能、`syslog`、
ログマクロ等。出力形式・有効化方法はカーネル/リポジトリ依存）。まず「いつ・どのタスクが・
どの状態になり・どのサービスコールがどう復帰したか」を時系列で取り出す。観測すべき軸：

- **ディスパッチ**：最初に実行が始まるタスク／切り替えの列（期待どおりの優先度順か）。
- **タスク状態遷移**：RUNNING/READY(RUNNABLE)/WAITING/SUSPENDED/WAITING-SUSPENDED(二重待ち)/DORMANT の遷移。
- **サービスコールの出入りと復帰値**：どのAPIが `E_*` を返したか。
- **割込み/例外/周期/アラームハンドラの出入り**：想定外の例外が起きていないか。

詳しい観測ポイントは [`references/observing-execution.md`](references/observing-execution.md)。

### (b) GDB の OS Awareness

OS Awareness 対応ビルドでは GDB がタスク（スレッド）を認識し、`info threads` 相当で
**タスク一覧・各タスクの状態・スタック/レジスタ**を一望できる。ハング・優先度問題で
「いまどのタスクがどの状態か」を確認するのに有効。

---

## 2. 症状から原因へ（プレイブック）

代表症状の切り分けは [`references/debugging-playbook.md`](references/debugging-playbook.md)。早見：

| 症状 | まず疑う | 最初の一手 |
|---|---|---|
| ハードフォルト／例外で停止 | スタック溢れ・不正ベクタ・メモリ保護(MPU/PMP/PSPLIM)違反・整列違反 | フォルトステータス／例外ハンドラの発生／スタック上限 |
| 何も起きない／ハング | 最初のディスパッチが起きない・割込み未許可・全タスク待ち | 実行開始タスクの有無、初期化ルーチン、割込み許可 |
| タスクが動かない | 優先度・`act_tsk` 漏れ・待ちが解除されない | 状態遷移とサービスコールの復帰を追う |
| 優先度逆転っぽい | ミューテックス属性（優先度継承/上限の有無）・割込み優先度マスク | 関係タスクの状態と保持ミューテックス |
| `E_CTX` が返る | 非タスクコンテキストで待ち系API／その逆 | エラーを返したAPIと呼出コンテキスト |
| `E_PAR`/`E_ID`/`E_OBJ` | 引数範囲・存在しないID・状態不正 | `toppers-asp` のエラー辞書と突合 |

エラーコード値の意味の正本は `toppers-asp/references/error-codes.md`。本skillは
「その症状でなぜそのエラーが出るか」の切り分けに使う。

---

## 3. 適合性テスト（TTSP3）

**TTSP3** は TOPPERS が提供する適合性テストスイート（API/SIL の仕様適合を網羅検証）。
新ターゲットの移植検証や回帰確認に使う。概念と読み方は
[`references/conformance-ttsp3.md`](references/conformance-ttsp3.md)。

- **SKIP** は未対応HW依存（HWタイマ制御・割込み生成等）＝想定内のことが多い。
- **FAIL** は「ターゲット依存の差（cfgエラーコード差等）」か「真の不具合」かを切り分ける。

> TTSP3 の起動方法（Makefile／各リポジトリのドライバ）はリポジトリ固有。

---

## references/

| ファイル | 引くタイミング |
|---|---|
| [`references/observing-execution.md`](references/observing-execution.md) | トレース/ログで何をどう観測するか、読み解きの着眼点 |
| [`references/debugging-playbook.md`](references/debugging-playbook.md) | ハードフォルト/ハング/優先度逆転/スタック溢れ/コンテキストエラー等の症状→原因→確認手順 |
| [`references/conformance-ttsp3.md`](references/conformance-ttsp3.md) | TTSP3 の概念・PASS/FAIL/SKIP の解釈・移植検証での使い方 |

エラー辞書・API仕様は `toppers-asp`、カーネル作業規約は `toppers-kernel-dev`、
実装固有の手順は各リポジトリの `AGENTS.md`／`docs/`／専用skillを参照。
