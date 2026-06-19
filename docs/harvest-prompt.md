# skill 棚卸し 定型プロンプト（各開発リポで使う）

各TOPPERS開発リポジトリ（asp3_core / asp3_tz_work / 各SDK統合リポ 等）のClaudeセッションに
**そのまま貼って**使う。固有skillは自リポに作り、汎用候補は toppers-skills へ提案させる。

- 置換するプレースホルダは `{{TOPPERS_SKILLS}}` の1つだけ … toppers-skills リポのパス
  （例：`/home/honda/TOPPERS/toppers-skills`）。
- 本文中の `<repo>` は自リポの短い識別子（プロンプト内で定義済み。例：asp3-core → `asp3-core-ops`）。置換不要。

---

## コピペ用プロンプト

```
このリポジトリから「skill 化すべき情報」を棚卸しして整理してください。
分担の正本は {{TOPPERS_SKILLS}}/AGENTS.md です。まずそれを読んでから進めてください。

## 前提（分担ルール）
- 汎用（実装非依存＝別のTOPPERS派生でもそのまま通じる概念）→ toppers-skills へ「提案」。
  直接 push しない。既存の汎用skillと重複させない：
    - toppers-asp（アプリ：API/静的API/エラー辞書/例示）
    - toppers-kernel-dev（カーネル作業規約：禁則/静的生成/移植/上流マージ）
    - toppers-kernel-debug（診断：症状→原因/観測/TTSP3概念）
- 実装固有（コマンド・プリセット名・スクリプト・ファイルパス・ターゲット名・出力形式・
  台帳ファイル名）→ 自リポの .claude/skills/<repo>-ops/ に直接作る（自分の所有物）。
- 判定の一言：「別のTOPPERS派生でもそのまま通じるか？」
  通じる＝汎用＝提案／通じない＝固有＝自リポ。

## やること

1. 棚卸し（読むだけ・変更しない）
   - AGENTS.md / CLAUDE.md / README / docs/ / 主要な Makefile・CMake・スクリプト・
     テスト・移植ガイド・台帳（DIVERGENCE_MAP 等）を確認。
   - 「このリポで作業するAIが繰り返し必要とする非自明な情報」を洗い出す。
     既に AGENTS.md/docs に十分書いてあるものは skill 化しない（重複を避ける）。

2. 仕分け（固有 / 汎用候補）
   - 固有：ビルド/実行/解析の具体コマンド、プリセット、スクリプト、ターゲット名、
     出力形式、台帳ファイル名、固有の落とし穴。
   - 汎用候補：実装に依存しない概念・規約・診断の着眼で、上記3つの汎用skillに
     まだ無いもの。

3. 固有 skill を作る（自リポ）
   - .claude/skills/<repo>-ops/SKILL.md を作成（必要なら references/ に分割）。
     ※ <repo> は短い識別子（例：asp3-core → asp3-core-ops）。
   - SKILL.md frontmatter：
       name: <repo>-ops
       description: <発動条件を具体語で。日本語＋英語キーワード。
                    他skill（toppers-* / 自リポ）との使い分けを明記>
   - 本文は「このリポでの叩き方」に徹し、規約は AGENTS.md、概念は toppers-skills の
     該当skill（toppers-asp / toppers-kernel-dev / toppers-kernel-debug）へリンクして委譲。
   - コマンド・手順は実際にビルド/実行して**裏取りしてから**書く（憶測で書かない）。
   - .claude/ を gitignore しているリポは、.gitignore に `.claude/*` と `!.claude/skills/`
     を入れて skills だけ追跡対象にする。

4. 汎用候補を「提案」にまとめる（push しない）
   - {{TOPPERS_SKILLS}} の既存3 skill のどれに、何を、なぜ足すべきか（または新規skillが
     要るか）を、提案メモとして列挙する。各項目に**次のメタ情報を必ず付ける**：
       - 対象skill … 追記先（references/SKILL のどこか）
       - 追記案の要旨 … 1〜2行
       - 分類 … 知見の種別（例：ブート/例外・ベクタ/整列・観測/デバッガ・観測/シリアル・
         移植/リンカ・統合/CI 等）
       - プロセッサ … 該当アーキ（例：ARMv8-M／ARM Cortex-M／非依存）。
         アーキ非依存なら「非依存」と書く（ホスト側・ツール依存もここで明示）
       - 出所リポ … その知見が出た開発リポ（`asp3_mcuxsdk` 程度の粒度でよい）
       - 実装非依存の根拠 … 固有名・コマンドを含まないこと
   - 取り込み時は本文に注記として残す（書式例）：
     `〔分類: ブート/例外 ｜ プロセッサ: ARMv8-M ｜ 出所: asp3_mcuxsdk〕`
     ＝**分類・プロセッサ・出所は注記（メタ情報）として許容**。本文の説明そのものは
     実装非依存に保つ（固有のコマンド・パス・ターゲット名は本文へ書かない）。
   - そのまま toppers-skills のキュレーターに渡せる形（PR下書き or 箇条書き）で出力。

## 出力（この順で報告）
A. 作成した固有 skill のパスと概要（裏取りした検証結果つき）
B. 汎用候補の提案リスト（対象skill・要旨・**分類・プロセッサ・出所リポ**・根拠）。無ければ「なし」
C. 重複のため skill 化しなかったもの（既に AGENTS.md/docs にある等）

制約：toppers-skills を直接編集・push しない（提案のみ）。固有skillの手順は裏取り必須。
```

---

## 使い方メモ

- 新規リポでも既存リポでも使える（既存skillがあれば「重複させず追記提案」になる）。
- 出力 B（汎用候補）を受け取ったら、toppers-skills 側のキュレーターが §6 のフローで
  dedup・命名・実装非依存性を点検して取り込む（AGENTS.md §6）。
- 上流ソース系（TTSP3/FMP3/HRP3）はそれ自体に skill を置かない。得た汎用知識は提案で反映。
