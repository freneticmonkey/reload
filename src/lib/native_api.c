
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lib/native_api.h"

#include "lib/table.h"
#include "object.h"
#include "vm.h"


static obj_error_t* _native_error(const char * message) {
    l_vm_runtime_error(message);
    obj_string_t* msg = l_copy_string(message, strlen(message));
    return l_new_error(msg, NULL);
}

static value_t _new_table(int argCount, value_t* args) {
    obj_table_t* table = l_new_table();
    return OBJ_VAL(table);
}

static value_t _len(int argCount, value_t* args) {
    if (argCount > 1 || argCount == 0) {
        return OBJ_VAL(_native_error("len(): invalid parameter count"));
    }
    switch (args[0].type) {
        case VAL_OBJ: {
            obj_t* obj = AS_OBJ(args[0]);
            switch (obj->type) {
                case OBJ_TABLE: {
                    obj_table_t* table = AS_TABLE(args[0]);
                    return NUMBER_VAL(table->table.count);
                }
                case OBJ_ARRAY: {
                    obj_array_t* array = AS_ARRAY(args[0]);
                    return NUMBER_VAL(array->values.count);
                }
                case OBJ_BOUND_METHOD:
                case OBJ_CLASS:
                case OBJ_CLOSURE:
                case OBJ_FUNCTION:
                case OBJ_INSTANCE:
                case OBJ_NATIVE:
                case OBJ_STRING:
                case OBJ_UPVALUE:
                case OBJ_ERROR:
                    break;
            }
        }
        case VAL_BOOL:
        case VAL_NIL:
        case VAL_NUMBER:
            break;
    }
    return OBJ_VAL(_native_error("len(): invalid parameter type"));
}

static value_t _push(int argCount, value_t* args) {
    if (argCount != 2) {
        return OBJ_VAL(_native_error("push(): invalid parameter count"));
    }
    if (args[0].type != VAL_OBJ) {
        return OBJ_VAL(_native_error("push(): invalid parameter type"));
    }
    obj_t* obj = AS_OBJ(args[0]);
    if (obj->type != OBJ_ARRAY) {
        return OBJ_VAL(_native_error("push(): invalid parameter type"));
    }
    obj_array_t* array = AS_ARRAY(args[0]);
    l_push_array(array, args[1]);
    return args[0];
}

static value_t _pop(int argCount, value_t* args) {
    if (argCount != 1) {
        return OBJ_VAL(_native_error("pop(): invalid parameter count"));
    }
    if (args[0].type != VAL_OBJ) {
        return OBJ_VAL(_native_error("pop(): invalid parameter type"));
    }
    obj_t* obj = AS_OBJ(args[0]);
    if (obj->type != OBJ_ARRAY) {
        return OBJ_VAL(_native_error("pop(): invalid parameter type"));
    }
    obj_array_t* array = AS_ARRAY(args[0]);
    if (array->values.count == 0) {
        return OBJ_VAL(_native_error("pop(): array is empty"));
    }
    return l_pop_array(array);
}

static value_t _clock_native(int argCount, value_t* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

static value_t _usleep_native(int argCount, value_t* args) {
    if ( argCount == 1 && IS_NUMBER(args[0]) ) {
        return NUMBER_VAL(usleep((unsigned int)AS_NUMBER(args[0])));
    }
    return NUMBER_VAL(-1);
}

static value_t _type(int argCount, value_t* args) {
    if ( argCount == 1 ) {
        const char* type = NULL;
        switch (args[0].type) {
            case VAL_BOOL:
                type = "<bool>";
                break;
            case VAL_NIL:
                type = "<nil>";
                break;
            case VAL_NUMBER:
                type = "<number>";
                break;
            case VAL_OBJ: {
                obj_t* obj = AS_OBJ(args[0]);
                type = obj_type_to_string[obj->type];
                break;
            }
        }
        return OBJ_VAL(l_copy_string(type, strlen(type)));
    }
    const char* message = "type(): invalid argument(s)";
    l_vm_runtime_error(message);
    obj_string_t* msg = l_copy_string(message, strlen(message));
    return OBJ_VAL(l_new_error(msg, NULL));
}

void l_table_add_native() {
    l_vm_define_native("Table", _new_table);

    l_vm_define_native("len", _len);
    // l_vm_define_native("set", _set);
    // l_vm_define_native("get", _get);


    // TODO: Table functions
    // len()
    // add()
    // get()
    // set()
    // range()

    // array functions
    l_vm_define_native("push", _push);
    l_vm_define_native("pop", _pop);

    l_vm_define_native("type", _type);

    // native functions
    l_vm_define_native("clock", _clock_native);
    l_vm_define_native("usleep", _usleep_native);
}