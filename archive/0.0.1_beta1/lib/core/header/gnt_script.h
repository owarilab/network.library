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

#define STRBUF_SIZE				1024	// �g�[�N����͗p�����o�b�t�@(������̑傫�������Ē���)
#define TOKEN_INIT_LENGTH		102400	// �����g�[�N���̗̈�m�ۃT�C�Y(�ŏ��Ɋm�ۂ���鐔���킩���Ă���΍���)
#define TOKEN_ADD_LENGTH		102400	// �m�ۂ�������𒴂����Ƃ��ɍĊm�ۂ��鐔(�m�ۂ���񐔂����Ȃ��ق�������)
#define ID_UNK					0		// �s��
#define ID_NUM					1		// ���l
#define ID_OP					2		// ���Z�q
#define ID_SIGN					3		// �L��
#define ID_STR					4		// ������
#define ID_VAL					5		// �ϐ�
#define ID_SYS					6		// �\�񕶎�
#define STR_MAX					0x100	// �ő啶����(����͓��I�Ƀ������m�ۂł���悤�ɂ��Ȃ���΁E�E�E)
#define MAX_STACK				128		// �v�Z�p�X�^�b�N�̍ő吔

#define OPE_PRIORITY_LOWEST		9		// ���Z�q�̍Œ�v���C�I���e�B
#define OPE_END_PRIORITY		1		// ���[�L���̃v���C�I���e�B
#define OPE_EQU_PRIORITY		2		// ������Z�q�Ȃǂ̃v���C�I���e�B
#define OPE_CAST_PRIORITY		3		// �^�̃L���X�g�̃v���C�I���e�B
#define OPE_COMP_PRIORITY		4		// ��r���Z�q�Ȃǂ̃v���C�I���e�B
#define OPE_ADDSUB_PRIORITY		5		// �������Z�̃v���C�I���e�B
#define OPE_MULDIV_PRIORITY		6		// �揜�]���Z�̃v���C�I���e�B
#define OPE_SINGLE_PRIORITY		7		// �P�����Z�q�̃v���C�I���e�B
#define OPE_SYSVAL_PRIORITY		8		// �ϐ���`�̃v���C�I���e�B
#define ADDNODE					102400	// �m�[�h�̃������m�ۂ��鐔
#define ADDVARIABLE				64		// �ϐ��̃������m�ۂ��鐔

// �V�X�e���̗\���
#define SYS_INT					"int"	// �����^
#define SYS_IF					"if"	// if��
#define SYS_ELSE				"else"	// else��

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
// �v�Z�Ȃǂ����s����Ƃ��Ɏg���X�^�b�N
//-----------------------------------------------------
struct STACKDATA{
	int id;
	int array_index;	// �z��Q�Ɨp
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
// �X�^�b�N�Ǘ��p�̍\����
//-----------------------------------------------------
struct STACK{
	STACKDATA sdata[MAX_STACK];
	int endpos;
};

//-----------------------------------------------------
// �͈͎w��p�̍\����
//-----------------------------------------------------
struct RANGE{
	int start;
	int end;
};

//-----------------------------------------------------
// �g�[�N���\����
//-----------------------------------------------------
struct TOKEN{
	char* token;				// ������
	int type;					// �g�[�N���̎��
	unsigned int size;			// �����T�C�Y
};

//-----------------------------------------------------
// �g�[�N���Ǘ��\����
//-----------------------------------------------------
struct TOKENS{
	struct TOKEN* ptoken;		// �g�[�N��
	unsigned int size;			// �������m�ۂ�����
	unsigned int length;		// �g�[�N����ǉ�������
	unsigned int useheepsize;	// �������g�p��
};

//-----------------------------------------------------
// �c���[�̃m�[�h
//-----------------------------------------------------
struct NODE{
	int id;						// ���Z�q�Ƃ��̎��ʗp
	char str_data[STR_MAX];		// �ϐ���
	int value;					// �l
	struct NODE* left;			// ��
	struct NODE* right;			// �E
};

//-----------------------------------------------------
// �m�[�h�Ǘ��p�̍\���� TODO : �����̓������Ǘ��p�̃m�[�h�v�[���Ȃ̂�memory_allocator�ɕύX
//-----------------------------------------------------
struct NODES{
	unsigned int size;
	unsigned int length;
	struct NODE* pnode;
};

struct GNT_SCRIPT_TREE
{
	int id;						// ���Z�q�Ƃ��̎��ʗp
	int32_t data_munit;			// �f�[�^�i�[�p
	int32_t parent_munit;		// �e�c���[( �����������Ă���m�[�h )
	int32_t child_munit;		// �Ԃ牺�����Ă���c���[( ���Ɍ@�艺����c���[ )
	int32_t element_munit;		// �����̎����Ă�v�f( �����т̃c���[ )
};

//-----------------------------------------------------
// �ϐ�
//-----------------------------------------------------
struct VARIABLE{
	char name[STR_MAX];			// �ϐ���
	int type;					// �ϐ��̎��
	// �^�ɂ��킹�ăf�[�^���쐬
	union{
		// �P�̂̕ϐ�
		int data_int;
		float data_float;
		double data_double;
		long data_long;
		short data_short;
		char data_char;
		// �z��
		int* pdata_int;
		float* pdata_float;
		double* pdata_double;
		long* pdata_long;
		short* pdata_short;
		char* pdata_char;
	};
};

//-----------------------------------------------------
// �ϐ��Ǘ��p�̍\����
//-----------------------------------------------------
struct VARIABLES{
	unsigned int size;
	unsigned int length;
	struct VARIABLE* pvariable;
};

//-----------------------------------------------------
// ���䕶�̃u���b�N(�֐�{},if(){},�����,while(){},for(;;){})
//-----------------------------------------------------
struct TEXTBLOCK{
	int type;				// �u���b�N�̃^�C�v(class,�֐�,if,while,for,do~while�Ȃ�)
	VARIABLE* value;		// �u���b�N���̕ϐ�(�ϐ���{}�P�ʂŊǗ�����Ă�̂�)
	NODES node;				// �u���b�N���̃m�[�h�c���[
	TEXTBLOCK* p_tblock;	// �u���b�N�̒��ɂ���\��(�֐��̒��̂����{}�ŋ�u���b�N�������ꂽ����)
};

// �������P�t�@�C���̃R�[�h���Ǘ�����\����
struct CODE{
	TOKENS token;			// �R�[�h�������邽�߂̃g�[�N��
	VARIABLE* value;		// �u���b�N�O�̒萔�Ƃ��O���[�o���ϐ�
	TEXTBLOCK* pblock;		// {}�ł�����ꂽ�u���b�N
};

//-----------------------------------------------------
// ������->�\�����+�Ӗ����->�R�[�h����
//-----------------------------------------------------
void gnt_read_script( GNT_MEMORY_POOL* _ppool, int32_t *p_unitid, char* filename );	// �t�@�C���ǂݍ��݂���R�[�h�����܂ł��s��
void FreeTokens();																		// �g�[�N�������
void FreeNodes();																		// �m�[�h�̊J��
void TokenAnalyzer( char* filename );													// �X�N���v�g��ǂݍ��݃g�[�N���ɕ���
int AddToken( TOKENS* tokens, char* tokenbuf, int* tokensize, int type );				// �g�[�N����ǉ�
int IsSystemWord( char* token );														// �\�񕶎�������(�\�񕶎��Ȃ�0�ȊO)
void add_val( char* name, int num );													// �ϐ���ǉ�����
NODE* create_node();																	// �m�[�h�̐���
VARIABLE* create_variable( char* name, int type );										// �ϐ��𐶐�
int parse_code( NODE* node, RANGE range );												// ���̉��(range�͈͓̔��̎��𕪉�����)
size_t get_opepos( RANGE range );														// �����̊�ɂ��鉉�Z�q�̈ʒu���擾
int rem_bracket( RANGE* prange );														// �ŏ���()�ň͂܂�Ă�����͈͂�()���ɏk�߂Ă���
void expr( NODE* node, STACK* pstack );													// �t�|�[�����h�\�L(�A�菇)�Ŏ������s
void ShowTokens();																		// �g�[�N������\��

#endif /*_GNT_SCRIPT_H_*/
