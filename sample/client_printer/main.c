/*
 * Copyright (c) Katsuya Owari
 */

#include "qs_socket.h"
int on_connect(QS_SERVER_CONNECTION_INFO* connection);
void* on_pkt_recv( void* args );
int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info );
int on_close(QS_SERVER_CONNECTION_INFO* connection);
int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info);

int main( int argc, char *argv[], char *envp[] )
{
	QS_SOCKET_OPTION* tcp_client = qs_create_tcp_client_plain("192.168.11.15", "9100");
	set_on_connect_event(tcp_client,on_connect);
	//set_on_payload_recv_event(tcp_client,on_recv);
	set_on_plain_recv_event(tcp_client, on_pkt_recv );
	set_on_close_event(tcp_client,on_close);
	qs_socket(tcp_client);
	
	//QS_SOCKET_OPTION*  udp_client = qs_create_udp_client("localhost", "52001");
	//set_on_payload_recv_event(udp_client, on_udp_client_payload_recv);
	//qs_socket(udp_client);
	
	while(1){
		qs_client_update(tcp_client);
		qs_sleep(100);

		//qs_client_update(udp_client);
		//qs_sleep(100000);
		//qs_client_send_message(0x01,"test1",4,tcp_client);
		//qs_sleep(100000);
		//qs_client_send_message(0x02,"test2",4,udp_client);
	}
	qs_free_socket(tcp_client);
	qs_free(tcp_client->memory_pool);
	//qs_free_socket(udp_client);
	//qs_free(udp_client->memory_pool);
	return 0;
}

void command_set_mode(char* msg, int* len, int mode)
{
    // 16 進: 1B 69 61 0 = コマンドモード切替 (ESC i a) 
    msg[(*len)++] = 0x1B;
    msg[(*len)++] = 0x69;
    msg[(*len)++] = 0x61;
    msg[(*len)++] = mode;
}

void command_clear(char* msg, int* len)
{
	// 16 進: 1B 40 = クリア (ESC @)
	msg[(*len)++] = 0x1B;
	msg[(*len)++] = 0x40;
}

void command_print(char* msg, int* len)
{
	// 16 進: 0C = 印刷開始 (FF)
	msg[(*len)++] = 0x0C;
}

void command_set_font(char* msg, int* len, int font)
{
	// 16 進: 1B 6B 08 = 書体の設定 (ESC k n)
	msg[(*len)++] = 0x1B;
	msg[(*len)++] = 0x6B;
	msg[(*len)++] = font;
}

void command_set_size(char* msg, int* len, int size)
{
	// ascii: ESC X   00h  43h  00h = 文字サイズの設定
	msg[(*len)++] = 0x1B;
	msg[(*len)++] = 0x58;
	msg[(*len)++] = 0x00;
	msg[(*len)++] = size;
	msg[(*len)++] = 0x00;
}

void command_set_randscape(char* msg, int* len, int landscape)
{
	// ascii: ESC i L 01h = ランドスケープ(横置き)の設定
	msg[(*len)++] = 0x1B;
	msg[(*len)++] = 0x69;
	msg[(*len)++] = 0x4C;
	msg[(*len)++] = landscape; // 0x01 = ランドスケープ (横置き), 0x00 = ポートレート (縦置き)
}

void command_set_page_length(char* msg, int* len, float page_length_inch)
{
    // 2インチまでの範囲か
    if (page_length_inch <= 0 || page_length_inch > 2)
    {
        printf("Error: Page length must be between 1 and 2 inches.\n");
        return;
    }

    // インチをドットに変換 (1インチ = 300ドット)
    int page_length = (int)(page_length_inch * 300);

    // 余白の72ドットを引く
    page_length -= 72;

    // ページ長が有効な範囲内にあるか確認
    if (page_length <= 0 || page_length >= 12000)
    {
        printf("Error: Page length must be between 1/300 and 11999/300 inches after subtracting the margin.\n");
        return;
    }

    // ページ長をmLとmHに分割
    int mL = page_length & 0xFF;
    int mH = (page_length >> 8) & 0xFF;

    // ascii: ESC ( C nL nH mL mH = ページの長さの設定
    msg[(*len)++] = 0x1B;
    msg[(*len)++] = 0x28;
    msg[(*len)++] = 0x43;
    msg[(*len)++] = 0x02; // nL
    msg[(*len)++] = 0x00; // nH
    msg[(*len)++] = mL;
    msg[(*len)++] = mH;
}

void command_set_position(char* msg, int* len, float x_inch, float y_inch)
{
	// 1インチ = 300ドット
	int dot_per_inch = 300;
	
	// 水平位置の設定
	{
		int x_dot = (int)(x_inch * dot_per_inch);

		// n1+n2*256 = x_dot
		int n1 = x_dot & 0xFF;
		int n2 = (x_dot >> 8) & 0xFF;

		// ascii: ESC $  96h  00h = 水平位置の設定
		msg[(*len)++] = 0x1B;
		msg[(*len)++] = 0x24;
		msg[(*len)++] = n1;
		msg[(*len)++] = n2;
	}

	// 垂直位置の設定
	{
		int y_dot = (int)(y_inch * dot_per_inch);

		// n1+n2*256 = y_dot
		int n1 = y_dot & 0xFF;
		int n2 = (y_dot >> 8) & 0xFF;

		// ascii: ESC ( V 02h  00h  1Ah  01h = 垂直位置の設定
		msg[(*len)++] = 0x1B;
		msg[(*len)++] = 0x28;
		msg[(*len)++] = 0x56;
		msg[(*len)++] = 0x02;
		msg[(*len)++] = 0x00;
		msg[(*len)++] = n1;
		msg[(*len)++] = n2;
	}
}

void set_ascii_text(char* msg, int* len, const char* text)
{
	int text_len = strlen(text);
	for (int i = 0; i < text_len; i++)
	{
		msg[(*len)++] = text[i];
	}
}

void set_kanji_mode(char* msg, int* len, int mode)
{
	if(mode==1){ // 1 = 漢字モード
		msg[(*len)++] = 0x1C;
		msg[(*len)++] = 0x26;
	}else{ // 0 = ASCII モード
		msg[(*len)++] = 0x1C;
		msg[(*len)++] = 0x2E;
	}
}

void command_set_qr_code(char* msg, int* len, const char* qr_code, int qr_code_len)
{
	// QRコードの設定
	msg[(*len)++] = 0x1B;
	msg[(*len)++] = 0x69;
	msg[(*len)++] = 0x51;
	
	msg[(*len)++] = 0x04; //1セルのドット数 default 0x04
	msg[(*len)++] = 0x02; // qrmodel 1 = 1, 2 = 2, 3 = micro qr

	// 分割 QRコードの場合は0
	msg[(*len)++] = 0x00;
	msg[(*len)++] = 0x00;
	msg[(*len)++] = 0x00;

	// パリティデータ(全ての印字データをバイト単位でEX-ORを取った値)
	int parity = 0;
	for (int i = 0; i < qr_code_len; i++)
	{
		parity ^= qr_code[i];
	}
	msg[(*len)++] = parity;

	// 誤り訂正レベル
	msg[(*len)++] = 0x03; // 0x01 = l, 0x02 = m, 0x03 = q, 0x04 = h

	// データ入力方法
	msg[(*len)++] = 0x00; // 0x00 = 自動

	// QRコードデータ
	for (int i = 0; i < qr_code_len; i++)
	{
		msg[(*len)++] = qr_code[i];
	}

	// ascii ¥ を3つ追加
	msg[(*len)++] = 0x5C;
	msg[(*len)++] = 0x5C;
	msg[(*len)++] = 0x5C;
}

int on_connect(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_connect\n");
	QS_SOCKET_OPTION* tcp_client = (QS_SOCKET_OPTION*)connection->qs_socket_option;

	char msg[1024];
	int len = 0;
	memset(msg,0,1024);

	// コマンドモード切替
	command_set_mode(msg, &len, 0x0); // 0x0 = ESC/P モード

	// クリア
	command_clear(msg, &len);

	// ランドスケープ方向の設定
	command_set_randscape(msg, &len, 0x01); // 0x01 = ランドスケープ (横置き)

	// ページの長さの設定
	command_set_page_length(msg, &len, 1.0); // 1.0インチ

	// 書体の設定
	command_set_font(msg, &len, 0x08); // 0x08 = ゴシック

	// 文字サイズの設定
	command_set_size(msg, &len, 0x26); // 0x26 = フォントサイズ 38

	set_kanji_mode(msg, &len, 0);
	command_set_position(msg, &len, 0.0, 0.0);

    char kyabetsu[] = { 0xB7, 0xAC, 0xCD, 0xDE, 0xC2, '\0' };
	set_ascii_text(msg, &len, kyabetsu);

	command_set_position(msg, &len, 0.0, 0.2);
	set_ascii_text(msg, &len, "100");

	// 円
	char enn[] = { 0xF1 ,'\0' };
	set_ascii_text(msg, &len, enn);

	command_set_position(msg, &len, 0.0, 0.4);
	command_set_qr_code(msg, &len, "https://www.youtube.com", strlen("https://www.youtube.com"));

	// 0x0c = FF = 印刷開始
	command_print(msg, &len);

	printf("msg len:%d\n",len);

	qs_send(tcp_client, &connection->sockparam, msg, len, 0);

	return 0;
}

void* on_pkt_recv( void* args )
{
	QS_RECV_INFO *rinfo = (QS_RECV_INFO*)args;
	QS_SERVER_CONNECTION_INFO *connection = (QS_SERVER_CONNECTION_INFO*)rinfo->tinfo;
	printf("on_pkt_recv\n");
	return ( (void *) NULL );
}

int32_t on_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info )
{
	printf("on_recv(payload_type:%d,payload_len:%d)%s\n",payload_type,(int)payload_len,(char*)payload);
	return 0;
}

int on_close(QS_SERVER_CONNECTION_INFO* connection)
{
	printf("on_close\n");
	return 0;
}

int32_t on_udp_client_payload_recv(uint32_t payload_type, uint8_t* payload, size_t payload_len, QS_RECV_INFO *qs_recv_info)
{
	char *pbuf = (char*)payload;
	pbuf[payload_len] = '\0';
	printf("recv udp(payload_type:%d,payload_len:%d) %s\n", (int)payload_type, (int)payload_len, pbuf);
	return 0;
}
