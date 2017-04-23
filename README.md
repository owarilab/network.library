# General-purpose Data Transmission Library  
  
////////////////////////////////////////////////////////  
// build( linux )  
////////////////////////////////////////////////////////  
cd core/src  
make  
  
cp ../libgdt_core.a /path/to/develop_branch/archive/0.1.0/linux/libgdt_core.a  
  
cd /path/to/develop_branch/archive/0.1.0/linux/linux_sample  
make  
  
./gdt_sample  
  
////////////////////////////////////////////////////////  
// build( mac )  
////////////////////////////////////////////////////////  
makeファイルのプリプロセッサの指定_LINUXから_BSD_UNIXに変更すればlinuxと同じ  

////////////////////////////////////////////////////////  
// build( windows )  
////////////////////////////////////////////////////////  
※http_serverはlinux sampleからコピー。  
  
Visual Studioのプロジェクトを新規作成し  
coreのソースコードとヘッダーファイルを全部コピーし  
windowsサンプルのソースコードを追加。  
  
ソリューションのプロパティを変更  
・プリプロセッサの定義に_WINDOWSを追加  
・追加の依存ファイルにws2_32.libを追加  
  
ビルド  


