/*
 * test script
 *
 * メモリ効率とか速度を全く考えてないので
 * 起動時のコンフィグ読み込み用途とか
 * ちょっとした処理には使えると思う。
 */

echo( "\n\n");
// echo関数とかiのc側組み込み関数はgdt_add_system_functionで指定して増やせる
echo( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );
echo( "@@ start : test.gs\n" );
echo( "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n" );

// ハッシュを変数に格納
party = [
	[
		"name" : "player1",
		"level" : 1,
		"exp" : 0,
		"skill" : [
			"name":"normal",
			"effect":"attack",
			"effect_value":5
		]
	],
	[
		"name" : "player2",
		"level" : 1,
		"exp" : 0,
		"skill" : [
			"name":"normal",
			"effect":"attack",
			"effect_value":5
		]
	]
];

// json encodeのテスト
json = json_encode( party );
echo( "echo json string\n");
echo( json  + "\n\n");

// json decodeのテスト
json_hash = json_decode( json );
echo( "dump json hash\n");
echo( json_hash );
echo( "\n");

// 繰り返し処理( まだwhileしかないしメモリのこと考えてないのであまり使えない )
i = 0;
while( i < 10 )
{
	// 計算はできるけどまだ+=とか実装してない
	i = i + 1;
}
echo( "i : "+ i +"\n" );

// if文テスト
if( i == 10 ){
	echo("i == 10\n");
}
else{
	echo("i != 10\n");
}
