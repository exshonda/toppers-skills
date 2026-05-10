# TOPPERS/ASP カーネル サービスコール（C言語API）仕様

本書は TOPPERS新世代カーネル統合仕様書 Release 1.7.1（<https://www.toppers.jp/docs/tech/ngki_spec-171.pdf>）のうち TOPPERS/ASPカーネルが標準でサポートするサービスコール（C言語API）を一覧化し，引数・返値・呼出可能コンテキスト・エラーコード・主要な機能を機能別にまとめたものである。動的生成機能拡張パッケージ・オーバランハンドラ拡張パッケージ・制約タスク拡張パッケージのAPIは含めない。FMP/HRP2/HRMP/SSP固有のAPIも含めない。

## 0. 表記法

### 0.1 コンテキスト記号

- `〔T〕` タスクコンテキスト専用。非タスクコンテキストから呼ぶと `E_CTX`。
- `〔I〕` 非タスクコンテキスト専用。タスクコンテキストから呼ぶと `E_CTX`。
- `〔TI〕` 両方から呼出可能。

### 0.2 共通エラーコード

- `E_CTX`: コンテキストエラー（呼出可能コンテキスト違反，CPUロック状態違反，ディスパッチ保留状態違反）
- `E_ID`: 不正ID番号（ID番号が有効範囲外）
- `E_PAR`: パラメータエラー（タイムアウト値・ビットパターン等が無効）
- `E_OBJ`: オブジェクト状態エラー（対象オブジェクトの状態が操作と不整合）
- `E_NOEXS`: オブジェクト未登録（ASPでは静的生成のみのため通常起こらない）
- `E_RLWAI`: 待ち禁止状態または待ち状態の強制解除
- `E_TMOUT`: ポーリング失敗またはタイムアウト
- `E_DLT`: 待ちオブジェクトの削除または再初期化（ASPでは ini_*** で発生）
- `E_QOVR`: キューイングオーバフロー
- `E_NOSPT`: 未サポート機能

詳細は `TOPPERS-ASP_静的API_エラー.md` を参照。

### 0.3 共通データ型

| 型 | 意味 |
|---|---|
| `ER` | エラーコード（符号付き整数, int_t） |
| `ER_ID` | ID番号またはエラーコード |
| `ER_UINT` | 符号無し整数またはエラーコード |
| `ER_BOOL` | 真偽値またはエラーコード |
| `ID` | オブジェクトのID番号 |
| `ATR` | オブジェクト属性（符号無し整数） |
| `STAT` | オブジェクトの状態 |
| `MODE` | サービスコールの動作モード |
| `PRI` | 優先度（符号付き整数。値が小さいほど高優先度） |
| `SIZE` | メモリサイズ（バイト） |
| `TMO` | タイムアウト指定（ms。`TMO_POL`=0, `TMO_FEVR`=-1） |
| `RELTIM` | 相対時間（ms） |
| `SYSTIM` | システム時刻（ms） |
| `SYSUTM` | 性能評価用時刻（μs） |
| `INTNO` | 割込み番号 |
| `INHNO` | 割込みハンドラ番号 |
| `EXCNO` | CPU例外ハンドラ番号 |
| `TEXPTN` | タスク例外要因のビットパターン |
| `FLGPTN` | イベントフラグのビットパターン |
| `bool_t` | 真偽値 |
| `intptr_t` | ポインタ格納可能な符号付き整数 |

## 1. タスク管理機能

### 1.1 タスクの起動・終了

```c
ER ercd  = act_tsk(ID tskid);          /* 〔T〕 タスクを起動（実行できる状態へ） */
ER ercd  = iact_tsk(ID tskid);         /* 〔I〕 同上（非タスクコンテキスト用） */
ER_UINT actcnt = can_act(ID tskid);    /* 〔T〕 起動要求キャンセル。返値はキャンセルした起動要求の数 */
ER ercd  = ext_tsk(void);              /*       自タスクを終了（休止状態へ）。返らない */
ER ercd  = ter_tsk(ID tskid);          /* 〔T〕 他タスクを強制終了（自タスク不可） */
```

`tskid`に `TSK_SELF`(=0) を指定すると（許される範囲のサービスコールで）自タスクを意味する。`act_tsk` の起動要求はキューイングされるが，ASPでは `TMAX_ACTCNT`=1 固定なので2回目以降の起動要求は `E_QOVR`（注: 実装上はキューイング数を超えると `E_QOVR`）。

`ext_tsk` は呼び出すと自タスクが休止状態に遷移する。タスクのメインルーチンが return しても同じ振舞いとなる。

主なエラー: `E_CTX`, `E_ID`, `E_OBJ`（自タスクへの `ter_tsk`），`E_QOVR`（起動要求オーバ）。

### 1.2 タスク優先度

```c
ER ercd = chg_pri(ID tskid, PRI tskpri);    /* 〔T〕 ベース優先度を tskpri に変更 */
ER ercd = get_pri(ID tskid, PRI *p_tskpri); /* 〔T〕 現在優先度を取得 */
```

`tskpri`に `TPRI_INI`(=0) を指定すると起動時優先度に戻す。`tskid` に `TSK_SELF`(=0) で自タスクを対象とする。ミューテックス使用中は現在優先度が変動する場合がある（4.6節参照）。

エラー: `E_CTX`, `E_ID`, `E_PAR`（優先度範囲外），`E_OBJ`（休止状態のタスク）。

### 1.3 拡張情報の参照

```c
ER ercd = get_inf(intptr_t *p_exinf);   /* 〔T〕 自タスクの拡張情報を取得 */
```

タスク生成時に与えた `exinf` を取得する。

### 1.4 タスクの状態参照

```c
ER ercd = ref_tsk(ID tskid, T_RTSK *pk_rtsk);   /* 〔T〕 デバッグ用 */
```

`T_RTSK` には `tskstat`（タスク状態），`tskpri`（現在優先度），`tskbpri`（ベース優先度），`actcnt`（起動要求キューイング数），`wobjid`（待ちオブジェクトID）等が返る。デバッグ目的のみ推奨。

## 2. タスク付属同期機能

### 2.1 タスクの起床と起床要求のキャンセル

```c
ER ercd = slp_tsk(void);                 /* 〔T〕 自タスクを起床待ちに */
ER ercd = tslp_tsk(TMO tmout);           /* 〔T〕 起床待ち（タイムアウト付） */
ER ercd = wup_tsk(ID tskid);             /* 〔T〕 タスクを起床 */
ER ercd = iwup_tsk(ID tskid);            /* 〔I〕 同上 */
ER_UINT wupcnt = can_wup(ID tskid);      /* 〔T〕 起床要求のキャンセル */
```

起床要求はキューイングされる（最大 `TMAX_WUPCNT`）。

エラー: `E_CTX`, `E_ID`, `E_TMOUT`（タイムアウト），`E_RLWAI`（待ち状態強制解除），`E_QOVR`（起床要求オーバ）。

### 2.2 待ちの強制解除

```c
ER ercd = rel_wai(ID tskid);             /* 〔T〕 対象タスクの待ち状態を強制解除 */
ER ercd = irel_wai(ID tskid);            /* 〔I〕 同上 */
```

対象タスクには `E_RLWAI` が返る。

### 2.3 強制待ち（suspended）

```c
ER ercd = sus_tsk(ID tskid);             /* 〔T〕 タスクを強制待ち状態へ */
ER ercd = rsm_tsk(ID tskid);             /* 〔T〕 強制待ちから再開 */
```

二重待ちの実現に用いる。エラー: `E_OBJ`（休止／既に強制待ち中）。

### 2.4 起床なしでの自タスク待ち

```c
ER ercd = dly_tsk(RELTIM dlytim);        /* 〔T〕 dlytim ms の遅延 */
```

タイマ割込みベースの遅延。`dly_tsk(0)` でも次のタイムティックまで待つことに注意。

## 3. タスク例外処理機能

```c
ER ercd  = ras_tex(ID tskid, TEXPTN rasptn);   /* 〔T〕 タスクに例外を要求 */
ER ercd  = iras_tex(ID tskid, TEXPTN rasptn);  /* 〔I〕 同上 */
ER ercd  = dis_tex(void);                      /* 〔T〕 自タスクの例外をマスク */
ER ercd  = ena_tex(void);                      /* 〔T〕 例外マスクを解除 */
bool_t state = sns_tex(void);                  /* 〔TI〕 例外マスク状態を参照 */
ER ercd  = ref_tex(ID tskid, T_RTEX *pk_rtex); /* 〔T〕 状態参照（デバッグ用） */
```

タスク例外処理ルーチンはタスク毎に1つ `DEF_TEX` で登録。`rasptn` のビットパターンで要因を伝達。

## 4. 同期・通信機能

### 4.1 セマフォ

```c
ER ercd = sig_sem(ID semid);                       /* 〔T〕  資源の返却 */
ER ercd = isig_sem(ID semid);                      /* 〔I〕  同上 */
ER ercd = wai_sem(ID semid);                       /* 〔T〕  資源の獲得（永久待ち） */
ER ercd = pol_sem(ID semid);                       /* 〔T〕  資源の獲得（ポーリング） */
ER ercd = twai_sem(ID semid, TMO tmout);           /* 〔T〕  資源の獲得（タイムアウト付） */
ER ercd = ini_sem(ID semid);                       /* 〔T〕  セマフォの再初期化 */
ER ercd = ref_sem(ID semid, T_RSEM *pk_rsem);      /* 〔T〕  状態参照（デバッグ用） */
```

セマフォは0以上の整数カウンタ。`sig_sem` で待ち行列にタスクがあれば先頭タスクを起床（資源数は変化しない）し，そうでなければカウンタを+1。最大資源数を超えると `E_QOVR`。

`wai_sem` 系: カウンタ>0 ならカウンタを-1，カウンタ=0 なら待ち行列に入る。

`ini_sem` 後はカウンタが初期資源数に戻り，待ち中のタスクは `E_DLT` で起こされる。

### 4.2 イベントフラグ

```c
ER ercd = set_flg(ID flgid, FLGPTN setptn);                              /* 〔T〕  ビットを 1 に */
ER ercd = iset_flg(ID flgid, FLGPTN setptn);                             /* 〔I〕  同上 */
ER ercd = clr_flg(ID flgid, FLGPTN clrptn);                              /* 〔T〕  ビットを 0 に（clrptn のビットだけ残す） */
ER ercd = wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
ER ercd = pol_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn);
ER ercd = twai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout);
ER ercd = ini_flg(ID flgid);
ER ercd = ref_flg(ID flgid, T_RFLG *pk_rflg);
```

`wfmode`:
- `TWF_ORW`(=0x01): `waiptn` 内のいずれかのビットがセットされるのを待つ
- `TWF_ANDW`(=0x02): `waiptn` 内のすべてのビットがセットされるのを待つ

属性:
- `TA_TPRI`(=0x01): 待ち行列をタスク優先度順に（指定しない場合はFIFO順）
- `TA_WMUL`(=0x02): 複数タスクの待ちを許可（指定しないと2人目以降は `E_ILUSE`）
- `TA_CLR`(=0x04): 待ち解除時にビットパターンをクリア

`p_flgptn` には待ち解除時のビットパターンを返す。

### 4.3 データキュー

```c
ER ercd = snd_dtq(ID dtqid, intptr_t data);                       /* 〔T〕  送信（永久待ち） */
ER ercd = psnd_dtq(ID dtqid, intptr_t data);                      /* 〔T〕  送信（ポーリング） */
ER ercd = ipsnd_dtq(ID dtqid, intptr_t data);                     /* 〔I〕  同上 */
ER ercd = tsnd_dtq(ID dtqid, intptr_t data, TMO tmout);           /* 〔T〕  送信（タイムアウト） */
ER ercd = fsnd_dtq(ID dtqid, intptr_t data);                      /* 〔T〕  強制送信（最古を上書き） */
ER ercd = ifsnd_dtq(ID dtqid, intptr_t data);                     /* 〔I〕  同上 */
ER ercd = rcv_dtq(ID dtqid, intptr_t *p_data);                    /* 〔T〕  受信（永久待ち） */
ER ercd = prcv_dtq(ID dtqid, intptr_t *p_data);                   /* 〔T〕  受信（ポーリング） */
ER ercd = trcv_dtq(ID dtqid, intptr_t *p_data, TMO tmout);        /* 〔T〕  受信（タイムアウト） */
ER ercd = ini_dtq(ID dtqid);
ER ercd = ref_dtq(ID dtqid, T_RDTQ *pk_rdtq);
```

データキューは1要素 `intptr_t`（4バイトもしくは8バイト）の固定数キュー。容量0でも生成可（ランデブー的に使う）。`fsnd_dtq` はキュー満杯時に最古要素を上書きしてエラーにならない。

### 4.4 優先度データキュー

```c
ER ercd = snd_pdq(ID pdqid, intptr_t data, PRI datapri);                              /* 〔T〕 */
ER ercd = psnd_pdq(ID pdqid, intptr_t data, PRI datapri);                             /* 〔T〕 */
ER ercd = ipsnd_pdq(ID pdqid, intptr_t data, PRI datapri);                            /* 〔I〕 */
ER ercd = tsnd_pdq(ID pdqid, intptr_t data, PRI datapri, TMO tmout);                  /* 〔T〕 */
ER ercd = rcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri);                        /* 〔T〕 */
ER ercd = prcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri);                       /* 〔T〕 */
ER ercd = trcv_pdq(ID pdqid, intptr_t *p_data, PRI *p_datapri, TMO tmout);            /* 〔T〕 */
ER ercd = ini_pdq(ID pdqid);
ER ercd = ref_pdq(ID pdqid, T_RPDQ *pk_rpdq);
```

データに優先度（`datapri`）を付けて送信。受信側は最も高優先度のデータを取り出す。

### 4.5 メールボックス

```c
ER ercd = snd_mbx(ID mbxid, T_MSG *pk_msg);                  /* 〔T〕 */
ER ercd = rcv_mbx(ID mbxid, T_MSG **ppk_msg);                /* 〔T〕 */
ER ercd = prcv_mbx(ID mbxid, T_MSG **ppk_msg);               /* 〔T〕 */
ER ercd = trcv_mbx(ID mbxid, T_MSG **ppk_msg, TMO tmout);    /* 〔T〕 */
ER ercd = ini_mbx(ID mbxid);
ER ercd = ref_mbx(ID mbxid, T_RMBX *pk_rmbx);
```

メッセージはアプリケーションが確保したメモリ領域へのポインタ。`T_MSG` 構造体（または `T_MSG_PRI`）を先頭に持つ任意の型を指す。

属性 `TA_MPRI`(=0x02) を指定するとメッセージキューがメッセージ優先度順になる（指定しない場合はFIFO順）。優先度順の場合，メッセージは `T_MSG_PRI` で始まる必要がある（先頭フィールドに `msgpri` が含まれる）。受信待ちタスクの待ち行列順序は `TA_TPRI` で指定（指定しない場合はFIFO順）。

### 4.6 ミューテックス

```c
ER ercd = loc_mtx(ID mtxid);                                 /* 〔T〕  ロック（永久待ち） */
ER ercd = ploc_mtx(ID mtxid);                                /* 〔T〕  ロック（ポーリング） */
ER ercd = tloc_mtx(ID mtxid, TMO tmout);                     /* 〔T〕  ロック（タイムアウト） */
ER ercd = unl_mtx(ID mtxid);                                 /* 〔T〕  アンロック */
ER ercd = ini_mtx(ID mtxid);
ER ercd = ref_mtx(ID mtxid, T_RMTX *pk_rmtx);
```

属性:
- `TA_TPRI`(=0x01): 待ち行列をタスク優先度順に（指定しない場合はFIFO順）
- `TA_CEILING`(=0x03): 優先度上限プロトコル（priority ceiling）

`TA_CEILING` 属性のミューテックスでは生成時に上限優先度（ceilpri）を指定し，ロックしているタスクの現在優先度がそこまで引き上げられる（PIプロトコル/PCプロトコルの実装はターゲット依存）。

### 4.7 メールボックス vs ミューテックスの選択

排他制御目的なら **ミューテックスを推奨**（優先度逆転を回避できる）。データ通知が目的ならセマフォ・データキュー・イベントフラグから用途に合わせて選択する。

## 5. メモリプール管理機能

### 5.1 固定長メモリプール

```c
ER ercd = get_mpf(ID mpfid, void **p_blk);                   /* 〔T〕  ブロック獲得（永久待ち） */
ER ercd = pget_mpf(ID mpfid, void **p_blk);                  /* 〔T〕  ブロック獲得（ポーリング） */
ER ercd = tget_mpf(ID mpfid, void **p_blk, TMO tmout);       /* 〔T〕  ブロック獲得（タイムアウト） */
ER ercd = rel_mpf(ID mpfid, void *blk);                      /* 〔T〕  ブロック返却 */
ER ercd = ini_mpf(ID mpfid);
ER ercd = ref_mpf(ID mpfid, T_RMPF *pk_rmpf);
```

固定サイズのブロックを管理する単純なメモリアロケータ。malloc/free よりリアルタイム性が高い。

属性: `TA_TPRI` で待ち行列をタスク優先度順に（指定しない場合はFIFO順）。

ASPはこれ以外のメモリ管理（malloc等）を提供しない。可変長メモリプール機能は本仕様にも含まれていない。

## 6. 時間管理機能

### 6.1 システム時刻管理

```c
ER ercd = get_tim(SYSTIM *p_systim);     /* 〔T〕  システム時刻 [ms] */
ER ercd = get_utm(SYSUTM *p_sysutm);     /* 〔TI〕 性能評価用時刻 [μs]（ターゲット依存） */
```

`get_utm` は性能測定向けの高分解能時刻（μs）。ハードウェアタイマが必要でターゲット依存。

### 6.2 周期ハンドラ

```c
ER ercd = sta_cyc(ID cycid);                            /* 〔T〕 動作開始（次回起動時刻を再設定） */
ER ercd = stp_cyc(ID cycid);                            /* 〔T〕 動作停止 */
ER ercd = ref_cyc(ID cycid, T_RCYC *pk_rcyc);           /* 〔T〕 状態参照 */
```

ASPでは生成時に `TA_STA` を指定すれば自動で動作開始。`TA_PHS` 属性は**ASPでサポートしない**ことに注意（生成時刻基準にしたい場合は使えない）。

周期ハンドラの記述形式:
```c
void cyclic_handler(intptr_t exinf)
{
    /* 非タスクコンテキストで実行される */
    /* タスクへの通知は iset_flg, isig_sem, ipsnd_dtq, iact_tsk 等を用いる */
}
```

### 6.3 アラームハンドラ

```c
ER ercd = sta_alm(ID almid, RELTIM almtim);      /* 〔T〕 almtim 後に起動するよう設定 */
ER ercd = ista_alm(ID almid, RELTIM almtim);     /* 〔I〕 同上 */
ER ercd = stp_alm(ID almid);                     /* 〔T〕 停止 */
ER ercd = istp_alm(ID almid);                    /* 〔I〕 同上 */
ER ercd = ref_alm(ID almid, T_RALM *pk_ralm);    /* 〔T〕 状態参照 */
```

アラームハンドラは1回限りの起動。動作中に再度 `sta_alm` を呼び出すと起動時刻が再設定される。記述形式は周期ハンドラと同じ。

## 7. システム状態管理機能

### 7.1 タスク優先順位の回転

```c
ER ercd = rot_rdq(PRI tskpri);    /* 〔T〕  指定優先度のFCFSタスクを回転 */
ER ercd = irot_rdq(PRI tskpri);   /* 〔I〕  同上 */
```

`tskpri` に `TPRI_SELF`(=0) を指定すると自タスクのベース優先度が対象。タイムスライス的なスケジューリングを実現する用途。

### 7.2 実行中タスクのID取得

```c
ER ercd = get_tid(ID *p_tskid);    /* 〔T〕  自タスクID（タスクなら自分） */
ER ercd = iget_tid(ID *p_tskid);   /* 〔I〕  実行中タスクID（割込み中なら割込まれたタスク） */
```

### 7.3 CPUロック

```c
ER ercd = loc_cpu(void);     /* 〔T〕  CPUロック状態へ */
ER ercd = iloc_cpu(void);    /* 〔I〕  同上 */
ER ercd = unl_cpu(void);     /* 〔T〕  CPUロック解除 */
ER ercd = iunl_cpu(void);    /* 〔I〕  同上 */
```

CPUロック中はカーネル管理割込みがマスクされ，ディスパッチが保留される。これらは入れ子で呼び出してはならない。CPUロック中に呼べるサービスコールは `loc_cpu/unl_cpu`, `iloc_cpu/iunl_cpu`, `dis_int/ena_int`, `sns_***`, `get_utm`, `ext_tsk`, `ext_ker` のみ。

### 7.4 ディスパッチ禁止／許可

```c
ER ercd = dis_dsp(void);    /* 〔T〕  ディスパッチ禁止 */
ER ercd = ena_dsp(void);    /* 〔T〕  ディスパッチ許可 */
```

タスク間の排他に使えるが，ミューテックスの使用が望ましい。`dis_dsp` 中はタスクが切り替わらないが割込みは入る。

### 7.5 状態参照（センサ系API）

```c
bool_t state = sns_ctx(void);     /* 〔TI〕 非タスクコンテキスト中なら true */
bool_t state = sns_loc(void);     /* 〔TI〕 CPUロック中なら true */
bool_t state = sns_dsp(void);     /* 〔TI〕 ディスパッチ禁止中なら true */
bool_t state = sns_dpn(void);     /* 〔TI〕 ディスパッチ保留状態なら true */
bool_t state = sns_ker(void);     /* 〔TI〕 カーネル非動作状態なら true */
```

### 7.6 カーネル終了

```c
ER ercd = ext_ker(void);    /* どのコンテキストからでも呼べる。返らない */
```

呼び出すとシステム全体の終了処理が走り，ターゲット定義の終了処理に移行する。

### 7.7 カーネル構成情報の参照

```c
ER ercd = ref_cfg(T_RCFG *pk_rcfg);    /* 〔T〕 構成情報（ターゲット依存） */
ER ercd = ref_ver(T_RVER *pk_rver);    /* 〔T〕 バージョン情報（maker/prid/spver/prver/prno） */
```

`T_RVER` には ITRON仕様準拠系の値（maker, prid, spver, prver, prno[4]）が返る。

## 8. 割込み管理機能

### 8.1 割込み要求ライン制御

```c
ER ercd = dis_int(INTNO intno);                /* 〔T〕  割込み要求禁止フラグをセット */
ER ercd = ena_int(INTNO intno);                /* 〔T〕  同フラグをクリア */
ER ercd = ref_int(INTNO intno, T_RINT *p_rint); /* 〔T〕 ターゲット依存（仕様で必須でない） */
```

`dis_int`/`ena_int` がサポートされるかは構成マクロ `TOPPERS_SUPPORT_DIS_INT` / `TOPPERS_SUPPORT_ENA_INT` で判定可能（ターゲット依存）。

### 8.2 割込み優先度マスク

```c
ER ercd = chg_ipm(PRI intpri);     /* 〔T〕  割込み優先度マスクの設定 */
ER ercd = get_ipm(PRI *p_intpri);  /* 〔T〕  現在のマスク値を取得 */
```

`intpri` には `TIPM_ENAALL`(=0) または `TMIN_INTPRI`〜-1 の値を指定。CPUロック中は呼べない。

### 8.3 割込みサービスルーチンの参照

ASPでは `CRE_ISR` の代わりに `ATT_ISR` で登録（ID無し）。動的サービスコール `acre_isr`, `del_isr`, `ref_isr` は動的生成パッケージのみ。

## 9. CPU例外管理機能

```c
bool_t stat = xsns_dpn(void *p_excinf);    /* 〔TI〕 CPU例外発生時にディスパッチ可能だったか */
bool_t stat = xsns_xpn(void *p_excinf);    /* 〔TI〕 CPU例外発生時にタスク例外処理可能だったか */
```

CPU例外ハンドラからリカバリ方針を判断するために使う。`p_excinf` はCPU例外ハンドラに渡されたCPU例外発生時情報へのポインタ。

CPU例外ハンドラの記述形式:
```c
void cpu_exception_handler(void *p_excinf)
{
    if (xsns_dpn(p_excinf) == false) {
        /* タスクへリカバリ依頼可能（act_tsk, wup_tsk, sig_sem 等） */
    } else if (xsns_xpn(p_excinf) == false) {
        /* CPU例外を起こしたタスクにタスク例外処理を要求 */
        ID tskid;
        iget_tid(&tskid);
        iras_tex(tskid, RECOVERY_PATTERN);
    } else {
        /* (a) ハードウェア独立な復旧 もしくは (d) システム再起動 */
    }
}
```

## 10. 呼出可能性まとめ表

下表は，主要なシステム状態における代表的なサービスコールの呼出可否を示す。

| サービスコール | タスクコンテキスト（通常状態） | 非タスクコンテキスト | CPUロック中 | ディスパッチ禁止中 | 割込み優先度マスクが全解除でない |
|---|---|---|---|---|---|
| `act_tsk` | OK | × | E_CTX | OK | OK |
| `iact_tsk` | × | OK | OK | — | — |
| `wai_sem` | OK | × | E_CTX | E_CTX | E_CTX |
| `pol_sem` | OK | × | E_CTX | OK | OK |
| `sig_sem` | OK | × | E_CTX | OK | OK |
| `isig_sem` | × | OK | OK | — | — |
| `loc_cpu` | OK | × | OK（無効） | OK | OK |
| `iloc_cpu` | × | OK | OK（無効） | — | — |
| `unl_cpu` | OK | × | OK | OK | OK |
| `dis_dsp` | OK | × | E_CTX | OK | OK |
| `chg_ipm` | OK | × | E_CTX | OK | OK |
| `dis_int`/`ena_int` | OK | × | OK | OK | OK |
| `sns_*` | OK | OK | OK | OK | OK |
| `ext_tsk` | OK | × | OK | OK | OK |
| `ext_ker` | OK | OK | OK | OK | OK |

ルール: 非タスクコンテキスト用の `i` 接頭辞付きAPIをタスクコンテキストから呼ぶ／タスクコンテキスト用APIを非タスクコンテキストから呼ぶと `E_CTX`。原則として広義の待ちに入る可能性のあるAPIはディスパッチ保留状態（CPUロック・優先度マスク・dis_dsp・非タスクコンテキスト）から呼べない。

## 11. 共通定数・マクロ

| 名前 | 値 | 意味 |
|---|---|---|
| `E_OK` | 0 | 正常終了 |
| `TRUE` / `FALSE` | 1 / 0 | 真偽値 |
| `NULL` | 0 | ヌルポインタ |
| `TA_NULL` | 0 | 属性なし |
| `TPRI_SELF` | 0 | 自タスクのベース優先度 |
| `TPRI_INI` | 0 | 起動時優先度 |
| `TSK_SELF` | 0 | 自タスク |
| `TSK_NONE` | 0 | タスクが存在しない |
| `TIPM_ENAALL` | 0 | 割込みマスクなし |
| `TMO_POL` | 0 | ポーリング |
| `TMO_FEVR` | -1 | 永久待ち |
| `TWF_ORW` | 0x01 | OR待ち |
| `TWF_ANDW` | 0x02 | AND待ち |
| `TIM_ZERO` | 0 | システム時刻ゼロ |

機能コードを構成するマクロ・エラーコードを構成するマクロ：

```c
ER  ercd = ERCD(MERCD mercd, SERCD sercd);   /* メイン+サブからエラーコード作成 */
ER  mercd = MERCD(ER ercd);                  /* メインエラーコード抽出 */
ER  sercd = SERCD(ER ercd);                  /* サブエラーコード抽出（ASPでは常に-1） */
```

## 12. 推奨スタイル

LLMがアプリケーションコードを生成する際の指針：

1. すべてのAPI呼出は返値を `ER`/`ER_ID`/`ER_UINT` で受け，少なくとも開発時には `assert(ercd >= 0)` 相当のチェックを行う。
2. 非タスクコンテキストから呼ぶときは必ず `i` 接頭辞付きAPIを使う。`act_tsk`/`iact_tsk`, `set_flg`/`iset_flg`, `sig_sem`/`isig_sem`, `psnd_dtq`/`ipsnd_dtq` 等。
3. 割込みサービスルーチン・周期ハンドラ・アラームハンドラの内部では，**待ちに入るサービスコールを呼ばない**（`wai_sem`, `tslp_tsk`, `loc_mtx` 等は禁止）。タスクへの通知はセマフォ・イベントフラグ・データキューで行い，処理本体はタスクで実行する。
4. タスクのメインルーチン末尾には `ext_tsk()` を明示的に書く（return でもよいがコード意図を明確にするため）。
5. CPUロック状態と割込み優先度マスクの設定は最小限にし，常に解除を伴う。`loc_cpu`/`unl_cpu` をペアにする。
6. ミューテックスをロックしたタスクが終了する前に必ず `unl_mtx` する。
7. `chg_pri` で動的に優先度を変える設計は避け，必要なら起動時優先度の調整やミューテックスの優先度上限プロトコルを使う。
8. デバッグ用の `ref_*` API は本番コードに残さない（時刻ずれの原因や副作用なしの参照ができないため）。
9. `dly_tsk(0)` ではなく `dly_tsk(1)` を使う（仕様上少なくとも1ティック以上経過してから処理が再開される）。
10. `chg_ipm(0)` で割込み優先度マスクを解除する場合，呼出元のコンテキストが許す状態のみで使う（処理単位の実行開始条件と整合）。

## 13. 範囲外API（参考）

LLMが TOPPERS/ASP 用にコードを生成するときに **使ってはならない** APIのリスト（参考）：

- メモリオブジェクト管理: `att_mem`, `att_pma`, `det_mem`, `prb_mem`, `ref_mem`
- 拡張サービスコール: `def_svc`, `cal_svc`
- スピンロック: `loc_spn`, `iloc_spn`, `try_spn`, `itry_spn`, `unl_spn`, `iunl_spn`, `ref_spn`
- マルチプロセッサ: `mact_tsk`, `imact_tsk`, `mig_tsk`, `msta_cyc`, `msta_alm`, `imsta_alm`, `mrot_rdq`, `imrot_rdq`, `get_pid`, `iget_pid`
- アクセス許可ベクタ系: `sac_tsk`, `sac_sem`, `sac_flg`, `sac_dtq`, `sac_pdq`, `sac_mtx`, `sac_mbf`, `sac_mpf`, `sac_cyc`, `sac_alm`, `sac_isr`, `sac_sys`, `sac_mem`
- メッセージバッファ: `snd_mbf`, `psnd_mbf`, `tsnd_mbf`, `rcv_mbf`, `prcv_mbf`, `trcv_mbf`, `ini_mbf`, `ref_mbf`
- 動的生成系: `acre_*`（acre_tsk, acre_sem, ...）, `del_*`（del_tsk, del_sem, ...）
- オーバランハンドラ: `def_ovr`, `sta_ovr`, `ista_ovr`, `stp_ovr`, `istp_ovr`, `ref_ovr`
- 待ち禁止状態（保護機能対応カーネル限定）: `dis_wai`, `idis_wai`, `ena_wai`, `iena_wai`
- 保護ドメインID取得: `get_did`

これらの記述があった場合，LLMは「ASPの標準パッケージでは利用不可」と注意し，**サポートされる代替APIを提示**すべきである。例えば `acre_tsk` を求められたら `CRE_TSK` を使った静的記述を提案する。
