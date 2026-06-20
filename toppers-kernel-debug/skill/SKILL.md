---
name: toppers-kernel-debug
description: TOPPERS系RTOS（ASP/ASP3/FMP3/HRP3 等 ITRON系）の不具合診断・動作検証のための切り分けskill（実装非依存）。ハードフォルト／ハング／タスクがディスパッチされない／優先度逆転／スタックオーバーフロー／サービスコールが想定外の `E_*` を返す、といった症状を原因へ辿るとき、カーネルのトレース/ログ機能で実行を観測するとき、GDBのOS Awareness（タスク一覧・状態）を使うとき、機械判定（TAP等）のテスト失敗を切り分けるとき、TTSP3（TOPPERSテストスイート）で適合性を確認するとき、FMP系（マルチプロセッサ/SMP）でPE間・遅延ディスパッチ枝やサブ優先度・割込みアフィニティのテスト設計やカバレッジ差を考えるとき、SMPの間欠ハング／タイミング依存レースを決定的に再現したいとき、ホストOS上で擬似実行する移植（POSIX等・タスク＝ホストスレッド）でディスパッチ窓のレース・per-PE状態の汚染・シグナル乗っ取り・lost-wakeup を切り分けるとき、適合性テストの分岐(C1)カバレッジを考えるとき・gcov が効かない手書きアセンブリ（`.S`：起動/コンテキスト切替/割込み・例外入口）の命令/分岐網羅を実行トレースで測るとき・タスクから踏めない実行時コンテキスト（アイドル＝実行中タスク無し等）の防御/復帰経路へ正規フック点を流用してカーネル無改変で到達させたいとき、実行時間分布(histogram)やサイクルカウンタでコンテキストスイッチ/サービスコールの所要時間を計測するとき・エミュレータ(QEMU等)でサイクルカウンタが0/一定になる現象を切り分けるとき、`E_CTX`/`E_PAR`/`E_ID`/`E_OBJ`/`E_RLWAI`/`E_QOVR` 等がなぜ出るかを調べるときに使う。ユーザが「ハードフォルト」「動かない」「ハングした」「間欠ハング」「タスクが動かない」「優先度逆転」「レースを再現」「POSIXシミュ」「ホストシミュレーション」「なぜこのエラー」「OS Awareness」「TTSP3」「テストが落ちる」「カバレッジ」「分岐網羅」「アセンブリのカバレッジ」「到達不能な経路」「アイドル中フック」「性能計測」「実行時間分布」「サイクルカウンタ」「QEMUでカウンタが0」、また TrustZone-M / dual-OS（Secure＋Non-secure）構成で「セキュアデバッグでhaltできない」「Secure gateで落ちる」「意図的例外テストがデバッガで止まる/自走で取りたい」「ワンショット出力が0バイト」「QEMUで遷移(secure gate)が再現しない」「FP退避でHardFault」と言ったときに発動。**エラーコードの網羅辞書・API仕様は別skill `toppers-asp`、トレース機能・テストランナの具体コマンドはリポジトリ固有（各リポジトリの専用skill/`docs`）。本skillは症状→原因の切り分けが中心。**
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

### マルチプロセッサ（FMP系）派生のテスト

SMP（FMP系）カーネルは、PE間・遅延ディスパッチ枝／タイムマスタへの時間イベント転送／
割込み番号のPEアフィニティ／割込みコントローラ方式によるテストのゲート／PE間状態観測の同期点／
サブ優先度の並び替えなど、**単核（ASP系）には無い差**を持ち、これがテスト設計とカバレッジに効く。
SMPで分岐カバレッジが単核より低く出るのは多くが正常（並行シナリオ不在）。概念は
[`references/multiprocessor-testing.md`](references/multiprocessor-testing.md)。

---

## 4. カバレッジと「到達不能な経路」（検証の網羅）

適合性テストで防御/復帰経路が実際に踏まれたかを確かめるには、**分岐(C1)カバレッジ**を見る
（行だけでは不足）。ここで2つの実装非依存の壁にぶつかる。

- **gcov が効かない手書きアセンブリ（依存部の起動・コンテキスト切替・割込み/例外入口）**は、
  逆アセンブル × 実行トレース × デバッグ行情報の突合で C0/C1 を測る。トレース爆発・
  デバッグ情報版差・別リンクの合算といった**ツールチェーン一般の落とし穴**がある。
  → [`references/hand-written-assembly-coverage.md`](references/hand-written-assembly-coverage.md)。
- **特定の実行時コンテキスト（実行中タスク無し＝アイドル等）でしか踏まれない経路**は、
  普通のテストタスクからは到達できない。**正規のフック点（アイドル中フック等）を計測用の
  刺激注入口に流用**し、カーネル無改変のまま決定的に届かせる（最初の進入で起こす・ハンドラは
  最小復帰・SMPは自PE固定が要点）。
  → [`references/reaching-unreachable-contexts.md`](references/reaching-unreachable-contexts.md)。

> 100%を追わず、届かない分は**残存分岐を分類して台帳化**する規律と組み合わせる
> （SMP起因は [`references/multiprocessor-testing.md`](references/multiprocessor-testing.md)、
> 計測スクリプト・台帳ファイル名はリポジトリ固有）。

---

## 5. 実行時間の計測（性能観測）

コンテキストスイッチやサービスコールの所要時間を測るとき：

- 実行時間分布(histogram)サービスの**時間源は差し替え可能**。既定の低分解能システム時刻
  （μs級）ではサブμsの処理が量子化されるので、サブμs（サイクル/ns）が要るときはアーキの
  サイクルカウンタを時間源にする。差し替えは「サービス無改変・アーキ共通層のフック上書き・
  オプトイン・コア初期化で有効化・**変換係数はクロック依存なのでターゲット/ボード層**」という
  アーキ横断の層構造で行う（新アーキは他アーキの同型フックに倣う）。
- **エミュレータはサイクル/性能カウンタを実装しないことが多い**（0/一定を返す）。しかも
  「非実装」を示す状態ビットが**実装ありのまま**のことがあるので信用せず、**実行時に
  カウンタが実際に進むかプローブ**してから使う。進まなければ低分解能へフォールバック or
  SKIP し、計測は実機で行う。「実装ありに見えるのに0」をコードのバグと誤認しない。

→ [`references/performance-measurement.md`](references/performance-measurement.md)。
具体のフラグ名・カウンタ番地・ビルド手順はリポジトリ固有（専用skill/`docs`）。

---

## 6. TrustZone-M / Secure境界 dual-OS の診断

ARMv8-M Security Extension の dual-OS モニタ型（Secure 側 RTOS＋Non-secure OS）特有の罠：

- **セキュアデバッグ・ロック**：Secure ファーム実走後に SWD の halt/examine が効かなくなる
  ことがある（接続は通るが halt がタイムアウト）。SWD反復書込みが破綻→ブートローダ経路で焼く。
- **常駐デバッガが意図的例外テストを隠す**：CPU フォルトで止まるデバッガは firmware の例外
  ハンドラより先に halt する→意図的例外の捕捉テストは**デバッガ非接続で自走**させて観測。
- **ワンショット出力は capture を reset より先に**起動（「0バイト」＝捕捉遅れのことがある）。
- **機械可読マーカは同期(即時)出力**に（低優先ログタスクのドレインはプリエンプト/別世界実行中に届かない）。
- **エミュレータの SG/Secure例外 忠実度限界**：SAU/FPU は忠実でも Secure Gateway/Secure例外を
  実機どおり再現しないことがある（複数版で同一・実機で出ない＝エミュ限界）。境界跨ぎ遷移の正は実機。
- **FPU 遅延スタッキング残渣の文脈跨ぎ**：境界 gate veneer の FP スクラッチで `CONTROL.FPCA`
  が残置→後の横取り切替の FP 退避がフォルト。代理(モニタ)タスクは FP文脈レス扱いにする。

→ [`references/trustzone-dual-os-debugging.md`](references/trustzone-dual-os-debugging.md)。
固有のフラグ・番地・取得コマンドはリポジトリ固有（専用skill/`docs`）。

## references/

| ファイル | 引くタイミング |
|---|---|
| [`references/observing-execution.md`](references/observing-execution.md) | トレース/ログで何をどう観測するか、読み解きの着眼点。実機 attach 観測。**カーネルに OS-awareness 機構が無いとき自作する作法**（静的const/動的CBの別・名前は生成cfgから・キュー/状態の読み解き・優先度ビットマップのMSB詰め・割込みコントローラのレジスタ観測・スタック使用量の概算・Python対応gdb） |
| [`references/debugging-playbook.md`](references/debugging-playbook.md) | ハードフォルト/ハング/優先度逆転/スタック溢れ/コンテキストエラー等の症状→原因→確認手順 |
| [`references/conformance-ttsp3.md`](references/conformance-ttsp3.md) | TTSP3 の概念・PASS/FAIL/SKIP の解釈・移植検証での使い方 |
| [`references/multiprocessor-testing.md`](references/multiprocessor-testing.md) | FMP系（SMP）派生のテスト・カバレッジ概念（PE間枝・時刻転送・割込みアフィニティ・IRC方式・PE間同期・サブ優先度）＋SMPレースの決定的再現法（最小リバート帰属・自己マイグレーション往復） |
| [`references/host-simulation-port.md`](references/host-simulation-port.md) | ホスト上で擬似実行する移植（POSIX等・タスク＝ホストスレッド）のデバッグ着眼（決定/担体分離のレース窓・per-PE汚染・シグナル乗っ取り・lost-wakeup） |
| [`references/reaching-unreachable-contexts.md`](references/reaching-unreachable-contexts.md) | タスクから踏めない実行時コンテキスト（アイドル＝実行中タスク無し等）の防御/復帰経路へ、正規フック点を計測用刺激注入口に流用してカーネル無改変で到達する作法（最初の進入で起こす・ハンドラ最小復帰・SMPは自PE固定） |
| [`references/hand-written-assembly-coverage.md`](references/hand-written-assembly-coverage.md) | gcov が効かない手書きアセンブリ（`.S`）の C0/C1 を実行トレース×デバッグ行情報で測る原理と落とし穴（トレース爆発の drain＋timeout・デバッグ情報版差・別リンクの行空間合算・union前クリーン再ビルド） |
| [`references/performance-measurement.md`](references/performance-measurement.md) | 実行時間分布(histogram)の時間源差し替え（μs時刻→アーキのサイクルカウンタ）の層構造＝サービス無改変・オプトイン・コア初期化で有効化・変換係数はターゲット/ボード層／エミュレータはサイクル/性能カウンタを実装せず0を返す（"非実装"ビットが実装ありのままの罠）＝実行時プローブして実機でのみ計測 |
| [`references/trustzone-dual-os-debugging.md`](references/trustzone-dual-os-debugging.md) | TrustZone-M dual-OS（Secure＋Non-secure）特有の診断：セキュアデバッグ・ロック（halt不可→ブートローダ経路で焼く）／常駐デバッガが意図的例外テストを隠す（デバッガ非接続で自走観測）／ワンショット出力は capture を reset より先に／機械可読マーカは同期(即時)出力／エミュレータの SG・Secure例外 忠実度限界（境界跨ぎ遷移の正は実機）／FPU遅延スタッキング残渣の文脈跨ぎ（代理タスクはFP文脈レス扱い） |

エラー辞書・API仕様は `toppers-asp`、カーネル作業規約は `toppers-kernel-dev`、
実装固有の手順は各リポジトリの `AGENTS.md`／`docs/`／専用skillを参照。
