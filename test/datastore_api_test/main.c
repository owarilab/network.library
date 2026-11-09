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

/* Index-scan list helper. The returned JSON is allocated in the scratch
 * pool, so it is not freed here. record_list output contains record JSON
 * only (no record id), so match on a unique field value. */
static int index_list_contains(QS_MEMORY_CONTEXT* scratch, QS_DB_API_CONTEXT* db, const char* table, const char* filter, const char* expect)
{
	char* records = api_qs_db_record_list(scratch, db, table, filter, QS_DB_SCAN_INDEX);
	return (records != NULL && strstr(records, expect) != NULL) ? 1 : 0;
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
	check(api_qs_db_schema_define(&db, "items", "{\"fields\":[{\"name\":\"name\",\"type\":\"string\",\"maxLength\":32,\"required\":1},{\"name\":\"count\",\"type\":\"int32\",\"required\":1,\"default\":7},{\"name\":\"status\",\"type\":\"string\",\"default\":\"new\"},{\"name\":\"note\",\"type\":\"string\",\"default\":\"say \\\"hi\\\"\\n\"}]}" ) == 0, "define schema with defaults");
	check(api_qs_db_schema_validate(&scratch, &db, "items", "{\"name\":\"sample\",\"count\":7}") == 0, "validate record schema after applying required default");
	check(api_qs_db_table_close(&db, "items") == 0, "close opaque table handle before record operations");
	check(api_qs_db_record_insert(&db, "items", "i1", "{\"name\":\"sample\"}") == 0, "insert record applies schema defaults");
	check(api_qs_db_record_get(&db, "items", "i1", buffer, sizeof(buffer)) == 0, "get record");
	check(strstr(buffer, "\"name\":\"sample\"") != NULL && strstr(buffer, "\"count\":7") != NULL && strstr(buffer, "\"status\":\"new\"") != NULL && strstr(buffer, "say \\\"hi\\\"\\n") != NULL, "inserted record contains defaults");
	check(api_qs_db_record_insert(&db, "items", "i2", "{\"name\":\"explicit\",\"count\":9,\"status\":\"ready\"}") == 0, "insert accepts explicitly supplied schema values");
	check(api_qs_db_record_get(&db, "items", "i2", buffer, sizeof(buffer)) == 0 && strstr(buffer, "\"count\":9") != NULL && strstr(buffer, "\"status\":\"ready\"") != NULL, "explicit values override defaults");
	check(api_qs_db_record_insert(&db, "items", "bad", "{\"name\":\"wrong\",\"count\":\"seven\"}") == -1, "insert rejects schema type mismatch");
	check(api_qs_db_record_insert(&db, "items", "missing", "{\"count\":7}") == -1, "insert rejects missing required field");
	check(api_qs_db_record_update(&db, "items", "i1", "{\"name\":\"updated\"}") == 0, "update applies schema defaults");
	check(api_qs_db_record_get(&db, "items", "i1", buffer, sizeof(buffer)) == 0 && strstr(buffer, "\"count\":7") != NULL && strstr(buffer, "\"status\":\"new\"") != NULL, "updated record contains defaults");
	check(api_qs_db_record_update(&db, "items", "i1", "{\"name\":\"invalid\",\"count\":\"seven\"}") == -1, "update rejects schema type mismatch");
	check(api_qs_db_record_get(&db, "items", "i1", buffer, sizeof(buffer)) == 0 && strstr(buffer, "\"name\":\"updated\"") != NULL, "failed update preserves prior record");
	check(api_qs_db_index_rebuild(&scratch, &db, "items", "[\"name\"]") == 0, "rebuild index");
	check(api_qs_db_index_add(&db, "items", "name", "updated", "i1") == 0, "add index entry");
	char* records = api_qs_db_record_list(&scratch, &db, "items", "{\"name\":\"updated\"}", QS_DB_SCAN_INDEX);
	check(records != NULL && strstr(records, "updated") != NULL, "indexed record list");
	check(api_qs_db_next_id(&db, "items", "i", generated_id, sizeof(generated_id)) == 0, "generate next id");
	check(0 == strcmp(generated_id, "i1"), "generated id matches");
	check(api_qs_db_index_remove(&db, "items", "name", "updated", "i1") == 0, "remove index entry");
	check(api_qs_db_record_delete(&db, "items", "i1") == 0, "delete record");
	check(api_qs_db_record_delete(&db, "items", "i2") == 0, "delete explicitly populated record");
	check(api_qs_db_create_table(&db, "legacy", 1) == 0, "create table without schema");
	check(api_qs_db_record_insert(&db, "legacy", "l1", "{\"untyped\":true}") == 0, "schema-less insert remains compatible");
	check(api_qs_db_record_update(&db, "legacy", "l1", "{\"untyped\":null}") == 0, "schema-less update remains compatible");
	check(api_qs_db_drop_table(&db, "legacy") == 0, "drop schema-less table");
	check(api_qs_db_table_open(&db, "items") == 0, "reopen table handle");
	check(api_qs_db_close_database(&db) == 1, "database close is deferred while table handle remains");
	check(api_qs_db_free(&db) == 1, "context free retains allocator while table handle remains");
	check(api_qs_db_table_close(&db, "items") == 0, "close remaining table handle");
	check(api_qs_db_drop_table(&db, "items") == 0, "drop table");
	check(api_qs_db_close_database(&db) == 0, "close database");
	check(api_qs_db_free(&db) == 0, "free database context");
	check(api_qs_db_open_database(&db, base_dir, "app") == -1, "open rejects freed context");
	check(api_qs_memory_free(&scratch) == 0, "scratch memory free succeeds");

	/* automatic index update */
	check(api_qs_db_init(&db) == 0, "auto-index init succeeds");
	check(api_qs_memory_alloc(&scratch, 1024 * 1024) == 0, "scratch memory re-alloc succeeds");
	check(api_qs_db_open_database(&db, base_dir, "app") == 0, "auto-index open database");
	check(api_qs_db_set_auto_index_update(NULL, 1) == -1, "set auto-index rejects NULL context");
	check(api_qs_db_set_auto_index_update(&db, 1) == 0, "enable auto-index update");
	check(api_qs_db_create_table(&db, "autoitems", 1) == 0, "create auto-index table");
	check(api_qs_db_schema_define(&db, "autoitems", "{\"fields\":[{\"name\":\"name\",\"type\":\"string\",\"maxLength\":32,\"required\":1,\"is_index\":1},{\"name\":\"count\",\"type\":\"int32\",\"required\":1,\"default\":0,\"is_index\":1},{\"name\":\"status\",\"type\":\"string\",\"default\":\"new\"}]}" ) == 0, "define auto-index schema");
	check(api_qs_db_record_insert(&db, "autoitems", "a1", "{\"name\":\"alpha\",\"count\":1}") == 0, "auto-index insert succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"alpha\"}", "alpha") == 1, "auto-index insert created string index entry");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "alpha") == 1, "auto-index insert created int index entry");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"status\":\"new\"}", "alpha") == 0, "auto-index insert skipped field without is_index");
	check(api_qs_db_record_insert(&db, "autoitems", "a2", "{\"name\":\"beta\",\"count\":1,\"status\":\"old\"}") == 0, "auto-index insert second record");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "alpha") == 1 && index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "beta") == 1, "auto-index list shows both records for same int value");
	check(api_qs_db_record_update(&db, "autoitems", "a1", "{\"name\":\"zulu\",\"count\":2,\"status\":\"new\"}") == 0, "auto-index update succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"alpha\"}", "alpha") == 0, "auto-index update removed old string index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"zulu\"}", "zulu") == 1, "auto-index update added new string index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "alpha") == 0, "auto-index update removed old int index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "beta") == 1, "auto-index update kept unchanged record index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"2\"}", "zulu") == 1, "auto-index update added new int index");
	check(api_qs_db_record_update(&db, "autoitems", "a2", "{\"name\":\"beta\",\"count\":1,\"status\":\"changed\"}") == 0, "auto-index update without index change succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "beta") == 1, "auto-index update without change keeps index");
	check(api_qs_db_record_delete(&db, "autoitems", "a2") == 0, "auto-index delete succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"beta\"}", "beta") == 0, "auto-index delete removed string index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"1\"}", "beta") == 0, "auto-index delete removed int index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"2\"}", "zulu") == 1, "auto-index delete kept other record index");
	check(api_qs_db_set_auto_index_update(&db, 0) == 0, "disable auto-index again");
	check(api_qs_db_record_insert(&db, "autoitems", "a3", "{\"name\":\"gamma\",\"count\":3}") == 0, "insert with auto-index disabled succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"gamma\"}", "gamma") == 0, "disabled auto-index does not create index");
	check(api_qs_db_record_update(&db, "autoitems", "a1", "{\"name\":\"eta\",\"count\":7,\"status\":\"new\"}") == 0, "update with auto-index disabled succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"zulu\"}", "eta") == 0, "disabled update leaves query results correct");
	check(api_qs_db_record_delete(&db, "autoitems", "a1") == 0, "delete with auto-index disabled succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"2\"}", "eta") == 0, "disabled delete leaves query results correct");
	check(api_qs_db_index_rebuild(&scratch, &db, "autoitems", "[\"name\",\"count\"]") == 0, "rebuild auto-index table");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"zulu\"}", "eta") == 0, "rebuild removed stale string index");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"gamma\"}", "gamma") == 1, "rebuild indexed existing record string field");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"3\"}", "gamma") == 1, "rebuild indexed existing record int field");
	check(api_qs_db_set_auto_index_update(&db, 1) == 0, "re-enable auto-index");
	check(api_qs_db_record_update(&db, "autoitems", "a3", "{\"name\":\"gamma2\",\"count\":99,\"status\":\"new\"}") == 0, "auto-index update after enable succeeds");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"gamma2\"}", "gamma2") == 1, "re-enabled auto-index created index on update");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"count\":\"99\"}", "gamma2") == 1, "re-enabled auto-index created int index on update");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"gamma\"}", "gamma2") == 0, "re-enabled auto-index removed old index on update");
	check(api_qs_db_record_delete(&db, "autoitems", "a3") == 0, "auto-index delete final record");
	check(index_list_contains(&scratch, &db, "autoitems", "{\"name\":\"gamma2\"}", "gamma2") == 0, "auto-index delete removed final index");
	/* int edge cases: negative int32-range values and large int64 values */
	check(api_qs_db_create_table(&db, "autoints", 1) == 0, "create int edge table");
	check(api_qs_db_schema_define(&db, "autoints", "{\"fields\":[{\"name\":\"name\",\"type\":\"string\",\"maxLength\":32,\"required\":1,\"is_index\":1},{\"name\":\"score\",\"type\":\"int64\",\"required\":1,\"is_index\":1}]}" ) == 0, "define int64 auto-index schema");
	check(api_qs_db_record_insert(&db, "autoints", "n1", "{\"name\":\"neg\",\"score\":-5}") == 0, "insert negative int succeeds");
	check(index_list_contains(&scratch, &db, "autoints", "{\"score\":\"-5\"}", "neg") == 1, "negative int index entry found");
	check(api_qs_db_record_insert(&db, "autoints", "b1", "{\"name\":\"big\",\"score\":9999999999}") == 0, "insert large int64 succeeds");
	check(index_list_contains(&scratch, &db, "autoints", "{\"score\":\"9999999999\"}", "big") == 1, "large int64 index entry found");
	check(index_list_contains(&scratch, &db, "autoints", "{\"score\":\"-5\"}", "big") == 0, "distinct int values do not cross-match");
	check(api_qs_db_record_insert(&db, "autoints", "o1", "{\"name\":\"one\",\"score\":1}") == 0, "insert small positive int succeeds");
	{
		char* non_canon = api_qs_db_record_list(&scratch, &db, "autoints", "{\"score\":\"01\"}", QS_DB_SCAN_FULL);
		check(non_canon == NULL || strstr(non_canon, "one") == NULL, "non-canonical int filter does not match");
	}
	check(api_qs_db_record_update(&db, "autoints", "n1", "{\"name\":\"neg2\",\"score\":7}") == 0, "update negative int field succeeds");
	check(index_list_contains(&scratch, &db, "autoints", "{\"score\":\"-5\"}", "neg2") == 0, "update removed negative int index");
	check(index_list_contains(&scratch, &db, "autoints", "{\"score\":\"7\"}", "neg2") == 1, "update added new int index");
	check(api_qs_db_record_delete(&db, "autoints", "b1") == 0, "delete large int64 record");
	check(index_list_contains(&scratch, &db, "autoints", "{\"score\":\"9999999999\"}", "big") == 0, "delete removed large int64 index");
	check(api_qs_db_drop_table(&db, "autoints") == 0, "drop int edge table");
	check(api_qs_db_drop_table(&db, "autoitems") == 0, "drop auto-index table");
	check(api_qs_db_close_database(&db) == 0, "auto-index close database");
	check(api_qs_db_free(&db) == 0, "auto-index free context");
	check(api_qs_memory_free(&scratch) == 0, "scratch memory free succeeds (auto-index)");
	check(api_qs_db_drop_database(base_dir, "app") == 0, "drop database");
	check(rmdir(base_dir) == 0, "remove temporary base directory");

	printf("\n=== datastore_api_test summary ===\nfail: %d\n", failures);
	return failures == 0 ? 0 : 1;
}
