# 配布計画メモ（将来用・未実施）

> **状態：保留（2026-06-19）**。今は実施しない。将来 toppers-skills を複数PCで低手間に
> 使う／更新するときの方針メモ。実施するときは本書の手順に従う。

## 背景：いまの保管レイアウトは「発見されない」

Claude Code がスキルを自動発見するパスは次のいずれか：

| スコープ | パス |
|---|---|
| 個人（全プロジェクト） | `~/.claude/skills/<name>/SKILL.md` |
| プロジェクト | `<repo>/.claude/skills/<name>/SKILL.md` |
| プラグイン | `<plugin>/skills/<name>/SKILL.md` |
| `--add-dir` 追加ディレクトリ | `<dir>/.claude/skills/<name>/SKILL.md` |

`references/`・`examples/` 等の同梱ファイルは可。**SKILL.md の frontmatter は
`description` が実質必須**（自動発動の判定に使う。`name` はディレクトリ名から自動）。

現在の保管は `toppers-skills/<name>/skill/SKILL.md`。**これはどの発見パスにも一致しない**
＝そのままでは各PCで使えない。配布の仕組みが要る。

## スキルは2カテゴリ。配り方が違う

### ① リポ固有スキル（例：asp3_core の `asp3-core-ops`）→ 各リポにコミット
各開発リポの `.claude/skills/<repo>-ops/` にコミットしておけば、**別PCでは
`git clone`/`pull` するだけでプロジェクトスキルとして自動発見**。追加作業ゼロ。
固有スキルはこの方式が最小手間。マーケットプレイスには載せない。

> `.claude/` を gitignore しているリポは `.gitignore` に `.claude/*` ＋ `!.claude/skills/`
> を入れて skills だけ追跡する（asp3_core で実施済みの方法）。

### ② 汎用スキル（このリポジトリ toppers-skills）→ プラグイン＋マーケットプレイス
各PCの `~/.claude/skills/` 相当へ届ける必要がある。**推奨＝Claude Code のプラグイン＋
マーケットプレイス**（GitHub）。公式サポートで、複数PC・自動更新に向く。
（`git clone + symlink` 案は symlink 対応が**公式未確認**のため非推奨。）

## 将来の実施手順（プラグイン化）

### (1) レイアウト変更：`<name>/skill/` → `skills/<name>/`

```
toppers-skills/
  .claude-plugin/
    plugin.json
    marketplace.json
  skills/
    toppers-asp/         SKILL.md (+ references/ examples/)
    toppers-kernel-dev/  SKILL.md (+ references/)
    toppers-kernel-debug/SKILL.md (+ references/)
  AGENTS.md / CLAUDE.md / docs/   ← ガバナンス（人間・キュレーター用。プラグインには含めない）
```

- `toppers-asp` は現状 `toppers-asp/skill/` に加え `toppers-asp/doc`・`example`・`AGENTS.md`
  （生成元の素材）を持つ。プラグインに含めるのは `skill/`（SKILL.md＋references/＋examples/）
  だけ。素材を同梱するか分離するかは実施時に判断。

### (2) マニフェスト2ファイル

> このリポを単一プラグイン（`source: "./"`）として配る場合、`plugin.json` と
> `marketplace.json` は**同じ `.claude-plugin/` ディレクトリに同居**する（別ファイル名なので衝突なし）。

`.claude-plugin/plugin.json`（**version は書かない**＝commit SHA 追従で自動更新検知）：
```json
{
  "name": "toppers-skills",
  "description": "TOPPERS系RTOS（ITRON系）開発の汎用skill：toppers-asp / toppers-kernel-dev / toppers-kernel-debug",
  "author": { "name": "exshonda" }
}
```

`.claude-plugin/marketplace.json`（リポジトリルート）：
```json
{
  "name": "toppers",
  "owner": { "name": "exshonda" },
  "plugins": [
    {
      "name": "toppers-skills",
      "source": "./",
      "description": "TOPPERS系RTOS 汎用skill集"
    }
  ]
}
```

### (3) 各PCでの利用・更新（これだけ）

```bash
/plugin marketplace add exshonda/toppers-skills   # 初回1回
/plugin install toppers-skills@toppers            # 初回1回（3スキル一括）
/plugin marketplace update                         # 更新時（version省略なら自動検出も働く）
```

## 落とし穴・メモ

- **`version` は固定しない**：固定すると push しても各PCが古いまま。省略で commit SHA 追従。
- スキルが認識されない典型：SKILL.md のパス誤り／frontmatter の `---` 囲み忘れ／
  プラグインの `skills/` 配置誤り（`.claude-plugin/` の中ではなくプラグインroot直下）。
- SKILL.md 本文の変更は同一セッションで反映されることがあるが、構成変更後は再起動が確実。
- **symlink での `~/.claude/skills/<name>` 共有は公式未確認**。使うなら各自で動作確認。
- 固有スキルは①（各リポにコミット）で運ぶ。マーケットプレイスは汎用 toppers-skills 専用に保つ。

## 判断の記録
- 2026-06-19：プラグイン化は**保留**（今は実施しない）。本メモを残し、必要になった時点で
  上記 (1)〜(3) を実施する。
