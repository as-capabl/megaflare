# はじめに
## このライブラリは
本ライブラリは、OpenCL API をラップする、C++11で作成されたテンプレートライブラリです。

現在プレビュー版です。*まだ実用はできません!!*test/buffer.cppおよびtest/stress.cppでやっている事が、可能な事の全てです。

## 注意事項
プレビュー版故、APIの仕様等は、今後予告なく変更される可能性があります。

# 動作環境
- OpenCL 1.1 のいずれかの実装
  - AMD APP SDK 2.6で動作確認
- boost buildとbjam
  - 1.49で動作確認
- C++コンパイラ 要C++11のいくつかの機能
  - gcc-4.7.2 --enable-libstdcxx-timeオプション付きビルドで動作確認

# インストール方法
## ソースの準備
megaflareのソースツリーを、ホームディレクトリ等書き込みアクセスが可能な場所に配置して下さい。必要に応じて、git modules updateを行って、サブモジュールをダウンロードして下さい。

## boostファイルの準備とBOOST_ROOT環境変数の設定
boostのソースツリーを、tar ballやVCS等で用意します。ホームディレクトリ等書き込みアクセスが可能な場所に配置し、そのパスをBOOST_ROOT環境変数に設定して下さい。

## テストのビルド
ソースツリーのtestディレクトリに移動し、bjamコマンドを実行して下さい。ビルドが行われ、テストが実行されます。

## ライブラリとしての使用
### bjamを使用する場合
Jamfile等で以下のように宣言します。
    use-project megaflare : (本READMEがあるディレクトリへのパス) ;
	
すると、以下のターゲットが使用できるようになります。
- megaflare//
  - boost-log
  - opencl
  - code
  - host 
megaflare/code.hppをインクルードする場合はmegaflare//code, megaflare/host.hpp をインクルードする場合はmegaflare//hostを、exe等のルールのソースに指定して下さい。

### bjamを使用しない場合
本READMEがあるディレクトリとSprout, $BOOST_ROOT, boost-logディレクトリをインクルードパスに設定し、OpenCL及びboost::logの必要ライブラリをリンクしてビルドして下さい。

# 情報
## 作者
as_capabl <as.capabl@gmail.com>
## サイト
- https://github.com/as-capabl
- http://d.hatena.ne.jp/acaaN/
# 著作権
このライブラリは Boost Software License の元で公開されています。


Copyright (C) 2011-2012 as_capabl.

Distributed under the Boost Software License, Version 1.0.  (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
