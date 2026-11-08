#include "qs_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int failures;

static void check(int condition, const char* name)
{
	if(condition){
		printf("[PASS] %s\n", name);
	}else{
		printf("[FAIL] %s\n", name);
		failures++;
	}
}

int main(void)
{
	char base_dir[] = "/tmp/qs_datastore_api_test_XXXXXX";
	if(NULL == mkdtemp(base_dir)){
		perror("mkdtemp");
		return 1;
	}

	QS_DB_API_CONTEXT db = {0};
	QS_MEMORY_CONTEXT scratch = {0};
	QS_JSON_ELEMENT_ARRAY databases = {-1, NULL};
	QS_JSON_ELEMENT_ARRAY tables = {-1, NULL};
	char buffer[1024];
	char generated_id[64];
	check(api_qs_db_init(NULL) == -1, "init rejects NULL context");
	check(api_qs_db_init(&db) == 0, "init succeeds");
	check(api_qs_db_init(&db) == -1, "double init is rejected");
	check(api_qs_memory_alloc(&scratch, 1024 * 1024) == 0, "scratch memory alloc succeeds");
	check(api_qs_db_create_database(base_dir, "app") == 0, "create database");
	check(api_qs_db_open_database(&db, base_dir, "app") == 0, "open database");
	check(api_qs_db_show_database(&scratch, &databases, base_dir) == 0, "show database");
	check(api_qs_array_get_length(&databases) == 1, "show database returns one entry");
	check(0 == strcmp(api_qs_array_get_string(&databases, 0), "app"), "show database entry matches");
	check(api_qs_db_create_table(&db, "items", 1) == 0, "create table");
	check(api_qs_db_show_table(&scratch, &db, &tables) == 0, "show table");
	check(api_qs_array_get_length(&tables) == 1, "show table returns one entry");
	check(api_qs_db_table_open(&db, "items") == 0, "open table with opaque handle");
	check(api_qs_db_schema_define(&db, "items", "{\"fields\":[{\"name\":\"name\",\"type\":\"string\",\"maxLength\":32,\"required\":1}]}" ) == 0, "define schema");
	check(api_qs_db_schema_validate(&scratch, &db, "items", "{\"name\":\"sample\"}") == 0, "validate record schema");
	check(api_qs_db_table_close(&db, "items") == 0, "close opaque table handle before record operations");
	check(api_qs_db_record_insert(&db, "items", "i1", "{\"name\":\"sample\"}") == 0, "insert record");
	check(api_qs_db_record_get(&db, "items", "i1", buffer, sizeof(buffer)) == 0, "get record");
	check(0 == strcmp(buffer, "{\"name\":\"sample\"}"), "record JSON matches");
	check(api_qs_db_index_rebuild(&scratch, &db, "items", "[\"name\"]") == 0, "rebuild index");
	check(api_qs_db_index_add(&db, "items", "name", "sample", "i1") == 0, "add index entry");
	char* records = api_qs_db_record_list(&scratch, &db, "items", "{\"name\":\"sample\"}", QS_DB_SCAN_INDEX);
	check(records != NULL && strstr(records, "sample") != NULL, "indexed record list");
	check(api_qs_db_next_id(&db, "items", "i", generated_id, sizeof(generated_id)) == 0, "generate next id");
	check(0 == strcmp(generated_id, "i1"), "generated id matches");
	check(api_qs_db_index_remove(&db, "items", "name", "sample", "i1") == 0, "remove index entry");
	check(api_qs_db_record_update(&db, "items", "i1", "{\"name\":\"updated\"}") == 0, "update record");
	check(api_qs_db_record_delete(&db, "items", "i1") == 0, "delete record");
	check(api_qs_db_table_open(&db, "items") == 0, "reopen table handle");
	check(api_qs_db_close_database(&db) == 1, "database close is deferred while table handle remains");
	check(api_qs_db_free(&db) == 1, "context free retains allocator while table handle remains");
	check(api_qs_db_table_close(&db, "items") == 0, "close remaining table handle");
	check(api_qs_db_drop_table(&db, "items") == 0, "drop table");
	check(api_qs_db_close_database(&db) == 0, "close database");
	check(api_qs_db_free(&db) == 0, "free database context");
	check(api_qs_db_open_database(&db, base_dir, "app") == -1, "open rejects freed context");
	check(api_qs_memory_free(&scratch) == 0, "scratch memory free succeeds");
	check(api_qs_db_drop_database(base_dir, "app") == 0, "drop database");
	check(rmdir(base_dir) == 0, "remove temporary base directory");

	printf("\n=== datastore_api_test summary ===\nfail: %d\n", failures);
	return failures == 0 ? 0 : 1;
}
