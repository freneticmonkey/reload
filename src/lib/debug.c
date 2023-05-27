#include <stdio.h>

#include "lib/debug.h"
#include "object.h"
#include "value.h"

void l_dissassemble_chunk(chunk_t *chunk, const char *name) {
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) {
        offset = l_disassemble_instruction(chunk, offset);
    }
}

static int _constant_instruction(const char* name, chunk_t* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    l_print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int _invoke_instruction(const char* name, chunk_t* chunk, int offset) {
    uint8_t constant = chunk->code[offset + 1];
    uint8_t argCount = chunk->code[offset + 2];
    printf("%-16s (%d args) %4d '", name, argCount, constant);
    l_print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

static int _simple_instruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int _byte_instruction(const char* name, chunk_t* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2; 
}

static int _jump_instruction(const char* name, int sign, chunk_t* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, (offset + 3 + sign * jump) );
    return offset + 3;
}

int l_disassemble_instruction(chunk_t* chunk, int offset) {
    printf("%04d ", offset);

    if (offset > 0 &&
        chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        
        case OP_CONSTANT:
            return _constant_instruction("OP_CONSTANT", chunk, offset);
        case OP_NIL:
            return _simple_instruction("OP_NIL", offset);
        case OP_TRUE:
            return _simple_instruction("OP_TRUE", offset);
        case OP_FALSE:
            return _simple_instruction("OP_FALSE", offset);
        case OP_POP:
            return _simple_instruction("OP_POP", offset);
        case OP_GET_LOCAL:
            return _byte_instruction("OP_GET_LOCAL", chunk, offset);
        case OP_SET_LOCAL:
            return _byte_instruction("OP_SET_LOCAL", chunk, offset);
        case OP_GET_GLOBAL:
            return _constant_instruction("OP_GET_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return _constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL:
            return _constant_instruction("OP_SET_GLOBAL", chunk, offset);
        case OP_GET_UPVALUE:
            return _byte_instruction("OP_GET_UPVALUE", chunk, offset);
        case OP_SET_UPVALUE:
            return _byte_instruction("OP_SET_UPVALUE", chunk, offset);
        case OP_GET_PROPERTY:
            return _constant_instruction("OP_GET_PROPERTY", chunk, offset);
        case OP_SET_PROPERTY:
            return _constant_instruction("OP_SET_PROPERTY", chunk, offset);
        case OP_GET_SUPER:
            return _constant_instruction("OP_GET_SUPER", chunk, offset);
        case OP_GET_INDEX:
            return _byte_instruction("OP_GET_INDEX", chunk, offset); // FIXME: Is this what I need?
        case OP_SET_INDEX:
            return _byte_instruction("OP_SET_INDEX", chunk, offset); // FIXME: Is this what I need?v
        case OP_EQUAL:
            return _simple_instruction("OP_EQUAL", offset);
        case OP_GREATER:
            return _simple_instruction("OP_GREATER", offset);
        case OP_LESS:
            return _simple_instruction("OP_LESS", offset);
        case OP_ADD:
            return _simple_instruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return _simple_instruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return _simple_instruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return _simple_instruction("OP_DIVIDE", offset);
        case OP_NOT:
            return _simple_instruction("OP_NOT", offset);
        case OP_NEGATE:
            return _simple_instruction("OP_NEGATE", offset);
        case OP_PRINT:
            return _simple_instruction("OP_PRINT", offset);
        case OP_JUMP:
            return _jump_instruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return _jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return _jump_instruction("OP_LOOP", -1, chunk, offset);
        case OP_CALL:
            return _byte_instruction("OP_CALL", chunk, offset);
        case OP_INVOKE:
            return _invoke_instruction("OP_INVOKE", chunk, offset);
        case OP_SUPER_INVOKE:
            return _invoke_instruction("OP_SUPER_INVOKE", chunk, offset);
        case OP_CLOSURE: {
            offset++;
            uint8_t constant = chunk->code[offset++];
            printf("%-16s %4d ", "OP_CLOSURE", constant);
            l_print_value(chunk->constants.values[constant]);
            printf("\n");

            obj_function_t* function = AS_FUNCTION(chunk->constants.values[constant]);
            for (int j = 0; j < function->upvalue_count; j++) {
                int isLocal = chunk->code[offset++];
                int index = chunk->code[offset++];
                printf("%04d      |                     %s %d\n",
                    offset - 2, isLocal ? "local" : "upvalue", index);
            }
        
            return offset;
        }
        case OP_CLOSE_UPVALUE:
            return _simple_instruction("OP_CLOSE_UPVALUE", offset);
        case OP_RETURN:
            return _simple_instruction("OP_RETURN", offset);
        case OP_CLASS:
            return _constant_instruction("OP_CLASS", chunk, offset);
        case OP_INHERIT:
            return _simple_instruction("OP_INHERIT", offset);
        case OP_METHOD:
            return _constant_instruction("OP_METHOD", chunk, offset);
        case OP_ARRAY_EMPTY:
            return _byte_instruction("OP_ARRAY_EMPTY", chunk, offset);
        case OP_ARRAY_PUSH:
            return _byte_instruction("OP_ARRAY_PUSH", chunk, offset);
        case OP_ARRAY_RANGE:
            return _byte_instruction("OP_ARRAY_RANGE", chunk, offset);

        
        // ITERATORS
        case OP_TEST_ITERATOR:
            return _simple_instruction("OP_TEST_ITERATOR", offset);
        case OP_GET_ITERATOR:
            return _simple_instruction("OP_GET_ITERATOR", offset);
        case OP_NEXT_ITERATOR:
            return _simple_instruction("OP_NEXT_ITERATOR", offset);

        // NO_OP codes that the VM shouldn't ever see
        case OP_BREAK:
            printf("OP_BREAK: Compiler Error. The VM should never see this\n");
            return offset + 3;
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}