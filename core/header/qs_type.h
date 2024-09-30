/*
 * Copyright (c) 2014-2024 Katsuya Owari
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

#ifdef __cplusplus
extern "C"{
#endif

#ifndef _QS_TYPE_H_
#define _QS_TYPE_H_

// base elements
#define ELEMENT_NONE				0
#define ELEMENT_ROOT				1
#define ELEMENT_CHILD				2
#define ELEMENT_CHILD_ARRAY			3
#define ELEMENT_CHILD_HASH			4

// additional elements
#define ELEMENT_LITERAL_NUM			5
#define ELEMENT_LITERAL_FLOAT		6
#define ELEMENT_LITERAL_STR			7
#define ELEMENT_CMP					8
#define ELEMENT_FUNCTION			9
#define ELEMENT_VALIABLE			10
#define ELEMENT_IF					11
#define ELEMENT_ELSEIF				12
#define ELEMENT_ELSE				13
#define ELEMENT_ASSIGNMENT			14
#define ELEMENT_OP					15
#define ELEMENT_LOOP				16
#define ELEMENT_WHILE				17
#define ELEMENT_OP_LITERAL_MINUS	18
#define ELEMENT_RETURN				19
#define ELEMENT_ARRAY				20
#define ELEMENT_ARRAY_REFERENCE		21
#define ELEMENT_HASH				22
#define ELEMENT_HASH_OP				23
#define ELEMENT_LITERAL_BIN			24
#define ELEMENT_QUEUE				25
#define ELEMENT_INCREMENT           26
#define ELEMENT_INCREMENT_AFTER     27
#define ELEMENT_DECREMENT           28
#define ELEMENT_DECREMENT_AFTER     29
#define ELEMENT_LITERAL_NUM_8		100
#define ELEMENT_LITERAL_NUM_U8		101
#define ELEMENT_LITERAL_NUM_16		102
#define ELEMENT_LITERAL_NUM_U16		103
#define ELEMENT_LITERAL_NUM_32		104
#define ELEMENT_LITERAL_NUM_U32		105
#define ELEMENT_LITERAL_NUM_64		106
#define ELEMENT_LITERAL_NUM_U64		107
#define ELEMENT_NULL				1000

#endif /*_QS_TYPE_H_*/

#ifdef __cplusplus
}
#endif
