# EMU8088_57Q BIOS V2.0 CP/M-86<br>
<br>
MS-DOSを動かしているファームウェアの方式で、CP/M-86が動作するように<br>
PICのファームウェアと、CBIOSを更新しました。<br>
MS-DOSと同じように、8088/V20の８Mhz版に対応し、8Mhzで動作するようになります。<br>
<br>

# 2-29-2024 新規ソフト追加<br>
<br>
CP/M-86で動作する、以下の新規ソフトをDRIVEJに追加しました。<br>
1. GM88.CMD    : GAME言語（インタープリタ）<br>
2. TTBASIC.CMD : 豊四季タイニーBASIC（インタープリタ）<br>
3. VTL88.CMD   : Very Tiny Language（インタープリタ）<br>
<br>
電脳伝説さん（@vintagechips）のSBCV20で動かしていたものをCP/M-86で<br>
動作するようにコンバートしたものです。コンバートしただけで、新規機能<br>
はありません。（プログラムロードとかあれば便利なのでしょうけど^^;）<br>
詳細は以下を参照してください。<br>
https://github.com/akih-san/SBCV20_8088
<br>

# BIOS V2.0のCP/M-86起動画面<br>
![EMU8088 1](photo/emu88v2_cpm.png)


SBCの詳細と、CP/M-86の詳細については、以下を参照してください。<br>
https://github.com/akih-san/EMU8088_57Q_CPM86

<br>
FWのソースのコンパイルは、マイクロチップ社の<br>
<br>
「MPLAB® X Integrated Development Environment (IDE)」<br>
<br>
を使っています。（MPLAB X IDE v6.10）コンパイラは、XC8を使用しています。<br>
https://www.microchip.com/en-us/tools-resources/develop/mplab-x-ide<br>
<br>
8088/V20用のアセンブラは、Macro Assembler AS V1.42を使用しています。<br>
http://john.ccac.rwth-aachen.de:8000/as/<br>
<br>
FatFsはR0.15を使用しています。<br>
＜FatFs - Generic FAT Filesystem Module＞<br>
http://elm-chan.org/fsw/ff/00index_e.html<br>
<br>
SDカード上のCP/Mイメージファイルの作成は、CpmtoolsGUIを利用しています。<br>
＜CpmtoolsGUI - neko Java Home Page＞<br>
http://star.gmobb.jp/koji/cgi/wiki.cgi?page=CpmtoolsGUI<br>
<br>
<br>
＜＠hanyazouさんのソース＞<br>
https://github.com/hanyazou/SuperMEZ80/tree/mez80ram-cpm<br>
<br>
<br>
＜@electrelicさんのユニバーサルモニタ＞<br>
https://electrelic.com/electrelic/node/1317<br>
<br>
<br>
＜参考＞<br>
・EMUZ80<br>
EUMZ80はZ80CPUとPIC18F47Q43のDIP40ピンIC2つで構成されるシンプルなコンピュータです。<br>
<br>
＜電脳伝説 - EMUZ80が完成＞  <br>
https://vintagechips.wordpress.com/2022/03/05/emuz80_reference  <br>
＜EMUZ80専用プリント基板 - オレンジピコショップ＞  <br>
https://store.shopping.yahoo.co.jp/orangepicoshop/pico-a-051.html<br>
<br>
・SuperMEZ80<br>
SuperMEZ80は、EMUZ80にSRAMを追加し、Z80をノーウェイトで動かすことができるメザニンボードです<br>
<br>
SuperMEZ80<br>
https://github.com/satoshiokue/SuperMEZ80<br>
<br>
