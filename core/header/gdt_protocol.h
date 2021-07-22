#ifdef __cplusplus
extern "C"{
#endif

#ifndef _GDT_PROTOCOL_H_
#define _GDT_PROTOCOL_H_

#include "gdt_core.h"
#include "gdt_memory_allocator.h"

ssize_t gdt_get_protocol_buffer_size(ssize_t payload_size);
uint8_t gdt_get_protocol_header_size_byte(ssize_t payload_size);
uint32_t gdt_get_protocol_header_size(ssize_t payload_size);
uint32_t gdt_make_protocol_header(uint8_t* bin,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);
ssize_t gdt_make_protocol_buffer(uint8_t* bin, uint8_t* payload,ssize_t payload_size, uint32_t payload_type, uint32_t seq_num);

#endif /*_GDT_PROTOCOL_H_*/

#ifdef __cplusplus
}
#endif
