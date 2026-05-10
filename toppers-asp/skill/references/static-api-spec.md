# TOPPERS/ASP カーネル 静的API仕様

本書は TOPPERS新世代カーネル統合仕様書 Release 1.7.1（<https://www.toppers.jp/docs/tech/ngki_spec-171.pdf>）のうち TOPPERS/ASPカーネル が標準でサポートする**静的API**（システムコンフィギュレーションファイル `.cfg` に記述するインタフェース）をまとめたものである。動的生成パッケージ・拡張パッケージ・FMP/HRP2/HRMP/SSP固有APIは含まない。

LLMがTOPPERS/ASP向けの `.cfg` を生成・編集する際の唯一の静的API辞書として参照されることを想定する。エラーコードの詳細は `error-codes.md` を参照。

## 0. システムコンフィギュレーションファイルの基本

### 0.1 ファイル形式

- 拡張子は `.cfg`。テキストファイル。
- 1行に1つの静的APIを書くのが慣例（C言語の関数呼出しと同じ文法）。
- 静的APIの末尾には必ず `;` を付ける。
- 構造体パラメータは `{ ... }` で囲む。

### 0.2 記述できる要素

```c
/* インクルードディレクティブ（C言語プリプロセッサ） */
#include <kernel.h>
#include "myapp.h"

/* コンフィギュレータ向けインクルード */
INCLUDE("\"sample1.h\"");

/* 条件ディレクティブ */
#ifdef ENABLE_FEATURE_X
CRE_SEM(SEM_X, { TA_NULL, 0, 1 });
#endif

/* 静的API */
CRE_TSK(MAIN_TASK, { TA_ACT, 0, main_task, MAIN_PRIORITY, STACK_SIZE, NULL });
ATT_INI({ TA_NULL, 0, init_routine });
```

ASPでは保護ドメインの囲み（`DOMAIN`, `KERNEL_DOMAIN`）・クラスの囲み（`CLASS`）は **使用しない**。

### 0.3 パラメータ種別

| 種別 | 説明 | 記述可能な式 |
|---|---|---|
| (a) オブジェクト識別名 | ID指定パラメータ | 単一の識別名（数字・整数定数式は不可） |
| (b) 整数定数式パラメータ | 属性・サイズ・優先度等 | プリプロセッサ展開後にCコンパイラで評価できる整数定数式 |
| (c) 一般定数式パラメータ | 関数ポインタ・拡張情報・先頭番地等 | 番地に依存してよい任意の定数式 |
| (d) 文字列パラメータ | セクション名等 | C言語の文字列リテラル |

### 0.4 オブジェクト識別名

`CRE_*` 静的APIの第1引数に書いた識別子は，コンフィギュレータがID番号を割り付けて `kernel_cfg.h` に `#define` として出力する。アプリケーションコードからはこのマクロ名を使ってサービスコールを呼ぶ。

```c
/* my_app.cfg */
CRE_SEM(SEM_BUTTON, { TA_NULL, 0, 1 });

/* my_app.c */
#include "kernel_cfg.h"   /* SEM_BUTTON が #define されている */
wai_sem(SEM_BUTTON);
```

### 0.5 廃止された属性（μITRON互換用に注意）

ASP仕様では，**値が0のオブジェクト属性は廃止**され「指定しない（=デフォルト動作）」が正しい記述方法となった。以下の属性は **ASP本体仕様からは廃止** されており，μITRON互換のために `itron.h`（明示的にインクルードした場合のみ）に定義が残っているだけである。**ASPアプリケーションでは使用しない**：

| 廃止された属性 | μITRONでの意味 | ASPでの正しい記述 |
|---|---|---|
| `TA_TFIFO`(=0x00) | 待ち行列をFIFO順に | 何も指定しない（属性が他にないなら `TA_NULL`） |
| `TA_MFIFO`(=0x00) | メールボックスのメッセージキューをFIFO順に | 何も指定しない |
| `TA_HLNG`(=0x00) | 高級言語用インタフェース | 何も指定しない |
| `TA_WSGL`(=0x00) | 待ちタスクは1つのみ（複数待ちは `TA_WMUL`） | 何も指定しない（複数待ちは `TA_WMUL` を明示） |

例（誤）:
```c
CRE_SEM(SEM1, { TA_TFIFO, 0, 1 });             /* TA_TFIFOは廃止 */
CRE_FLG(FLG1, { TA_TFIFO | TA_WMUL, 0 });      /* TA_TFIFOは廃止 */
CRE_MBX(MBX1, { TA_TFIFO | TA_MFIFO, 0, NULL });/* どちらも廃止 */
```

例（正）:
```c
CRE_SEM(SEM1, { TA_NULL, 0, 1 });              /* 待ち行列FIFO順がデフォルト */
CRE_FLG(FLG1, { TA_WMUL, 0 });                 /* 複数待ちのみ明示 */
CRE_MBX(MBX1, { TA_NULL, 0, NULL });           /* タスク待ちFIFO順・メッセージFIFO順がデフォルト */
```

なお `TA_TPRI`(=0x01) など値が0以外の属性は引き続き使用する。「FIFO順 vs 優先度順」は `TA_TPRI` を指定するかしないかで区別する。

## 1. タスク管理機能

### 1.1 CRE_TSK — タスクの生成

```c
CRE_TSK(ID tskid, { ATR tskatr, intptr_t exinf, TASK task,
                    PRI itskpri, SIZE stksz, STK_T *stk });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `tskid` | (a) | タスクのID番号（識別名） |
| `tskatr` | (b) | タスク属性（ビット和） |
| `exinf` | (c) | 拡張情報（タスクのメインに渡される） |
| `task` | (c) | タスクのメインルーチン（関数ポインタ） |
| `itskpri` | (b) | 起動時優先度（1〜TMAX_TPRI） |
| `stksz` | (b) | スタックサイズ（バイト） |
| `stk` | (c) | スタック領域の先頭番地。`NULL` ならカーネルが確保 |

**タスク属性**（`tskatr`）:
- `TA_NULL`(=0): 既定（生成直後は休止状態）
- `TA_ACT`(=0x02): 生成と同時に起動（生成直後は実行できる状態）
- `TA_FPU`: ターゲット定義（FPUレジスタをコンテキストに含める）

ASPでは制約タスク属性 `TA_RSTR`(=0x04) は標準パッケージでは使えない（制約タスク拡張パッケージが必要）。

例：
```c
CRE_TSK(MAIN_TASK,
        { TA_ACT,
          (intptr_t)0,
          main_task,
          MAIN_PRIORITY,
          STACK_SIZE,
          NULL });
```

### 1.2 DEF_TEX — タスク例外処理ルーチンの定義

```c
DEF_TEX(ID tskid, { ATR texatr, TEXRTN texrtn });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `tskid` | (a) | 対象タスクのID番号 |
| `texatr` | (b) | タスク例外処理ルーチン属性（`TA_NULL` のみ） |
| `texrtn` | (c) | タスク例外処理ルーチンの先頭番地 |

タスク例外処理ルーチン記述形式:
```c
void task_exception_handler(TEXPTN texptn, intptr_t exinf)
{
    /* texptn には ras_tex で渡されたビットパターンが入る */
    /* exinf にはタスクの拡張情報が渡される */
}
```

タスクの例外マスクは `ena_tex`/`dis_tex` で操作。タスク例外処理ルーチンが定義されていない場合に `ras_tex` を呼ぶと `E_OBJ` エラーとなる。

## 2. 同期・通信機能

### 2.1 CRE_SEM — セマフォの生成

```c
CRE_SEM(ID semid, { ATR sematr, uint_t isemcnt, uint_t maxsem });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `semid` | (a) | セマフォのID番号 |
| `sematr` | (b) | セマフォ属性 |
| `isemcnt` | (b) | 初期資源数（0以上 maxsem 以下） |
| `maxsem` | (b) | 最大資源数（1以上 `TMAX_MAXSEM`=`UINT_MAX` 以下） |

**セマフォ属性**:
- `TA_NULL`(=0): 何も指定しない（待ち行列はデフォルトでFIFO順）
- `TA_TPRI`(=0x01): 待ち行列をタスク優先度順

例（バイナリセマフォ）:
```c
CRE_SEM(SEM_BUTTON, { TA_NULL, 0, 1 });
```

例（カウンティングセマフォ）:
```c
CRE_SEM(SEM_BUFFER, { TA_TPRI, 4, 4 });
```

### 2.2 CRE_FLG — イベントフラグの生成

```c
CRE_FLG(ID flgid, { ATR flgatr, FLGPTN iflgptn });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `flgid` | (a) | イベントフラグのID番号 |
| `flgatr` | (b) | イベントフラグ属性 |
| `iflgptn` | (b) | 初期ビットパターン |

**イベントフラグ属性**:
- `TA_TPRI`(=0x01): 待ち行列をタスク優先度順に（指定しない場合はFIFO順）
- `TA_WMUL`(=0x02): 複数タスクが同時に待つことを許可
- `TA_CLR`(=0x04): 待ち解除時にビットパターン全クリア

例：
```c
CRE_FLG(FLG_EVENT, { TA_WMUL, 0 });
```

### 2.3 CRE_DTQ — データキューの生成

```c
CRE_DTQ(ID dtqid, { ATR dtqatr, uint_t dtqcnt, void *dtqmb });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `dtqid` | (a) | データキューのID番号 |
| `dtqatr` | (b) | データキュー属性 |
| `dtqcnt` | (b) | データキュー容量（要素数。0でも可） |
| `dtqmb` | (c) | データキュー管理領域（`NULL` ならカーネルが確保） |

**データキュー属性**:
- `TA_TPRI`(=0x01): 送信側タスクの待ち行列をタスク優先度順に（指定しない場合はFIFO順）

例：
```c
CRE_DTQ(DTQ_LOG, { TA_NULL, 16, NULL });
```

### 2.4 CRE_PDQ — 優先度データキューの生成

```c
CRE_PDQ(ID pdqid, { ATR pdqatr, uint_t pdqcnt,
                    PRI maxdpri, void *pdqmb });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `pdqid` | (a) | 優先度データキューのID番号 |
| `pdqatr` | (b) | 優先度データキュー属性 |
| `pdqcnt` | (b) | データキュー容量（要素数） |
| `maxdpri` | (b) | データ優先度の最大値（1〜TMAX_DPRI） |
| `pdqmb` | (c) | 管理領域 |

例：
```c
CRE_PDQ(PDQ_MSG, { TA_NULL, 8, 3, NULL });
```

### 2.5 CRE_MBX — メールボックスの生成

```c
CRE_MBX(ID mbxid, { ATR mbxatr, PRI maxmpri, void *mprihd });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `mbxid` | (a) | メールボックスのID番号 |
| `mbxatr` | (b) | メールボックス属性 |
| `maxmpri` | (b) | メッセージ優先度の最大値（`TA_MPRI` 属性のとき有効） |
| `mprihd` | (c) | 優先度ヘッダ領域（通常は `NULL`） |

**メールボックス属性**:
- `TA_TPRI`(=0x01): 受信待ちタスクの待ち行列をタスク優先度順に（指定しない場合はFIFO順）
- `TA_MPRI`(=0x02): メッセージキューはメッセージ優先度順（指定しない場合はFIFO順）

例：
```c
CRE_MBX(MBX_CMD, { TA_NULL, 0, NULL });
```

ASP標準ではメールボックス機能はサポートされる（HRP2では削除されている）。

### 2.6 CRE_MTX — ミューテックスの生成

```c
CRE_MTX(ID mtxid, { ATR mtxatr, PRI ceilpri });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `mtxid` | (a) | ミューテックスのID番号 |
| `mtxatr` | (b) | ミューテックス属性 |
| `ceilpri` | (b) | 上限優先度（`TA_CEILING` のときのみ有効） |

**ミューテックス属性**:
- `TA_TPRI`(=0x01): 待ち行列をタスク優先度順に（指定しない場合はFIFO順）
- `TA_CEILING`(=0x03): 優先度上限プロトコル

`TA_CEILING` のとき，ロックしているタスクの優先度が `ceilpri` まで引き上げられる（優先度逆転を抑止）。

例：
```c
CRE_MTX(MTX_RESOURCE, { TA_CEILING, MTX_CEIL_PRI });
```

## 3. メモリプール管理機能

### 3.1 CRE_MPF — 固定長メモリプールの生成

```c
CRE_MPF(ID mpfid, { ATR mpfatr, uint_t blkcnt, uint_t blksz,
                    MPF_T *mpf, void *mpfmb });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `mpfid` | (a) | メモリプールID |
| `mpfatr` | (b) | メモリプール属性 |
| `blkcnt` | (b) | ブロック数 |
| `blksz` | (b) | ブロックサイズ（バイト） |
| `mpf` | (c) | メモリプール領域（`NULL` ならカーネルが確保） |
| `mpfmb` | (c) | 管理領域（`NULL` ならカーネルが確保） |

**メモリプール属性**: `TA_TPRI`(=0x01) で待ち行列をタスク優先度順に（指定しない場合はFIFO順）

例：
```c
CRE_MPF(MPF_PACKET, { TA_NULL, 32, sizeof(packet_t), NULL, NULL });
```

## 4. 時間管理機能

### 4.1 CRE_CYC — 周期ハンドラの生成

```c
CRE_CYC(ID cycid, { ATR cycatr, intptr_t exinf, CYCHDR cychdr,
                    RELTIM cyctim, RELTIM cycphs });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `cycid` | (a) | 周期ハンドラID |
| `cycatr` | (b) | 周期ハンドラ属性 |
| `exinf` | (c) | 拡張情報 |
| `cychdr` | (c) | 周期ハンドラの先頭番地 |
| `cyctim` | (b) | 起動周期（>0, ≤`TMAX_RELTIM`，単位ms） |
| `cycphs` | (b) | 起動位相（≥0, ≤`TMAX_RELTIM`） |

**周期ハンドラ属性**:
- `TA_NULL`(=0): 生成直後は動作していない状態
- `TA_STA`(=0x02): 生成時に動作開始
- `TA_PHS`(=0x04): **ASPではサポート外**

ASPでは `TA_STA` を指定して `cycphs=0` を指定すると警告（カーネル起動直後の最初のタイムティックで起動）。

例：
```c
CRE_CYC(CYC_HEARTBEAT,
        { TA_STA, (intptr_t)0, heartbeat_handler, 100, 50 });
```

周期ハンドラの先頭番地は次の形式：
```c
void heartbeat_handler(intptr_t exinf)
{
    /* 非タスクコンテキストで実行されるため，
       i接頭辞付きAPI（iset_flg, isig_sem, ipsnd_dtq, iact_tsk等）を使う */
}
```

### 4.2 CRE_ALM — アラームハンドラの生成

```c
CRE_ALM(ID almid, { ATR almatr, intptr_t exinf, ALMHDR almhdr });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `almid` | (a) | アラームハンドラID |
| `almatr` | (b) | アラームハンドラ属性（`TA_NULL` のみ） |
| `exinf` | (c) | 拡張情報 |
| `almhdr` | (c) | アラームハンドラの先頭番地 |

アラームハンドラは生成直後は動作していない状態。`sta_alm` で起動時刻を指定して動作開始する。

例：
```c
CRE_ALM(ALM_TIMEOUT, { TA_NULL, (intptr_t)0, timeout_handler });
```

## 5. システム状態管理機能

ASPではシステム状態に関する静的APIは存在しない（`SAC_SYS` は保護機能対応カーネル専用）。

## 6. 割込み管理機能

### 6.1 CFG_INT — 割込み要求ラインの属性設定

```c
CFG_INT(INTNO intno, { ATR intatr, PRI intpri });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `intno` | (b) | 割込み番号 |
| `intatr` | (b) | 割込み要求ライン属性（ビット和） |
| `intpri` | (b) | 割込み優先度（`TMIN_INTPRI`〜`TMAX_INTPRI`(=-1)） |

**割込み要求ライン属性**:
- `TA_NULL`(=0): 既定（割込み要求禁止フラグはセットされたまま）
- `TA_ENAINT`(=0x01): 割込み要求禁止フラグをクリア（割込み許可状態）
- `TA_EDGE`(=0x02): エッジトリガ
- `TA_POSEDGE`/`TA_NEGEDGE`/`TA_BOTHEDGE`/`TA_LOWLEVEL`/`TA_HIGHLEVEL`: ターゲット定義の予約属性

`CFG_INT` を行わずに `ATT_ISR` で割込み番号にISRを登録しようとすると `E_OBJ` エラー。`ATT_ISR` の前に `CFG_INT` を記述するのが基本。

例：
```c
CFG_INT(INTNO_BUTTON,
        { TA_ENAINT | TA_EDGE | TA_NEGEDGE, BUTTON_INT_PRI });
```

### 6.2 ATT_ISR — 割込みサービスルーチンの追加

```c
ATT_ISR({ ATR isratr, intptr_t exinf,
          INTNO intno, ISR isr, PRI isrpri });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `isratr` | (b) | 割込みサービスルーチン属性（`TA_NULL` のみ） |
| `exinf` | (c) | 拡張情報 |
| `intno` | (b) | 登録対象の割込み番号 |
| `isr` | (c) | 割込みサービスルーチンの先頭番地 |
| `isrpri` | (b) | ISR優先度（`TMIN_ISRPRI`(=1)〜`TMAX_ISRPRI`(=16)） |

ASPでは静的API `CRE_ISR`（ID付き）はサポートしない。常に `ATT_ISR`（ID無し）を使用する。

ISR記述形式：
```c
void button_isr(intptr_t exinf)
{
    /* 必ず割込み要求が発生したかチェックして，
       自分の割込みでなければ何もせずreturn */
    if (!button_check_irq()) return;
    button_clear_irq();
    iset_flg(FLG_BUTTON_PRESSED, BUTTON_BIT);
}
```

例：
```c
ATT_ISR({ TA_NULL, (intptr_t)0, INTNO_BUTTON, button_isr, 1 });
```

1つの割込み要求ラインに複数のISRを登録すると，ISR優先度の高い順（同じ優先度ならファイル中の記述順）にすべて呼ばれる。

### 6.3 DEF_INH — 割込みハンドラの定義

```c
DEF_INH(INHNO inhno, { ATR inhatr, INTHDR inthdr });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `inhno` | (b) | 割込みハンドラ番号 |
| `inhatr` | (b) | 割込みハンドラ属性 |
| `inthdr` | (c) | 割込みハンドラの先頭番地 |

**割込みハンドラ属性**:
- `TA_NULL`(=0): カーネル管理の割込み
- `TA_NONKERNEL`(=0x02): カーネル管理外の割込み（高優先度・割込み応答性重視用）

割込みハンドラは ISR より低レベルで，カーネル管理外の割込みやカーネル標準の割込みハンドラで対応できないケースに使う。原則として `ATT_ISR` を使うべき。

割込みハンドラ記述形式：
```c
void irq_handler(void)
{
    /* exinf は渡されない */
    /* TA_NONKERNEL の場合は SIL のAPIと sns_ker, ext_ker のみ呼べる */
}
```

## 7. CPU例外管理機能

### 7.1 DEF_EXC — CPU例外ハンドラの定義

```c
DEF_EXC(EXCNO excno, { ATR excatr, EXCHDR exchdr });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `excno` | (b) | CPU例外ハンドラ番号 |
| `excatr` | (b) | CPU例外ハンドラ属性（`TA_NULL` のみ） |
| `exchdr` | (c) | CPU例外ハンドラの先頭番地 |

CPU例外ハンドラ記述形式：
```c
void cpu_exception_handler(void *p_excinf)
{
    /* p_excinf には CPU例外発生時の情報（PC・レジスタ等）が渡される */
    /* xsns_dpn(p_excinf), xsns_xpn(p_excinf) でリカバリ手段を判断 */
}
```

例：
```c
DEF_EXC(EXCNO_UNDEF, { TA_NULL, undef_exception_handler });
```

## 8. システム構成管理機能

### 8.1 DEF_ICS — 非タスクコンテキスト用スタック領域の定義

```c
DEF_ICS({ SIZE istksz, STK_T *istk });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `istksz` | (b) | スタックサイズ（バイト） |
| `istk` | (c) | スタック領域の先頭番地（`NULL` ならカーネルが確保） |

非タスクコンテキスト（割込みハンドラ・CPU例外ハンドラ等）で用いるスタック領域を定義する。`.cfg` 中に1回だけ記述（複数記述すると `E_OBJ` エラー）。

例：
```c
DEF_ICS({ ICS_STACK_SIZE, NULL });
```

### 8.2 ATT_INI — 初期化ルーチンの追加

```c
ATT_INI({ ATR iniatr, intptr_t exinf, INIRTN inirtn });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `iniatr` | (b) | 初期化ルーチン属性（`TA_NULL` のみ） |
| `exinf` | (c) | 拡張情報 |
| `inirtn` | (c) | 初期化ルーチンの先頭番地 |

カーネル動作開始前（カーネル非動作状態）に呼ばれる。`.cfg` の記述順に実行される。

記述形式：
```c
void init_routine(intptr_t exinf)
{
    /* デバイス初期化等。サービスコールはほぼ呼べない */
}
```

例：
```c
ATT_INI({ TA_NULL, (intptr_t)0, hardware_init });
```

### 8.3 ATT_TER — 終了処理ルーチンの追加

```c
ATT_TER({ ATR teratr, intptr_t exinf, TERRTN terrtn });
```

| パラメータ | 種別 | 意味 |
|---|---|---|
| `teratr` | (b) | 終了処理ルーチン属性（`TA_NULL` のみ） |
| `exinf` | (c) | 拡張情報 |
| `terrtn` | (c) | 終了処理ルーチンの先頭番地 |

`ext_ker` 呼出後に実行される。**`.cfg` の記述順とは逆順**に実行される（ATT_INIと反対）。

記述形式：
```c
void term_routine(intptr_t exinf)
{
    /* デバイスの停止等。サービスコールはほぼ呼べない */
}
```

## 9. 静的API一覧表（ASPサポート分）

| 静的API | 用途 |
|---|---|
| `CRE_TSK` | タスクの生成 |
| `DEF_TEX` | タスク例外処理ルーチンの定義 |
| `CRE_SEM` | セマフォの生成 |
| `CRE_FLG` | イベントフラグの生成 |
| `CRE_DTQ` | データキューの生成 |
| `CRE_PDQ` | 優先度データキューの生成 |
| `CRE_MBX` | メールボックスの生成 |
| `CRE_MTX` | ミューテックスの生成 |
| `CRE_MPF` | 固定長メモリプールの生成 |
| `CRE_CYC` | 周期ハンドラの生成 |
| `CRE_ALM` | アラームハンドラの生成 |
| `CFG_INT` | 割込み要求ラインの属性設定 |
| `ATT_ISR` | 割込みサービスルーチンの追加 |
| `DEF_INH` | 割込みハンドラの定義 |
| `DEF_EXC` | CPU例外ハンドラの定義 |
| `DEF_ICS` | 非タスクコンテキスト用スタック領域の定義 |
| `ATT_INI` | 初期化ルーチンの追加 |
| `ATT_TER` | 終了処理ルーチンの追加 |

## 10. 静的API使用例（最小システム）

```c
/* sample.cfg */
#include <kernel.h>
#include "sample.h"

/* 非タスクコンテキスト用スタック */
DEF_ICS({ DEF_ICSSZ, NULL });

/* 初期化ルーチン */
ATT_INI({ TA_NULL, (intptr_t)0, hardware_init });

/* タスク */
CRE_TSK(MAIN_TASK,
        { TA_ACT, 0, main_task, MAIN_PRI, STACK_SIZE, NULL });
CRE_TSK(LOG_TASK,
        { TA_NULL, 0, log_task, LOG_PRI, STACK_SIZE, NULL });

/* 同期・通信オブジェクト */
CRE_SEM(SEM_BUTTON, { TA_NULL, 0, 1 });
CRE_FLG(FLG_EVENT, { TA_WMUL, 0 });
CRE_DTQ(DTQ_LOG, { TA_NULL, 16, NULL });

/* 排他制御 */
CRE_MTX(MTX_LCD, { TA_CEILING, LCD_CEIL_PRI });

/* メモリプール */
CRE_MPF(MPF_PACKET, { TA_NULL, 8, sizeof(packet_t), NULL, NULL });

/* タイマ */
CRE_CYC(CYC_HB,    { TA_STA, 0, heartbeat_handler, 1000, 1 });
CRE_ALM(ALM_TMOUT, { TA_NULL, 0, timeout_handler });

/* 割込み */
CFG_INT(INTNO_BUTTON,
        { TA_ENAINT | TA_EDGE | TA_NEGEDGE, BUTTON_INT_PRI });
ATT_ISR({ TA_NULL, 0, INTNO_BUTTON, button_isr, 1 });
```

## 11. ID番号の参照と制限

### 11.1 オブジェクト識別名の規則

- 識別名は C言語の識別子規則に従う（英文字・数字・`_` で構成。先頭は数字以外）。
- 同一名のオブジェクトを `CRE_*` で複数回生成すると `E_OBJ` エラー（コンフィギュレータが報告）。
- 識別名は `kernel_cfg.h` に `#define` として展開されるため，他のマクロ・グローバル変数との名前衝突に注意。
- 慣例として全大文字を使う（例: `MAIN_TASK`, `SEM_BUTTON`）。

### 11.2 コンフィギュレータが生成するもの

`.cfg` を処理して生成されるファイル：

- `kernel_cfg.c` — オブジェクトの初期化テーブル，初期化処理・終了処理を含むC言語ファイル。
- `kernel_cfg.h` — オブジェクト識別名のID番号定義を含むヘッダ。アプリケーションは `#include "kernel_cfg.h"` で参照。

ASPでは `TNUM_TSKID`, `TNUM_SEMID`, `TNUM_FLGID`, ... 等の構成マクロが `kernel_cfg.h` に出力される。これらは `.cfg` で `CRE_*` を記述した数に等しい。

## 12. 推奨スタイル（LLM出力規約）

LLMが TOPPERS/ASP の `.cfg` を生成・編集するときの指針：

1. **必ず `#include <kernel.h>` を冒頭に書く**。アプリ固有の定義は別ヘッダにまとめて `#include "myapp.h"` する。
2. **オブジェクト識別名は全大文字**。複数のオブジェクトに対して接頭辞を統一する（例: `TASK_xxx`, `SEM_xxx`, `MTX_xxx`）。
3. **タスクは `MAIN_TASK` など意味のある識別名**。
4. **`CRE_TSK` で `stk` に `NULL` を指定**するとカーネルが自動でスタックを確保する。固定アドレスにスタックを置く必要があれば `STK_T` 配列で確保し，`COUNT_STK_T(size)` でサイズを指定。
5. **割込み登録は `CFG_INT` → `ATT_ISR` の順**に書く。優先度・属性・トリガモードを必ず明示する。
6. **`DEF_ICS` を必ず1回書く**。スタックサイズは `DEF_ICSSZ` のような既定マクロを使うか，アプリで定義する。
7. **`ATT_INI` で初期化したものを `ATT_TER` で逆順に終了処理する**よう設計する（記述順は同じでも，`ATT_TER` は逆順実行されるため整合する）。
8. **オブジェクト属性は `TA_NULL` か `TA_xxx` の論理和**で記述。`0` をマジックナンバーで書かない。**廃止された値0属性（`TA_TFIFO`, `TA_MFIFO`, `TA_HLNG`, `TA_WSGL`）は使わない**。FIFO順がデフォルトなので明示しない（タスク優先度順なら `TA_TPRI` を指定）。
9. **拡張情報 `exinf` は使わないなら `(intptr_t)0`**。型キャストを明示する。
10. **`#if`/`#ifdef` で機能を切り替える設計**は許容。ただし1つの静的APIの途中に条件ディレクティブを挿入することは禁止。
11. **`AID_*` は静的APIでもASP標準パッケージではサポートされない**（動的生成パッケージ専用）ので使わない。
12. **`DOMAIN`/`KERNEL_DOMAIN`/`CLASS` の囲みは絶対に使わない**（保護機能対応・マルチプロセッサ対応カーネル専用）。

## 13. 範囲外静的API（参考）

LLMが TOPPERS/ASP用 `.cfg` を生成するときに **使ってはならない** 静的APIのリスト：

- `AID_TSK`, `AID_SEM`, `AID_FLG`, `AID_DTQ`, `AID_PDQ`, `AID_MBX`, `AID_MTX`, `AID_MBF`, `AID_MPF`, `AID_CYC`, `AID_ALM`, `AID_ISR`, `AID_SPN` — 動的生成パッケージのみ
- `SAC_TSK`, `SAC_SEM`, `SAC_FLG`, `SAC_DTQ`, `SAC_PDQ`, `SAC_MTX`, `SAC_MBF`, `SAC_MPF`, `SAC_CYC`, `SAC_ALM`, `SAC_ISR`, `SAC_SYS` — 保護機能対応カーネルのみ
- `CRE_ISR` — ASPでは `ATT_ISR` のみサポート
- `CRE_MBF` — メッセージバッファはASPでは未サポート
- `CRE_SPN` — スピンロックはマルチプロセッサ対応カーネルのみ
- `DEF_OVR` — オーバランハンドラは拡張パッケージのみ
- `DEF_SVC` — 拡張サービスコールは保護機能対応カーネルのみ
- `DEF_EPR` — 制約タスク用，SSPカーネル限定
- `ATT_REG`, `DEF_SRG`, `ATT_SEC`, `ATA_SEC`, `LNK_SEC`, `ATT_MOD`, `ATA_MOD`, `ATT_MEM`, `ATA_MEM`, `ATT_PMA`, `ATA_PMA` — メモリオブジェクト管理（保護機能対応のみ）
- `LMT_DOM` — 保護ドメイン制限（保護機能対応のみ）
- `DEF_STK` — 共有スタック領域定義（SSP固有）
- 保護ドメインの囲み `DOMAIN(...)`, `KERNEL_DOMAIN { ... }` — 保護機能対応のみ
- クラスの囲み `CLASS(...)` — マルチプロセッサ対応のみ

これらの記述があった場合，LLMは「ASPの標準パッケージでは利用不可」と注意して，**ASP標準にある代替APIを提示**すること。

## 14. 廃止された属性（μITRON 4.0 → ASP）

ASP仕様で廃止された値0の属性は，`itron.h` を明示的にインクルードしない限り使用できない。**ASPアプリケーションでは以下を使わないこと**：

| 廃止属性 | 元の意味 | ASPでの正解 |
|---|---|---|
| `TA_TFIFO`(=0x00) | 待ち行列FIFO順 | 何も指定しない（属性ゼロは `TA_NULL`） |
| `TA_MFIFO`(=0x00) | メールボックスのメッセージキューFIFO順 | 何も指定しない |
| `TA_HLNG`(=0x00) | 高級言語インタフェース | 何も指定しない |
| `TA_WSGL`(=0x00) | 単一タスクのみ待ち | 何も指定しない（複数待ちなら `TA_WMUL` 明示） |

合わせてμITRON 4.0で定義されていた以下のサービスコール・マクロも **ASP仕様からは廃止**：

- `frsm_tsk` — `rsm_tsk` で代替（`itron.h` でマクロ定義）
- `set_tim` — システム時刻の設定機能を廃止
- `TMAX_SUSCNT` — 強制待ち要求ネスト数の最大値（1に固定，`itron.h` に定義）
