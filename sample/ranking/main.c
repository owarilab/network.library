/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_api.h"
#include <stddef.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	QS_MEMORY_CONTEXT memory = {0};
	QS_MEMORY_CONTEXT dest_memory = {0};
	QS_RANKING_CONTEXT ranking = {0};

	if( api_qs_memory_alloc( &memory, 1024 * 1024 * 8 ) != 0 ){
		return -1;
	}
	if( api_qs_memory_alloc( &dest_memory, 1024 * 1024 * 2 ) != 0 ){
		api_qs_memory_free( &memory );
		return -1;
	}
	if( api_qs_ranking_create( &memory, &ranking, 100, 32, 10, 10 ) != 0 ){
		printf("Error: Failed to create ranking\n");
		api_qs_memory_free( &memory );
		api_qs_memory_free( &dest_memory );
		return -1;
	}
	
	// ユーザーをエントリしてスコアを設定
	const char* users[] = {"player1", "player2", "player3", "player4", "player5"};
	uint32_t scores[] = {100, 250, 180, 320, 95};
	
	for(int i = 0; i < 5; i++){
		if( api_qs_ranking_entry( &ranking, users[i] ) == 0 ){
			api_qs_ranking_set_value( &ranking, users[i], scores[i] );
		}
	}
	
	api_qs_ranking_sort( &ranking );
	
	char* json_str = api_qs_ranking_get( &ranking, 0, 5, &dest_memory );
	if( json_str != NULL ){
		printf("Ranking Result:\n%s\n", json_str);
	}
	
	api_qs_memory_free( &memory );
	api_qs_memory_free( &dest_memory );
	return 0;
}
