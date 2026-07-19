# spwave 0.9.0 Windows ビルド手順

spwave は音声ファイルエディタ(作者: 板野秀樹氏 / 名城大学)。
公式サイト: https://www-ie.meijo-u.ac.jp/~banno/spLibs/spwave/index-j.html

## 必要環境

- Windows 10/11
- Visual Studio(v143 ツールセット = VS2022 相当の MSVC。VS2026 でも v143 コンポーネントを追加すれば可)
- spLibs のバイナリライブラリ(下記)

## クイックスタート

```powershell
.\setup-deps.ps1     # spLibs のダウンロード・展開・配置(下記1と3を自動実行)
MSBuild.exe spwave\spwave.vcxproj /p:Configuration=Release /p:Platform=x64
```

取得し直す場合は `.\setup-deps.ps1 -Clean`、バージョン変更は `-SpBaseVer` などの
パラメータで指定(詳細は `Get-Help .\setup-deps.ps1`)。zip は `.deps\` に
キャッシュされる。以下は手動で行う場合の手順。

## 1. spLibs バイナリの取得と配置

公式アーカイブ https://www-ie.meijo-u.ac.jp/~banno/archive/ から以下を取得する
(バージョンは 2025/05 時点のもの。spLibs ページ https://www-ie.meijo-u.ac.jp/~banno/spLibs/index-j.html で最新版を確認):

| アーカイブ | 用途 |
|---|---|
| spBase-0.8.25-1.bin.zip | 基盤ライブラリ |
| spLib-0.9.5-1.bin.zip | 信号処理ライブラリ |
| spAudio-0.7.16-4.bin.zip | オーディオ入出力 |
| spComponent-0.6.23-1.bin.zip | GUI コンポーネント |
| spPlugin-0.8.6-4.win64.zip | 実行時プラグイン(ファイル入出力に必須) |

各 bin.zip を展開し、リポジトリ直下に以下の形でマージ配置する
(`spwave.vcxproj` が `..\include` と `..\lib\$(Platform)\$(PlatformToolset)` を参照するため):

```
spwave-0.9.0/
├── include/sp/*.h        ← 各アーカイブの include/sp/ を全部マージ
├── lib/x64/v143/*.lib    ← 各アーカイブの lib/x64/v143/ の .lib をコピー
│     (spBase.lib, sp.lib, spAudio.lib, spComponent.lib と各 MT 版)
├── spwave/               ← spwave 本体ソース(このリポジトリに含まれる)
└── help/
```

Win32 でビルドする場合は `lib/v143/`、ARM64 は `lib/ARM64/v143/` も同様に配置する。

## 2. ビルド

Developer PowerShell などから:

```powershell
MSBuild.exe spwave\spwave.vcxproj /p:Configuration=Release /p:Platform=x64
```

または `spwave\spwave.sln` を Visual Studio で開いて Release|x64 をビルド。
成果物は `spwave\x64\Release\spwave.exe`。

構成は Release(/MD)・ReleaseMT(/MT、MT 版 .lib とリンク)・Debug・DebugMT の4種、
プラットフォームは Win32 / x64 / ARM64。

## 3. プラグインの配置(必須)

spwave はファイル入出力をすべてプラグインで行うため、プラグインなしでは
ファイルを一切開けない。spPlugin-0.8.6-4.win64.zip 内の `plugins` フォルダを
exe と同じ場所にコピーする:

```
spwave\x64\Release\
├── spwave.exe
└── plugins\   (input_wav.dll, output_wav.dll, input_audio.dll, ...)
```

## このリポジトリ独自の変更

- **quit_prompt 設定の追加**: すべてのウィンドウを閉じる/終了するときの
  「終了しますか?」確認ダイアログをオプションで無効化できる。
  - 環境設定ダイアログ「外観」タブの「終了する前に尋ねる」チェックボックス、
    または設定ファイル `%APPDATA%\spwave\.spwave` に `quit_prompt False` を
    記載して無効化(デフォルトは従来通り確認あり)。
  - ファイル編集済みの場合の保存確認は従来通り表示される。

## 備考

- ソース中の文字コード警告(C4819)とポインタキャスト警告(C4311/C4312)は
  アップストリーム由来で無害。
- MP3 書き出しはライセンス上の理由で別配布の spMpeg プラグインが必要。
- ライセンス: LGPL v3(spwave/LICENSE.txt)
