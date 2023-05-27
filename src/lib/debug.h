#ifndef LIB_DEBUG_H
#define LIB_DEBUG_H

#include "chunk.h"

void l_dissassemble_chunk(chunk_t *chunk, const char *name);
int  l_disassemble_instruction(chunk_t *chunk, int offset);

#endif