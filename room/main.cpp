#include<stdio.h>
#include<stdint.h>
#include<map>
#include<vector>
#include<string>

using namespace std;

#define BOSS_SIZE 3

class BattleRoom
{
	private:
		typedef struct Player{
			uint32_t damage;
		} Player;
		typedef struct Enemy{
			uint32_t hp;
			string id;
			uint8_t status;
		} Enemy;
		map<int32_t,Player> player;
		vector<Enemy> boss;
	public:
		BattleRoom(){
			player.clear();
			Enemy e = { 0, "", 0 };
			for( int i = 0; i < BOSS_SIZE; i++ ){
				boss.push_back( e );
			}
		}

		int Damage( uint8_t enemy_num, int32_t player_id, uint32_t damage )
		{
			if( player.find( player_id ) == player.end() ){
				Player p = { 0 };
				player[player_id] = p;
			}
			if( enemy_num < 0 || enemy_num >= BOSS_SIZE ){
				return -1;
			}
			if( damage >= boss[enemy_num].hp ){
				player[player_id].damage += boss[enemy_num].hp;
				boss[enemy_num].hp = 0;
			}
			else{
				player[player_id].damage += damage;
				boss[enemy_num].hp -= damage;
			}
			return 0;
		}

		void InitBoss( uint8_t enemy_num, string id, uint32_t hp, uint8_t status )
		{
			if( enemy_num < 0 || enemy_num >= BOSS_SIZE ){
				return;
			}
			boss[enemy_num].id = id;
			boss[enemy_num].hp = hp;
			boss[enemy_num].status = status;
		}

		string ToJson(){
			string json = "";
			json += "{\"boss\":";
			json += "[";
			for( int i = 0; i < BOSS_SIZE; i++ ){
				if( i > 0 ){ json += ","; }
				json += "{\"id\":\"" + boss[i].id + "\",\"hp\":" + to_string( boss[i].hp ) + ",\"status\":" + to_string( boss[i].status ) + "}";
			}
			json += "],";
			json += "\"player\":";
			json += "[";
			for( map<int,Player>::iterator it = player.begin(); it != player.end(); ++it ){
				if( it != player.begin() ){ json += ","; }
				json += "{\"id\":"+ to_string( it->first ) +",\"damage\":" + to_string( it->second.damage ) + "}";
			}
			json += "]";
			json += "}";
			return json;
		}

		~BattleRoom(){
			player.clear();
			boss.clear();
		}
};

static map<uint32_t,BattleRoom> room;

static void new_room( int32_t room_id )
{
	if( room.find( room_id ) == room.end() ){
		BattleRoom r;
		room[room_id] = r;
	}
}

int main( void )
{
	room.clear();
	int32_t room_id = 32;
	new_room( room_id );
	room[room_id].InitBoss( 0, "ENC00001", 2000, 0 );
	room[room_id].InitBoss( 1, "ENC00002", 4000, 0 );
	room[room_id].InitBoss( 2, "ENC00003", 6000, 0 );
	room[room_id].Damage( 0, 3, 20 );
	printf( "%s\n", room[room_id].ToJson().c_str() );
	return 0;
}
