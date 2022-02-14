# lambdaize-loop
卒論用に作ったLLVM IRの難読化プログラムです。卒論を書き終わったら公開してここにURLも貼るので、詳しい原理はそっちを見てください。とりあえずは最低限の使い方のみ書いておきます。コンパイルと使用にはLLVMのインストールが必要になります。
## lambdaize-loop
このプログラムの本体に当たる部分です。makeコマンドでlambdaize-loop.soができるので、
```
opt -load-pass-plugin lambdaize-loop.so -passes=lambdaize-loop -o OBFUSCATED_IR INPUT_IR
```
とするとINPUT_IRを難読化してOBFUSCATED_IRができます。但しこれ単体ではまだコンパイルできません。まずlooperディレクトリのほうでmakeコマンドを叩いてlooper.bcを作ってください。またこのとき、`make MAX_RECURSION_COUNT=N`とすると再帰を行う最大回数を指定できます(デフォルトでは8192回)。そのあとOBFUSCATED_IRとlooper.bcをこんな感じでリンクしてください。
```
llvm-link -o OUTPUT_IR OBFUSCATED_IR looper.bc
```
あとはOUTPUT_IRをClangでコンパイルすれば実行ファイルになります。
## test
名前の通りテストに使っていたディレクトリです。`test.sh SOURCE [INPUT]`とすると、SOURCEを普通にコンパイルしてできた実行ファイルにINPUTを入力したときの出力とSOURCEを難読化してからコンパイルしてできた実行ファイルにINPUTを入力したときの出力がちゃんと一致するか調べてくれます。例えばこんな感じで使えます。
```
test/test.sh test/sha256.cpp /bin/ls
```
## utilities
卒論用の資料を作るのに使っていた便利スクリプト類です。
### average_time.sh
`average_time.sh -n REPEAT_TIME EXE_FILE`とするとEXE_FILEをREPEAT_TIME回実行し、1回あたりの平均実行時間を表示してくれます。
### plot-instdist.sh
`plot-instdist.sh EXE_FILE1 EXE_FILE2`とするとEXE_FILE1とEXE_FILE2それぞれに含まれる機械語命令の出現分布をgnuplotで表示してくれます。
### count-cyclomatic-complexity
`count-cyclomatic-complexity/count-cyclomatic-complexity.sh INPUT_IR`とするとINPUT_IR内の関数の数、基本ブロックの数、辺の数、循環的複雑度を表示してくれます。
