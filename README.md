# spwave (Windows build fork)

これは板野秀樹氏(名城大学)による音声ファイルエディタ **spwave 0.9.0** の
ソースコードをもとにした、Windows ビルド用のフォークリポジトリです。

- 公式サイト: https://www-ie.meijo-u.ac.jp/~banno/spLibs/spwave/index-j.html
- オリジナルの著作権は板野秀樹氏に帰属します

## このリポジトリの内容

- spwave 0.9.0 のソース一式(`spwave/`, `help/`)
- Windows (x64) ビルドの自動化
  - [setup-deps.ps1](setup-deps.ps1) — 依存ライブラリ spLibs のダウンロード・配置
  - [make-dist.ps1](make-dist.ps1) — ビルド成果物を `dist/win-x64/` に集約
  - 手順の詳細は [BUILD-WINDOWS.md](BUILD-WINDOWS.md)
- ビルド済みイメージ(`dist/win-x64/` — そのまま実行可能)

## オリジナルからの変更点

- **quit_prompt 設定の追加**: 終了時の確認ダイアログを無効化できるオプション。
  環境設定ダイアログ「外観」タブの「終了する前に尋ねる」チェックボックス、
  または設定ファイル(`%APPDATA%\spwave\.spwave`)の `quit_prompt False` で設定。
  詳細は [BUILD-WINDOWS.md](BUILD-WINDOWS.md) を参照。

変更履歴はコミットログを参照してください。

## ライセンス

- spwave 本体: LGPL v3([spwave/LICENSE.txt](spwave/LICENSE.txt))
- 静的リンクしている spLibs(spBase / spLib / spAudio / spComponent)および
  同梱プラグイン(spPlugin): MIT系ライセンス
  ([THIRD-PARTY-LICENSES.txt](THIRD-PARTY-LICENSES.txt) 参照)
- Monkey's Audio / Windows Media Audio プラグインは、SDK のライセンスが
  別条件のため `dist/` には含めていません。必要な場合は公式の spPlugin
  パッケージから取得してください。
