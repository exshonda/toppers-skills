# AGENTS.md

このリポジトリは TOPPERS/ASPカーネル向けアプリケーションを書くためのリソース集である。

## 重要な前提（必読）

- ASPカーネルは **μITRON 4.0仕様をベースとしているが完全互換ではない**。`TA_TFIFO`/`TA_MFIFO` 等の値=0属性，`acre_*`/`del_*` の動的生成API，`frsm_tsk`/`set_tim` 等は **使えない**。差分の詳細は `doc/TOPPERS-ASP_RTOS仕様.md` の §1.1 を参照。
- **すべてのカーネルオブジェクトは静的API（`.cfg`）で生成**する。動的生成APIはASP標準では未サポート。タスク・セマフォ・イベントフラグ・データキュー・ミューテックス・固定長メモリプール・周期ハンドラ・アラームハンドラ・ISR等は `.cfg` の `CRE_*`/`DEF_*`/`ATT_*`/`CFG_INT` で予め登録する。

## マニュアル

`doc/` 配下のスタイルガイドを参照すること（`skill/references/` にも英名で同内容のコピーあり）。

- `doc/TOPPERS-ASP_RTOS仕様.md` — 概念・状態モデル・初期化／終了。**§1.1 にμITRON 4.0との差分一覧**
- `doc/TOPPERS-ASP_API仕様.md` — サービスコール（C言語API）
- `doc/TOPPERS-ASP_静的API_API仕様.md` — `.cfg` の静的API
- `doc/TOPPERS-ASP_静的API_エラー.md` — エラーコード一覧
- `doc/TOPPERS-ASP_クイックルール.md` — FreeRTOS/Zephyr対応表＋頻出パターン

## サンプル

`example/` 配下のサンプル19本をテンプレートとして使う。インデックスは `example/README.md` を参照。
