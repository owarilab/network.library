/*
 * Copyright (c) Katsuya Owari
 *
 * qs_datastore
 *   汎用データストア（DB）管理ライブラリ（低レベル API）。
 *
 * 既存の永続化 KVS（mmap ベース固定容量ストア: qs_create_storage_cache）を基盤に、
 *   - 1 database = 1 ディレクトリ
 *   - 1 table    = 1 .db ファイル
 *   - スキーマ   = schema.db（table 名 -> スキーマ JSON）
 *   - インデックス = index.db（index:{table}:{field}:{value} -> [id,...]）
 *   - メタ情報   = system.db（id 採番カウンタ等）
 * という構成で、ファイルパス指定で DB を作成し、スキーマ・インデックス付きで
 * レコードを CRUD できる。
 *
 * 本モジュールは core の低レベル API（qs_create_storage_cache / qs_cache_string /
 * qs_json_decode / qs_opendir 等）のみを使用し、qs_api.h の高レベル関数
 * （api_qs_*）には依存しない。
 *
 * サーバー（QS_SERVER_CONTEXT）に依存せず、CLI・サーバー両方で使用可能。
 * スクラッチメモリが必要な関数は QS_MEMORY_POOL* _ppool を第1引数に取る
 * （qs_ranking 等と同じ規約）。
 */

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_DATASTORE_H_
#define _QS_DATASTORE_H_

#include "qs_core.h"
#include "qs_memory_allocator.h"
#include "qs_variable.h"
#include "qs_hash.h"
#include "qs_array.h"
#include "qs_json.h"

/* 固定容量プロファイル。B1MB は既存 KVS との互換性のため約 980MiB の page 領域を使用する。 */
#define QS_DB_CAPACITY_B1MB    1
#define QS_DB_CAPACITY_B8MB    8
#define QS_DB_CAPACITY_B16MB   16
#define QS_DB_CAPACITY_B32MB   32
#define QS_DB_CAPACITY_B64MB   64
#define QS_DB_CAPACITY_B128MB  128
#define QS_DB_CAPACITY_B256MB  256
#define QS_DB_CAPACITY_B512MB  512
#define QS_DB_CAPACITY_B1024MB 1024

/* record list のスキャン方式 */
#define QS_DB_SCAN_FULL   0   /* フルスキャン（全レコードを走査してフィルタ） */
#define QS_DB_SCAN_INDEX  1   /* index 検索（index.db の id 配列を起点に照合） */

/* 内部で扱う特殊ファイル名 */
#define QS_DB_SYSTEM_FILE   "system.db"
#define QS_DB_SCHEMA_FILE   "schema.db"
#define QS_DB_INDEX_FILE    "index.db"

/*
 * QS_DB_STORE
 *   1 つの .db ファイル（永続化 KVS）を保持する。
 *   memory は mmap された QS_MEMORY_POOL、cache_id はその中の cache。
 */
typedef struct QS_DB_STORE
{
    QS_MEMORY_POOL* memory;
    int32_t cache_id;
    char path[4352];
    int is_open;
    int borrowed;   /* 1: ctx 側の store を借用中（close 時に解放しない） */
    struct QS_DB_CONTEXT* owner;
} QS_DB_STORE;

/*
 * QS_DB_CONTEXT
 *   database をオープンした状態を保持する。
 *   system.db / schema.db / index.db を保持する。
 *   同一 directory の open/drop は、対応する datastore 間で排他する。
 *   排他は directory の advisory flock を使用し、対応するファイルシステムが必要。
 *   同じ context の操作（open/close と table handle 使用期間を含む）は呼び出し側で直列化する。
 *   ロックを使わない外部アクセスや別 directory 内のハードリンクは保護されない。
 *   database 作成中の競合や directory の移動・差し替えは呼び出し側で防ぐこと。
 *   fork 後の子 process で継承した context を使用しないこと。
 *   context と table handle は open 中にコピー・移動しないこと。
 *   table は qs_db_open_table で必要時に QS_DB_STORE へオープンする。
 *   公開 table handle が残る間の close_database は何もせず戻る。
 *   close_database は 0=closed、1=table handle が残るため close 保留、-1=NULL context を返す。
 *   全 handle を close してから close_database を再度呼び出すこと。
 */
typedef struct QS_DB_CONTEXT
{
    char base_dir[4096];
    char database[256];
    char db_path[4352];          /* base_dir/database */

    QS_DB_STORE system;          /* system.db */
    QS_DB_STORE schema;          /* schema.db */
    QS_DB_STORE index;           /* index.db */
    size_t open_table_count;
    void* lock_dir;
    int is_open;
} QS_DB_CONTEXT;

/*
 * Database
 */
/* 作成途中の失敗時は生成物を削除する。削除に失敗した場合は一部が残ることがある。 */
int qs_db_create_database(const char* base_dir, const char* database);
/* 許可された名前の .db 通常ファイル以外があれば削除前に失敗する。削除中の失敗は部分削除になり得る。 */
int qs_db_drop_database(const char* base_dir, const char* database);
/* base_dir 直下の database（system.db を持つディレクトリ）名を _ppool 上の配列に格納する。末尾 '/' は許可。 */
int32_t qs_db_show_database(QS_MEMORY_POOL* _ppool, const char* base_dir);
/* database/table 名は ASCII 英数字、'_'、'-' のみ・1〜45 byte。使用中 context の再 open は拒否。 */
int qs_db_open_database(QS_DB_CONTEXT* ctx, const char* base_dir, const char* database);
/* 0=closed, 1=table handle が残るため close 保留, -1=NULL context。 */
int qs_db_close_database(QS_DB_CONTEXT* ctx);

/*
 * Table
 *   capacity_mb は QS_DB_CAPACITY_B*MB のいずれか。
 *   作成時の容量は system.db に保存され、record 操作・open_table は保存値を使う。
 *   容量 metadata が欠落・不正、またはファイル長・主要ヘッダーが不一致の場合は open に失敗する。
 *   system、schema、index は管理ストアの予約名であり、create/drop できない。
 *   これらへの open_table と record 操作は可能だが、管理データの変更に注意すること。
 */
int qs_db_create_table(QS_DB_CONTEXT* ctx, const char* table, int capacity_mb);
/* table は ASCII の英数字、_、- のみ。open table がある場合は先に close すること。
 * 削除は非原子的で、失敗時にファイルまたは metadata の一部が削除済みの場合がある。 */
int qs_db_drop_table(QS_DB_CONTEXT* ctx, const char* table);
/* db_path 配下の table（*.db、system/schema/index を除く）名を _ppool 上の配列に格納し、配列 munit を返す。 */
int32_t qs_db_show_table(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx);
/* capacity_mb は互換性のため残るが、table 作成時に system.db へ記録した値を使用する。
 * 失敗時 out_store は zero state（memory=NULL, is_open=0, owner=NULL）になる。 */
int qs_db_open_table(QS_DB_CONTEXT* ctx, const char* table, int capacity_mb, QS_DB_STORE* out_store);
void qs_db_close_table(QS_DB_STORE* store);

/*
 * Schema
 *   schema_define は JSON を解釈せず、そのまま保存する。形式・fields 配列の確認は schema_validate 時に行う。
 *   schema_validate が保証する field type は int32/int64/string、required と string maxLength。
 *   未知の field、重複 field、default、配列などは検証/適用しない。
 *   例: {"fields":[{"name":"id","type":"string","maxLength":64,"required":0,"unsettable":0}, ...]}
 */
int qs_db_schema_define(QS_DB_CONTEXT* ctx, const char* table, const char* schema_json);
/* スキーマ JSON を out_buf にコピーして返す。0=成功, -1=未定義/バッファ不足。 */
int qs_db_schema_get(QS_DB_CONTEXT* ctx, const char* table, char* out_buf, size_t out_size);
/* レコード JSON を明示的に検証する。CRUD はこの関数を自動呼び出ししない。0=適合, -1=違反。 */
int qs_db_schema_validate(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx, const char* table, const char* record_json);

/*
 * Record
 *   レコードは table.db に key=id, value=JSON として保存する。
 *   table は内部でオープン/クローズする。
 *   schema 検証と index 更新は自動では行わず、呼び出し側で実行する。
 */
int qs_db_record_insert(QS_DB_CONTEXT* ctx, const char* table, const char* id, const char* record_json);
int qs_db_record_update(QS_DB_CONTEXT* ctx, const char* table, const char* id, const char* record_json);
int qs_db_record_delete(QS_DB_CONTEXT* ctx, const char* table, const char* id);
/* レコード JSON を out_buf にコピーして返す。0=成功, -1=未存在/バッファ不足。 */
int qs_db_record_get(QS_DB_CONTEXT* ctx, const char* table, const char* id, char* out_buf, size_t out_size);
/*
 * record_list
 *   filter_json: フィルタ条件の JSON オブジェクト（例: {"status":"open","assignee":"u1"}）。
 *                NULL または {} の場合は全件。
 *   scan_mode:   QS_DB_SCAN_FULL / QS_DB_SCAN_INDEX。
 *                INDEX では全条件値が文字列で、各 index キーが 63 byte 以下であること。
 *                index の構築・CRUD 後の更新は呼び出し側の責任。未構築・更新漏れでは
 *                結果が欠落し得る。index 不在による空結果と該当なしは区別されない。
 *   _ppool:      戻り値 JSON 文字列を保持するスクラッチメモリ。
 *   戻り値:      結果 JSON 配列文字列（_ppool 内を指す）。0 件は "[]"、不正条件・エラーや
 *                走査対象の不正なレコードは NULL。INDEX では候補以外は検査しない。
 *                結果は終端 NUL を含め 64KB 以下。内部作業メモリ不足でも NULL を返す。
 */
char* qs_db_record_list(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx, const char* table, const char* filter_json, int scan_mode);

/*
 * Index
 *   index.db に key=index:{table}:{field}:{value}, value=[id,...] を保存する。
 *   既存 index の配列デコードに失敗した場合、add/remove は値を変更せず -1 を返す。
 *   index ID 配列のエンコード結果が 64KB に収まらない場合も -1 を返す。
 */
int qs_db_index_add(QS_DB_CONTEXT* ctx, const char* table, const char* field, const char* value, const char* record_id);
int qs_db_index_remove(QS_DB_CONTEXT* ctx, const char* table, const char* field, const char* value, const char* record_id);
/* table 全件から index を再構築する。失敗時は部分的な index が残り得るため、検索に使用せず再構築すること。 */
int qs_db_index_rebuild(QS_MEMORY_POOL* _ppool, QS_DB_CONTEXT* ctx, const char* table, const char* indexed_fields_json);

/*
 * System（id 採番カウンタ）
 *   counter:{table} を system.db に保持し、prefix + 連番 の id を生成する。
 *   prefix は空でない文字列とし、出力は 63 byte 以下。失敗時は counter を進めず out_id は変更しない。
 *   同じ context の同時操作は呼び出し側で直列化すること。
 */
int qs_db_next_id(QS_DB_CONTEXT* ctx, const char* table, const char* prefix, char* out_id, size_t out_size);

#endif /*_QS_DATASTORE_H_*/

#ifdef __cplusplus
}
#endif
