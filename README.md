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

## リリース方法

`v` で始まるタグを push すると、GitHub Actions がビルドして
`spwave-<タグ>-win-x64.zip` を Releases に自動で公開します。

```powershell
git tag v0.9.0-1
git push cartooh v0.9.0-1
```

## ライセンス

- spwave 本体: LGPL v3([spwave/LICENSE.txt](spwave/LICENSE.txt))
- 静的リンクしている spLibs(spBase / spLib / spAudio / spComponent)および
  同梱プラグイン(spPlugin): MIT系ライセンス
  ([THIRD-PARTY-LICENSES.txt](THIRD-PARTY-LICENSES.txt) 参照)
- 以下のプラグインは SDK・エンジンのライセンスが別条件のため `dist/` には
  含めていません(WAV 等の基本的な再生・編集には影響しません)。
  - Monkey's Audio(input/output_monkey.dll)
  - Windows Media Audio(input/output_wma.dll)
  - ASIO ドライバ対応(asio.dll — Steinberg ASIO SDK)
  - MP3 読み込み(input_mpeg.dll — FreeAmp/Zinf 由来エンジン, GPL系)

  これらの DLL は、[spLibs のページ](https://www-ie.meijo-u.ac.jp/~banno/spLibs/index-j.html)で配布されている
  **spPlugin の Windows バイナリパッケージ**(例:
  [spPlugin-0.8.6-4.win64.zip](https://www-ie.meijo-u.ac.jp/~banno/archive/spPlugin-0.8.6-4.win64.zip))の
  `plugins` フォルダにすべて同梱されています(ページ上に ASIO 等の個別の記載はありません)。
  必要な DLL を取り出して spwave.exe と同じ場所の `plugins\` フォルダに
  コピーすると有効になります。
