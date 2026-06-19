# CLAUDE.md

このリポジトリ（toppers-skills）の運用規約・分担・命名・提案フローは
**AGENTS.md** を正本とします。作業前に必ず AGENTS.md を読んでください。

@AGENTS.md

---

## 要点（AGENTS.md より）

- ここは **TOPPERS系RTOSの「汎用・実装非依存」skill** を集約する場。
- **実装固有**（コマンド・パス・ターゲット名・出力形式・台帳ファイル名）は
  **各開発リポジトリの `.claude/skills/<repo>-ops/`** に置く。ここには書かない。
- 汎用skill：`toppers-asp`（アプリ）／`toppers-kernel-dev`（カーネル作業規約）／
  `toppers-kernel-debug`（診断）。命名は汎用＝`toppers-*`、固有＝`<repo>-ops`。
- 各リポのClaudeは、汎用候補は**提案**として上げる（直 push しない）。判定の一言：
  **「別のTOPPERS派生でもそのまま通じるか？」** 通じない＝固有＝自リポへ。
