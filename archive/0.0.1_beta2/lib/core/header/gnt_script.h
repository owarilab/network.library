/*
 * Copyright (c) 2014-2016 Katsuya Owari
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * * Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 * * Neither the name of the <organization> nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software 
 *   without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _GNT_SCRIPT_H_
#define _GNT_SCRIPT_H_

#include "core.h"
#include "gnt_system.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define STRBUF_SIZE				1024	// トークン解析用文字バッファ(文字列の大きさを見て調整)
#define TOKEN_INIT_LENGTH		102400	// 初期トークンの領域確保サイズ(最初に確保される数がわかっていれば高速)
#define TOKEN_ADD_LENGTH		102400	// 確保した上限を超えたときに再確保する数(確保する回数が少ないほうが高速)
#define ID_UNK					0		// 不明
#define ID_NUM					1		// 数値
#define ID_OP					2		// 演算子
#define ID_SIGN					3		// 記号
#define ID_STR					4		// 文字列
#define ID_VAL					5		// 変数
#define ID_SYS					6		// 予約文字
#define STR_MAX					0x100	// 最大文字数(これは動的にメモリ確保できるようにしなければ・・・)
#define MAX_STACK				128		// 計算用スタックの最大数

#define OPE_PRIORITY_LOWEST		9		// 演算子の最低プライオリティ
#define OPE_END_PRIORITY		1		// 末端記号のプライオリティ
#define OPE_EQU_PRIORITY		2		// 代入演算子などのプライオリティ
#define OPE_CAST_PRIORITY		3		// 型のキャストのプライオリティ
#define OPE_COMP_PRIORITY		4		// 比較演算子などのプライオリティ
#define OPE_ADDSUB_PRIORITY		5		// 加減演算のプライオリティ
#define OPE_MULDIV_PRIORITY		6		// 乗除余演算のプライオリティ
#define OPE_SINGLE_PRIORITY		7		// 単項演算子のプライオリティ
#define OPE_SYSVAL_PRIORITY		8		// 変数定義のプライオリティ
#define ADDNODE					102400	// ノードのメモリ確保する数
#define ADDVARIABLE				64		// 変数のメモリ確保する数

// システムの予約語
#define SYS_INT					"int"	// 整数型
#define SYS_IF					"if"	// if文
#define SYS_ELSE				"else"	// else文

typedef struct NODE NODE;
typedef struct NODES NODES;
typedef struct RANGE RANGE;
typedef struct TOKEN TOKEN;
typedef struct TOKENS TOKENS;
typedef struct STACK STACK;
typedef struct STACKDATA STACKDATA;
typedef struct GNT_SCRIPT_TREE GNT_SCRIPT_TREE;
typedef struct VARIABLE VARIABLE;
typedef struct VARIABLES VARIABLES;
typedef struct TEXTBLOCK TEXTBLOCK;
typedef struct CODE CODE;

//-----------------------------------------------------
// 計算などを実行するときに使うスタック
//-----------------------------------------------------
struct STACKDATA{
	int id;
	int array_index;	// 配列参照用
	union{
		int data_int;
		/*
		int* data_int;
		float* data_float;
		double* data_double;
		long* data_long;
		char* data_char;
		*/
	};
};

//-----------------------------------------------------
// スタック管理用の構造体
//-----------------------------------------------------
struct STACK{
	STACKDATA sdata[MAX_STACK];
	int endpos;
};

//-----------------------------------------------------
// 範囲指定用の構造体
//-----------------------------------------------------
struct RANGE{
	int start;
	int end;
};

//-----------------------------------------------------
// トークン構造体
//-----------------------------------------------------
struct TOKEN{
	char* token;				// 文字列
	int type;					// トークンの種類
	unsigned int size;			// 文字サイズ
};

//-----------------------------------------------------
// トークン管理構造体
//-----------------------------------------------------
struct TOKENS{
	struct TOKEN* ptoken;		// トークン
	unsigned int size;			// メモリ確保した数
	unsigned int length;		// トークンを追加した数
	unsigned int useheepsize;	// メモリ使用量
};

//-----------------------------------------------------
// ツリーのノード
//-----------------------------------------------------
struct NODE{
	int id;						// 演算子とかの識別用
	char str_data[STR_MAX];		// 変数名
	int value;					// 値
	struct NODE* left;			// 左
	struct NODE* right;			// 右
};

//-----------------------------------------------------
// ノード管理用の構造体 TODO : こいつはメモリ管理用のノードプールなのでmemory_allocatorに変更
//-----------------------------------------------------
struct NODES{
	unsigned int size;
	unsigned int length;
	struct NODE* pnode;
};

struct GNT_SCRIPT_TREE
{
	int id;						// 演算子とかの識別用
	int32_t data_munit;			// データ格納用
	int32_t parent_munit;		// 親ツリー( 自分を持っているノード )
	int32_t child_munit;		// ぶら下がっているツリー( 下に掘り下げるツリー )
	int32_t element_munit;		// 自分の持ってる要素( 横並びのツリー )
};

//-----------------------------------------------------
// 変数
//-----------------------------------------------------
struct VARIABLE{
	char name[STR_MAX];			// 変数名
	int type;					// 変数の種類
	// 型にあわせてデータを作成
	union{
		// 単体の変数
		int data_int;
		float data_float;
		double data_double;
		long data_long;
		short data_short;
		char data_char;
		// 配列
		int* pdata_int;
		float* pdata_float;
		double* pdata_double;
		long* pdata_long;
		short* pdata_short;
		char* pdata_char;
	};
};

//-----------------------------------------------------
// 変数管理用の構造体
//-----------------------------------------------------
struct VARIABLES{
	unsigned int size;
	unsigned int length;
	struct VARIABLE* pvariable;
};

//-----------------------------------------------------
// 制御文のブロック(関数{},if(){},代入式,while(){},for(;;){})
//-----------------------------------------------------
struct TEXTBLOCK{
	int type;				// ブロックのタイプ(class,関数,if,while,for,do~whileなど)
	VARIABLE* value;		// ブロック内の変数(変数は{}単位で管理されてるので)
	NODES node;				// ブロック内のノードツリー
	TEXTBLOCK* p_tblock;	// ブロックの中にある構文(関数の中のさらに{}で区ブロック分けされた部分)
};

// こいつが１ファイルのコードを管理する構造体
struct CODE{
	TOKENS token;			// コード生成するためのトークン
	VARIABLE* value;		// ブロック外の定数とかグローバル変数
	TEXTBLOCK* pblock;		// {}でくくられたブロック
};

//-----------------------------------------------------
// 字句解析->構文解析+意味解析->コード生成
//-----------------------------------------------------
void gnt_read_script( GNT_MEMORY_POOL* _ppool, int32_t *p_unitid, char* filename );	// ファイル読み込みからコード生成までを行う
void FreeTokens();																		// トークンを解放
void FreeNodes();																		// ノードの開放
void TokenAnalyzer( char* filename );													// スクリプトを読み込みトークンに分解
int AddToken( TOKENS* tokens, char* tokenbuf, int* tokensize, int type );				// トークンを追加
int IsSystemWord( char* token );														// 予約文字か判定(予約文字なら0以外)
void add_val( char* name, int num );													// 変数を追加する
NODE* create_node();																	// ノードの生成
VARIABLE* create_variable( char* name, int type );										// 変数を生成
int parse_code( NODE* node, RANGE range );												// 式の解析(rangeの範囲内の式を分解する)
size_t get_opepos( RANGE range );														// 分割の基準にする演算子の位置を取得
int rem_bracket( RANGE* prange );														// 最初が()で囲まれていたら範囲を()内に縮めていく
void expr( NODE* node, STACK* pstack );													// 逆ポーランド表記(帰り順)で式を実行
void ShowTokens();																		// トークン情報を表示

#endif /*_GNT_SCRIPT_H_*/
