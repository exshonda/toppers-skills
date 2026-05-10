# TOPPERS/ASP カーネル 静的API・サービスコール エラーコード仕様

本書は TOPPERS新世代カーネル統合仕様書 Release 1.7.1（<https://www.toppers.jp/docs/tech/ngki_spec-171.pdf>）のうち TOPPERS/ASPカーネル に関するエラーコードと，静的API／サービスコールでエラーが検出される条件を整理したものである。

LLMがコードを生成・レビューする際に，どの状況でどのエラーが返るかを把握し，適切なエラー処理コードを提案するためのリファレンスである。

## 0. エラーコード機構

### 0.1 エラーコードの構造

エラーコードは負の値の符号付き整数であり，**メインエラーコード**と**サブエラーコード**から構成される。

```c
ER ercd = ERCD(MERCD mercd, SERCD sercd);   /* 構成 */
ER mercd = MERCD(ER ercd);                  /* メインエラーコード抽出 */
ER sercd = SERCD(ER ercd);                  /* サブエラーコード抽出 */
```

カーネル本体はサブエラーコードを使用せず，常に `-1` を返す。アプリケーションコード上はメインエラーコードのみを判定すればよい。

### 0.2 正常終了とエラーの判定

```c
ER ercd = wai_sem(SEM_BUTTON);
if (ercd < 0) {
    /* エラー（負値） */
    syslog(LOG_ERROR, "wai_sem failed: %d", ercd);
} else {
    /* 正常終了（E_OK = 0 もしくは正値） */
}
```

代表的な正常終了：
- `E_OK` = 0 — すべてのサービスコールに共通

ID取得型サービスコール（`acre_*` など）は正の値（生成されたID）を返す。`ER_ID` 型で受ける。ASPでは静的生成のみのため通常は使わない。

`ER_UINT` を返す `can_act`/`can_wup` は正常時にキャンセルした要求数を返す。

`bool_t` を返す `sns_***` 系は真偽値のみ返す（エラーは返さない）。

### 0.3 静的APIにおけるエラー

静的APIは返値を持たないため，コンフィギュレータがコンパイル時にエラーを検出してビルドを失敗させる。エラーメッセージにはエラーコード名・該当行・原因が出力される。

例（コンフィギュレータ出力）:
```
sample.cfg:25: E_PAR: itskpri (=20) is out of range (1..16)
```

## 1. メインエラーコード一覧

ASPで発生する可能性のあるメインエラーコードは次の通り（保護機能・動的生成・マルチプロセッサ専用エラーは除外）。

| エラーコード | 値 | 意味 |
|---|---|---|
| `E_OK` | 0 | 正常終了 |
| `E_SYS` | -5 | システムエラー |
| `E_NOSPT` | -9 | 未サポート機能 |
| `E_RSFN` | -10 | 予約機能コード |
| `E_RSATR` | -11 | 予約属性 |
| `E_PAR` | -17 | パラメータエラー |
| `E_ID` | -18 | 不正ID番号 |
| `E_CTX` | -25 | コンテキストエラー |
| `E_ILUSE` | -28 | サービスコール不正使用 |
| `E_NOMEM` | -33 | メモリ不足 |
| `E_OBJ` | -41 | オブジェクト状態エラー |
| `E_NOEXS` | -42 | オブジェクト未登録 |
| `E_QOVR` | -43 | キューイングオーバフロー |
| `E_RLWAI` | -49 | 待ち禁止状態または待ち状態の強制解除 |
| `E_TMOUT` | -50 | ポーリング失敗またはタイムアウト |
| `E_DLT` | -51 | 待ちオブジェクトの削除または再初期化 |
| `E_CLS` | -52 | 待ちオブジェクトの状態変化 |
| `E_BOVR` | -58 | バッファオーバフロー |

ASP範囲外（参考）：
- `E_MACV` (-26): メモリアクセス違反 — 保護機能対応カーネルのみ
- `E_OACV` (-27): オブジェクトアクセス違反 — 保護機能対応カーネルのみ
- `E_NOID` (-34): ID番号不足 — 動的生成パッケージのみ
- `E_NORES` (-35): 資源不足 — 主に動的生成パッケージ
- `E_WBLK` (-57): ノンブロッキング受付け — システムサービス向け

## 2. メインエラー別の発生条件

### 2.1 E_OK（=0, 正常終了）

すべての処理が正常に完了した。ID取得型サービスコールでは正の値（ID）が返り，`E_OK` ではないが「エラーでない」状態。

### 2.2 E_SYS（-5, システムエラー）

カーネルの内部エラーや想定外の状態を示す。アプリケーションが原因ではなく，カーネルの不具合かハードウェア異常を疑う。通常のプログラミングでは発生しない想定。

### 2.3 E_NOSPT（-9, 未サポート機能）

そのカーネルパッケージでサポートされていないAPIや機能が要求された場合に発生。

ASPで `E_NOSPT` が返る代表的ケース：

- `dis_int`/`ena_int`: ターゲットがこの機能をサポートしていない場合（構成マクロ `TOPPERS_SUPPORT_DIS_INT`/`TOPPERS_SUPPORT_ENA_INT` が未定義のとき）。
- 動的生成パッケージで未サポートのAPI（ASP標準では発生しない）。

### 2.4 E_RSFN（-10, 予約機能コード）

カーネルが認識しない機能コードでサービスコールが呼ばれた場合（拡張サービスコールに無効な機能コードを指定したとき等）。ASPは拡張サービスコール非対応のため，アプリケーションコードからはほぼ発生しない。

### 2.5 E_RSATR（-11, 予約属性）

オブジェクト属性に無効なビットが設定されている。例：

- `CRE_TSK` の `tskatr` に未定義のビットを指定
- `CRE_SEM` の `sematr` に `TA_TPRI` 以外のビット（FIFO順は `TA_NULL` で指定）
- `CRE_FLG` の `flgatr` に `TA_TPRI`/`TA_WMUL`/`TA_CLR` 以外のビット
- ASPで保護ドメイン関連属性を指定（`TA_DOM(...)` など）
- `DEF_INH` の `inhatr` でカーネル管理外の割込みでないのに `TA_NONKERNEL` を指定
- 廃止された属性 `TA_TFIFO`／`TA_MFIFO`／`TA_HLNG`／`TA_WSGL` を指定（ASP仕様で廃止）

コンフィギュレータがビルド時に検出する場合（`〔S〕` のエラー）と，サービスコール呼出時にカーネルが検出する場合（`〔s〕` のエラー）がある。

### 2.6 E_PAR（-17, パラメータエラー）

サービスコール／静的APIに渡されたパラメータ値が無効。

代表的な発生条件：

- `CRE_TSK`: `task` が不正な番地，`itskpri` が `1..TMAX_TPRI` 範囲外，`stksz` が0以下またはターゲット最小値未満
- `CRE_SEM`: `maxsem` が `1..TMAX_MAXSEM` 範囲外，`isemcnt` が `0..maxsem` 範囲外
- `CRE_CYC`: `cyctim` が `0..TMAX_RELTIM` 範囲外（0は不可），`cycphs` が `0..TMAX_RELTIM` 範囲外
- `CFG_INT`: `intno` が有効範囲外，`intpri` が `TMIN_INTPRI..TMAX_INTPRI(=-1)` 範囲外
- `chg_pri`: `tskpri` が有効範囲外
- `twai_sem`/`tslp_tsk`/`tloc_mtx` 等: `tmout` が無効値（`TMO_FEVR`(-1), `TMO_POL`(0), 正値以外）
- `chg_ipm`: `intpri` が範囲外（`TIPM_ENAALL`〜`TMIN_INTPRI` 以外，もしくは正値）
- `set_flg`/`clr_flg`/`wai_flg`: ビットパターンが0（`set_flg(...,0x0)` 等は許される実装が多いが仕様上0は意味がない）
- `wai_flg`/`pol_flg`/`twai_flg`: `wfmode` が `TWF_ORW` または `TWF_ANDW` のいずれでもない
- `dly_tsk`: `dlytim` が `TMAX_RELTIM` を超える

### 2.7 E_ID（-18, 不正ID番号）

サービスコールに渡されたID番号が有効範囲外。例：`act_tsk(99)` でID 99のタスクがそもそも登録されていない場合。

ASPでは静的生成のみのため，コード上は `kernel_cfg.h` 由来のマクロを使う限り発生しない。ハードコードされた数値や負値・ゼロを渡すと発生しうる。

### 2.8 E_CTX（-25, コンテキストエラー）

サービスコールが許可されていないコンテキストや状態から呼ばれた。**最も頻繁に発生するエラー**で，コード生成時に最も注意すべき。

代表的な発生条件：

| 違反 | 例 |
|---|---|
| 非タスクコンテキストからタスクコンテキスト専用APIを呼び出し | ISRから `wai_sem`, `slp_tsk`, `act_tsk`(iact_tskではない), `sig_sem`(isig_semではない) など |
| タスクコンテキストから非タスクコンテキスト専用APIを呼び出し | タスクから `iact_tsk`, `isig_sem`, `iset_flg` など |
| CPUロック状態から多くのサービスコールを呼び出し | `loc_cpu()` 後に `wai_sem`, `chg_pri`, `chg_ipm`, `dis_dsp`, `act_tsk` 等 |
| ディスパッチ保留状態（CPUロック・優先度マスク・dis_dsp）から待ちに入る可能性のあるサービスコール | `wai_sem`, `slp_tsk`, `tslp_tsk`, `wai_flg`, `twai_flg`, `rcv_dtq`, `trcv_dtq`, `tsnd_dtq`, `loc_mtx`, `tloc_mtx`, `get_mpf`, `tget_mpf`, `dly_tsk` 等 |
| 割込み優先度マスクが全解除でない状態から待ちに入るサービスコール | 上に同じ |
| ポーリング型APIをディスパッチ保留状態から呼び出し | `pol_sem`, `pol_flg`, `prcv_dtq`, `ploc_mtx`, `pget_mpf` は CPUロック中のみ E_CTX。dis_dsp や優先度マスク中はOK |

CPUロック中に呼べるサービスコール（E_CTX にならない）：
- `loc_cpu`/`unl_cpu`, `iloc_cpu`/`iunl_cpu`
- `dis_int`/`ena_int`
- `sns_*`（すべてのセンサ系）
- `xsns_dpn`, `xsns_xpn`（CPU例外ハンドラからのみ）
- `get_utm`
- `ext_tsk`, `ext_ker`

### 2.9 E_ILUSE（-28, サービスコール不正使用）

サービスコールの使い方が不正。例：

- `wai_flg`/`twai_flg`: `TA_WMUL` 属性でないイベントフラグに2人目以降のタスクが待ちに入ろうとした
- `unl_mtx`: ロックしていないミューテックスをアンロック，または最後にロックしたミューテックスでない（ネスト順違反）
- `loc_mtx`: 同じミューテックスを既にロックしているタスクが再度ロック（再帰禁止）
- `loc_mtx`: `TA_CEILING` 属性のミューテックスで現在優先度が上限優先度より高いタスクがロックを試行
- `chg_pri`: `TA_CEILING` ミューテックスをロック中のタスクのベース優先度を，上限優先度より低くしようとした

### 2.10 E_NOMEM（-33, メモリ不足）

オブジェクトに必要なメモリ領域が確保できない。コンフィギュレータでは検出できない場合があり，カーネルの初期化処理時または `acre_*`（動的生成）で検出される。ASPでは静的生成のみのため，コンフィギュレータが警告できるサイズ違反以外は通常発生しない。

代表的な発生：
- `CRE_TSK`/`CRE_DTQ`/`CRE_MPF` 等で `NULL` を指定したのにカーネルがメモリを確保できない（リンク時失敗）

### 2.11 E_OBJ（-41, オブジェクト状態エラー）

対象オブジェクトの状態が，要求された操作を許さない。例：

| サービスコール | E_OBJ になる条件 |
|---|---|
| `act_tsk` | （特になし。常に成功するか起動要求がキューイングされる） |
| `ter_tsk` | 自タスクを対象に指定（自タスクに対しては不可） |
| `chg_pri` | 対象タスクが休止状態，TA_CEILING ミューテックスとの整合違反 |
| `def_tex` | 対象タスクに既にタスク例外処理ルーチンが定義済み |
| `loc_mtx` 系 | 既に同タスクがロック中（再帰禁止） |
| `unl_mtx` | 自タスクがロックしていない／最後にロックしたものではない |
| `sta_alm` | 動作中のアラームを再起動するのは可（再設定） |
| `stp_alm` | 動作していないアラームを停止するのも可（無効化される） |
| `CRE_TSK` 等の静的API | 同名の識別名で生成済み |
| `ATT_ISR` | 対象割込み番号に既に `DEF_INH` で割込みハンドラが定義されている |
| `CFG_INT` | 同じ割込み番号に既に属性設定済み（静的API側のみ） |
| `DEF_ICS` | 既に定義済み（複数記述） |
| `def_inh` | 対象割込みハンドラ番号に既に登録済み |

### 2.12 E_NOEXS（-42, オブジェクト未登録）

操作対象のオブジェクトが登録されていない。ASPでは静的生成のみのため，識別名がコンフィギュレータで処理された後は発生しないが，動的生成サービスコール由来のID番号を不正に使用したり，まだ登録されていない番号を使うと発生。

### 2.13 E_QOVR（-43, キューイングオーバフロー）

要求のキューが上限に達した。

- `act_tsk`/`iact_tsk`: 起動要求キューイング数が `TMAX_ACTCNT`（ASPは1固定）に達している
- `wup_tsk`/`iwup_tsk`: 起床要求キューイング数が上限（`TMAX_WUPCNT`）に達している
- `sig_sem`/`isig_sem`: セマフォの資源数が最大資源数 `maxsem` に達している

### 2.14 E_RLWAI（-49, 待ち状態の強制解除）

タスクが待ち状態のときに `rel_wai`/`irel_wai` で強制的に待ちが解除された場合，**待ちに入っていたサービスコールから** `E_RLWAI` が返る。

発生する可能性のあるサービスコール：
- `slp_tsk`, `tslp_tsk`
- `wai_sem`, `twai_sem`
- `wai_flg`, `twai_flg`
- `snd_dtq`, `tsnd_dtq`, `rcv_dtq`, `trcv_dtq`
- `snd_pdq`, `tsnd_pdq`, `rcv_pdq`, `trcv_pdq`
- `snd_mbx`, `rcv_mbx`, `trcv_mbx`
- `loc_mtx`, `tloc_mtx`
- `get_mpf`, `tget_mpf`
- `dly_tsk`

ポーリング系（`pol_sem`, `prcv_dtq`, `ploc_mtx`, `pget_mpf`, `pol_flg`）は待ちに入らないので `E_RLWAI` は返らない。

### 2.15 E_TMOUT（-50, タイムアウトまたはポーリング失敗）

- タイムアウト付きサービスコールでタイムアウトが発生
- ポーリング系サービスコールで条件が満たされていない（資源なし・ブロックなし・データなし等）

`tmout=TMO_POL(0)` でも `E_TMOUT`，正の `tmout` でタイムアウト時刻に達しても `E_TMOUT`。

### 2.16 E_DLT（-51, 待ちオブジェクトの削除または再初期化）

待ち状態のタスクが，待ちオブジェクトの再初期化（`ini_sem`, `ini_flg`, `ini_dtq`, `ini_pdq`, `ini_mbx`, `ini_mtx`, `ini_mpf`）または削除（ASPはサポートしないが動的生成パッケージで `del_*`）によって待ちが解除された場合，待ちに入っていたサービスコールから `E_DLT` が返る。

ASP標準パッケージで主に発生するのは `ini_*` 経由。

### 2.17 E_CLS（-52, 待ちオブジェクトの状態変化）

待ちオブジェクトの状態がアプリケーションの想定外に変化したことを示す（主に保護機能対応カーネルや動的属性変更を伴うAPIで使用）。ASP標準では発生しないと考えてよい。

### 2.18 E_BOVR（-58, バッファオーバフロー）

メッセージバッファ（HRP2系）でメッセージサイズが上限を超えた等の場合。ASPでは未サポート機能関連のため通常発生しない。

## 3. 静的API別エラー条件詳細

### 3.1 CRE_TSK のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `tskatr` に `TA_ACT`/`TA_FPU` 以外の無効ビットが含まれる |
| `E_PAR` | `task` がプログラムの先頭番地として正しくない |
| `E_PAR` | `itskpri` が `1..TMAX_TPRI` の範囲外 |
| `E_PAR` | `stksz` が0以下，またはターゲット定義の最小値未満 |
| `E_PAR` | `stk` が指定されていてターゲット定義の制約に合致しない先頭番地 |
| `E_NOMEM` | `stk=NULL` 指定でカーネルがスタック領域を確保できない |
| `E_OBJ` | `tskid` で指定した識別名が既に登録済み |

### 3.2 CRE_SEM のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `sematr` が無効 |
| `E_PAR` | `maxsem` が `1..TMAX_MAXSEM` 範囲外 |
| `E_PAR` | `isemcnt` が `0..maxsem` 範囲外 |
| `E_OBJ` | 同名のセマフォが登録済み |

### 3.3 CRE_FLG のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `flgatr` が無効 |
| `E_OBJ` | 同名のフラグが登録済み |

### 3.4 CRE_DTQ のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `dtqatr` が無効 |
| `E_PAR` | `dtqcnt` が範囲外（0は許される） |
| `E_NOMEM` | `dtqmb=NULL` 指定でカーネルが管理領域を確保できない |
| `E_OBJ` | 同名のキューが登録済み |

### 3.5 CRE_PDQ のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `pdqatr` が無効 |
| `E_PAR` | `pdqcnt` が範囲外，`maxdpri` が `1..TMAX_DPRI` 範囲外 |
| `E_NOMEM` | カーネルが管理領域を確保できない |
| `E_OBJ` | 同名の優先度キューが登録済み |

### 3.6 CRE_MBX のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `mbxatr` が無効 |
| `E_PAR` | `TA_MPRI` 属性で `maxmpri` が `1..TMAX_MPRI` 範囲外 |
| `E_OBJ` | 同名のメールボックスが登録済み |

### 3.7 CRE_MTX のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `mtxatr` が無効 |
| `E_PAR` | `TA_CEILING` 属性で `ceilpri` が `1..TMAX_TPRI` 範囲外 |
| `E_OBJ` | 同名のミューテックスが登録済み |

### 3.8 CRE_MPF のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `mpfatr` が無効 |
| `E_PAR` | `blkcnt` が範囲外，`blksz` が0以下 |
| `E_NOMEM` | カーネルがメモリプール領域・管理領域を確保できない |
| `E_OBJ` | 同名のメモリプールが登録済み |

### 3.9 CRE_CYC のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `cycatr` が無効（ASPでは `TA_PHS` を含むと無効） |
| `E_PAR` | `cychdr` が不正な番地 |
| `E_PAR` | `cyctim` が `1..TMAX_RELTIM` 範囲外（0は不可） |
| `E_PAR` | `cycphs` が `0..TMAX_RELTIM` 範囲外 |
| `E_OBJ` | 同名の周期ハンドラが登録済み |

### 3.10 CRE_ALM のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `almatr` が無効（`TA_NULL` 以外） |
| `E_PAR` | `almhdr` が不正な番地 |
| `E_OBJ` | 同名のアラームが登録済み |

### 3.11 CFG_INT のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `intatr` が無効（ターゲット定義の制限を含む） |
| `E_PAR` | `intno` が有効範囲外，`intpri` が `TMIN_INTPRI..TMAX_INTPRI(=-1)` 範囲外 |
| `E_PAR` | ターゲット定義で連動して設定される割込みラインに矛盾する優先度を指定 |
| `E_OBJ` | 同じ割込み番号に対して属性が設定済み（複数の `CFG_INT` 重複） |
| `E_OBJ` | カーネル管理の割込みラインに対し `TMIN_INTPRI` より小さい値を指定（カーネル管理外用） |
| `E_OBJ` | カーネル管理外の割込みラインに対し `TMIN_INTPRI` 以上の値を指定 |

### 3.12 ATT_ISR のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `isratr` が `TA_NULL` 以外 |
| `E_PAR` | `intno` が有効範囲外 |
| `E_PAR` | `isr` が不正な番地（ターゲット定義） |
| `E_PAR` | `isrpri` が `TMIN_ISRPRI(=1)..TMAX_ISRPRI(=16)` 範囲外 |
| `E_OBJ` | 対象割込み要求ラインの属性が `CFG_INT` で設定されていない |
| `E_OBJ` | 対象割込み番号に対応する割込みハンドラ番号に `DEF_INH`/`def_inh` で割込みハンドラが定義済み |
| `E_OBJ` | カーネル管理外の割込み番号に対する登録 |

### 3.13 DEF_INH のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `inhatr` に `TA_NONKERNEL` 以外の無効ビット |
| `E_RSATR` | `TA_NONKERNEL` 指定時にカーネル管理の割込み（優先度がTMIN_INTPRI以上）を対象 |
| `E_RSATR` | `TA_NONKERNEL` 非指定時にカーネル管理外の割込みを対象 |
| `E_PAR` | `inhno` が範囲外 |
| `E_PAR` | `inthdr` が不正 |
| `E_OBJ` | 同じ割込みハンドラ番号に既に定義済み |

### 3.14 DEF_EXC のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `excatr` が `TA_NULL` 以外 |
| `E_PAR` | `excno` が範囲外，`exchdr` が不正 |
| `E_OBJ` | 同じ例外ハンドラ番号に既に定義済み |

### 3.15 DEF_TEX のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `texatr` が `TA_NULL` 以外 |
| `E_PAR` | `texrtn` が不正 |
| `E_NOEXS` | `tskid` で指定したタスクが未登録（記述順を間違えた場合） |
| `E_OBJ` | 既にタスク例外処理ルーチン定義済み |

### 3.16 DEF_ICS のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_PAR` | `istksz` がターゲット定義の最小値未満 |
| `E_PAR` | `istk` がターゲット定義の制約に合致しない |
| `E_NOMEM` | `istk=NULL` 指定でカーネルがスタック領域を確保できない |
| `E_OBJ` | 既に `DEF_ICS` が記述済み（複数指定） |

### 3.17 ATT_INI / ATT_TER のエラー条件

| エラーコード | 条件 |
|---|---|
| `E_RSATR` | `iniatr`/`teratr` が `TA_NULL` 以外 |
| `E_PAR` | `inirtn`/`terrtn` が不正な番地 |

複数の `ATT_INI`/`ATT_TER` を記述してもエラーにならない（複数の初期化ルーチンを登録するため）。

## 4. サービスコール別エラー早見表

### 4.1 タスク管理

| サービスコール | 主なエラー |
|---|---|
| `act_tsk` | `E_CTX`, `E_ID`, `E_QOVR` |
| `iact_tsk` | `E_CTX`, `E_ID`, `E_QOVR` |
| `can_act` | `E_CTX`, `E_ID` |
| `ext_tsk` | （返らない。CPUロック中も呼出可） |
| `ter_tsk` | `E_CTX`, `E_ID`, `E_OBJ`（自タスク指定） |
| `chg_pri` | `E_CTX`, `E_ID`, `E_PAR`, `E_OBJ`, `E_ILUSE` |
| `get_pri` | `E_CTX`, `E_ID`, `E_OBJ` |
| `get_inf` | `E_CTX` |
| `ref_tsk` | `E_CTX`, `E_ID` |

### 4.2 タスク付属同期

| サービスコール | 主なエラー |
|---|---|
| `slp_tsk` | `E_CTX`, `E_RLWAI` |
| `tslp_tsk` | `E_CTX`, `E_PAR`, `E_TMOUT`, `E_RLWAI` |
| `wup_tsk` | `E_CTX`, `E_ID`, `E_OBJ`, `E_QOVR` |
| `iwup_tsk` | `E_CTX`, `E_ID`, `E_OBJ`, `E_QOVR` |
| `can_wup` | `E_CTX`, `E_ID`, `E_OBJ` |
| `rel_wai` | `E_CTX`, `E_ID`, `E_OBJ` |
| `irel_wai` | `E_CTX`, `E_ID`, `E_OBJ` |
| `sus_tsk` | `E_CTX`, `E_ID`, `E_OBJ` |
| `rsm_tsk` | `E_CTX`, `E_ID`, `E_OBJ` |
| `dly_tsk` | `E_CTX`, `E_PAR`, `E_RLWAI` |

### 4.3 同期・通信

| サービスコール | 主なエラー |
|---|---|
| `sig_sem` | `E_CTX`, `E_ID`, `E_QOVR` |
| `wai_sem`/`twai_sem`/`pol_sem` | `E_CTX`, `E_ID`, `E_PAR`(twaiのみ), `E_TMOUT`, `E_RLWAI`, `E_DLT`(ini経由) |
| `set_flg`/`iset_flg` | `E_CTX`, `E_ID`, `E_PAR`(0bit setなど) |
| `clr_flg` | `E_CTX`, `E_ID` |
| `wai_flg`/`twai_flg`/`pol_flg` | `E_CTX`, `E_ID`, `E_PAR`(`wfmode`/`waiptn`), `E_ILUSE`(TA_WMUL違反), `E_TMOUT`, `E_RLWAI`, `E_DLT` |
| `snd_dtq`/`psnd_dtq`/`tsnd_dtq` | `E_CTX`, `E_ID`, `E_PAR`(tmout), `E_TMOUT`, `E_RLWAI`, `E_DLT` |
| `fsnd_dtq`/`ifsnd_dtq` | `E_CTX`, `E_ID` |
| `rcv_dtq`/`prcv_dtq`/`trcv_dtq` | `E_CTX`, `E_ID`, `E_PAR`, `E_TMOUT`, `E_RLWAI`, `E_DLT` |
| `loc_mtx`/`tloc_mtx`/`ploc_mtx` | `E_CTX`, `E_ID`, `E_ILUSE`(再帰/ceiling違反), `E_TMOUT`, `E_RLWAI`, `E_DLT` |
| `unl_mtx` | `E_CTX`, `E_ID`, `E_ILUSE`(順序違反/未ロック) |
| `get_mpf`/`pget_mpf`/`tget_mpf` | `E_CTX`, `E_ID`, `E_PAR`, `E_TMOUT`, `E_RLWAI`, `E_DLT` |
| `rel_mpf` | `E_CTX`, `E_ID`, `E_PAR`(無効ブロック) |

### 4.4 時間管理

| サービスコール | 主なエラー |
|---|---|
| `get_tim` | `E_CTX`, `E_PAR`(NULLポインタ等) |
| `get_utm` | `E_CTX`, `E_NOSPT`(未対応ターゲット) |
| `sta_cyc`/`stp_cyc` | `E_CTX`, `E_ID` |
| `ref_cyc` | `E_CTX`, `E_ID` |
| `sta_alm`/`ista_alm` | `E_CTX`, `E_ID`, `E_PAR`(almtim範囲外) |
| `stp_alm`/`istp_alm` | `E_CTX`, `E_ID` |
| `ref_alm` | `E_CTX`, `E_ID` |

### 4.5 システム状態管理

| サービスコール | 主なエラー |
|---|---|
| `rot_rdq`/`irot_rdq` | `E_CTX`, `E_PAR`(優先度範囲外) |
| `get_tid`/`iget_tid` | `E_CTX`, `E_PAR` |
| `loc_cpu`/`unl_cpu`/`iloc_cpu`/`iunl_cpu` | `E_CTX`(基本的にエラーなし) |
| `dis_dsp`/`ena_dsp` | `E_CTX` |
| `chg_ipm` | `E_CTX`, `E_PAR` |
| `get_ipm` | `E_CTX` |
| `sns_*` | エラー無し（bool_t を返す） |
| `ext_ker` | （返らない） |
| `ref_cfg`/`ref_ver` | `E_CTX`, `E_PAR` |

### 4.6 割込み管理

| サービスコール | 主なエラー |
|---|---|
| `dis_int`/`ena_int` | `E_NOSPT`, `E_PAR`(intno) |
| `chg_ipm`/`get_ipm` | （上記参照） |

### 4.7 CPU例外管理

| サービスコール | 主なエラー |
|---|---|
| `xsns_dpn`/`xsns_xpn` | エラー無し（bool_t を返す） |

## 5. エラー処理パターン（推奨）

### 5.1 アサーション

開発時はサービスコールの返値をアサートで検証する：

```c
ER ercd = wai_sem(SEM_BUTTON);
assert(ercd == E_OK);
```

### 5.2 ロギング

本番では `syslog` 等でロギング：

```c
ER ercd = wai_sem(SEM_BUTTON);
if (ercd < 0) {
    syslog(LOG_ERROR, "wai_sem failed: ercd=%d", ercd);
    /* リカバリ処理または ext_tsk */
}
```

### 5.3 タイムアウト処理

```c
ER ercd = twai_sem(SEM_BUTTON, 1000);   /* 1秒待ち */
switch (ercd) {
    case E_OK:
        /* 正常 */
        break;
    case E_TMOUT:
        /* タイムアウト処理 */
        break;
    case E_RLWAI:
        /* 待ち強制解除（タスク終了要求等） */
        return;
    case E_DLT:
        /* オブジェクト再初期化 */
        break;
    default:
        /* 想定外エラー */
        syslog(LOG_ERROR, "twai_sem unexpected error: %d", ercd);
        break;
}
```

### 5.4 ミューテックスのアンロック忘れ防止

```c
loc_mtx(MTX_RES);
do {
    /* クリティカルセクション */
} while (0);
unl_mtx(MTX_RES);   /* 必ず unl_mtx する */
```

`return` や `ext_tsk()` の前にも忘れずアンロックする。タスク例外処理ルーチンへの遷移にも注意（`ras_tex` でリカバリすると排他状態が中途半端に残る）。

## 6. ASPで発生しないエラーコード（参考）

ASP標準パッケージでは次のエラーは**通常発生しない**。発生したらカーネル不具合・実装外機能の使用を疑う：

- `E_MACV` (-26): 保護機能対応カーネルのみ
- `E_OACV` (-27): 保護機能対応カーネルのみ
- `E_NOID` (-34): 動的生成パッケージのみ
- `E_NORES` (-35): 主に動的生成パッケージ
- `E_WBLK` (-57): システムサービスでノンブロッキング指定時のみ
- `E_BOVR` (-58): メッセージバッファ機能（ASPでは未サポート）

LLMがこれらをエラーハンドラに含めようとした場合，「ASP標準では不要」と注意すること。

## 7. コンフィギュレータがビルド時に検出するエラー

`.cfg` の解析中にコンフィギュレータが検出するエラー（ビルド時失敗）：

| 検出タイミング | 内容 |
|---|---|
| パス1（C言語ファイル生成時） | 文法エラー，未サポートの記述，知らない静的API |
| パス1〜パス2（コンパイル時） | 整数定数式パラメータが解釈できない |
| パス2（kernel_cfg.c生成時） | パラメータ値の整合性チェック，重複ID，範囲外属性 |
| パス2以降（リンク時） | 一般定数式パラメータが解釈できない（関数ポインタの未定義など） |

ビルドエラーには静的APIのファイル名・行番号・原因が出力されるので，それに従って修正する。

LLMが `.cfg` 生成時にコンフィギュレータの検出を意識するなら：

1. オブジェクト識別名はC言語の識別子規則に従う（数字始まりNG）。
2. 整数定数式パラメータは `kernel_cfg.h` 由来のマクロ・`<kernel.h>` の定数・自前ヘッダのマクロのみで構成する。
3. 関数ポインタ・拡張情報は `.cfg` から `#include` した場所で宣言／定義しておく。
4. `CFG_INT` → `ATT_ISR` の順序を守る。
5. 未対応属性（`TA_PHS`, `TA_RSTR` 等）を使わない。
