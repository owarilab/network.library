#include "gdt_protocol.h"

uint8_t gdt_get_protocol_header_size_byte(ssize_t payload_size)
{
	uint8_t size_byte = 0;
	if( payload_size <= 125 ){
		size_byte = (uint8_t)payload_size;
	}
	else if( payload_size >= 126 && payload_size <= 65536 ){
		size_byte = 126;
	}
	else{
		size_byte = 127;
	}
	return size_byte;
}

uint32_t gdt_get_protocol_header_size(ssize_t payload_size)
{
	uint32_t headersize = 2;
	uint8_t header_size_byte = gdt_get_protocol_header_size_byte(payload_size);
	if( header_size_byte == 126 ){
		headersize += 2;
	}
	else if( header_size_byte == 127 ){
		headersize += 8;
	}
	headersize += 4; // payload_type
	headersize += 4; // sequence num
	return headersize;
}

ssize_t gdt_get_protocol_buffer_size(ssize_t payload_size)
{
	return gdt_get_protocol_header_size(payload_size) + payload_size;	
}

uint32_t gdt_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num)
{
	uint8_t* pbin = bin;
	int endian = gdt_endian();
	uint32_t headersize = 2;
	// mode
	*(pbin++) = 0;
	uint8_t header_size_byte = gdt_get_protocol_header_size_byte(payload_size);
	*(pbin++) = header_size_byte;
	// payload size
	if( header_size_byte == 126 ){
		printf("16bit\n");
		headersize += 2;
		MEMORY_PUSH_BIT16_B2(endian,pbin,payload_size);
	}
	else if( header_size_byte == 127 ){
		printf("64bit\n");
		headersize += 8;
		MEMORY_PUSH_BIT64_B2(endian,pbin,payload_size);
	}
	// payload type
	headersize += 4;
	MEMORY_PUSH_BIT32_B2( endian, pbin, payload_type );
	// sequence num
	headersize += 4;
	MEMORY_PUSH_BIT32_B2( endian, pbin, seq_num );
	return headersize;
}

ssize_t gdt_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num)
{
	uint8_t* pbin = bin;
	uint32_t headersize = gdt_make_protocol_header(bin,payload_size,payload_type,seq_num);
	pbin += headersize;
	memcpy(pbin,payload,payload_size);
	return headersize + payload_size;
}
