#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/file.h"
#include "lib/print.h"
#include "serialise.h"
#include "vm.h"

char* l_read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        fclose(file);
        return NULL;
    }
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

bool l_file_exists(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    fclose(file);
    return true;
}

int _deserialise_bytecode(vm_config_t *config, const char* path, const char* source) {

    l_init_memory();

    // Check for existing bytecode
    serialiser_t * serialiser = l_serialise_new(path, source, SERIALISE_MODE_READ);

    if ( serialiser->error != SERIALISE_OK) {

        switch ( serialiser->error ) {
            case SERIALISE_ERROR_SOURCE_HASH_MISMATCH:
                printf("source change detected. recompiling");
                break;
            case SERIALISE_ERROR_SOX_VERSION_MISMATCH:
                printf("Bytecode version for an old Sox version. recompiling");
                break;
            default:
                break;
        }
        return 1;
    }

    // deserialise the bytecode file

    // start tracking allocations
    l_allocate_track_init();

    // initialise the vm
    l_init_vm(config);

    // deserialise the objects
    obj_closure_t * entry_point = l_deserialise_vm(serialiser);

    // link the objects
    l_allocate_track_link_targets();

    printf("linking complete\n");

    // free tracking allocations
    l_allocate_track_free();

    // set the entry point for the VM
    // this must be done after the link step
    l_set_entry_point(entry_point);

    // free the reader serialiser
    l_serialise_del(serialiser);
    
    // run the vm
    InterpretResult result = l_run();

    //clean up
    l_free_vm();

    l_free_memory();

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;

    return 0;
}

int _interpret_serialise_bytecode(vm_config_t *config, const char* path, const char* source) {
    // setup to serialise the interpreted bytecode

    l_init_memory();

    // setup the writer serialiser
    serialiser_t * serialiser = l_serialise_new(path, source, SERIALISE_MODE_WRITE);

    l_init_vm(config);

    l_serialise_vm_set_init_state(serialiser);

    // interpret the source
    InterpretResult result = l_interpret(source);
    

    if (result == INTERPRET_COMPILE_ERROR) 
        return 65;

    // serialise the vm state
    l_serialise_vm(serialiser);

    // flush to the file and free the serialiser
    l_serialise_flush(serialiser);
    l_serialise_finalise(serialiser);
    
    l_serialise_del(serialiser);

    // run the vm
    result = l_run();

    //clean up
    l_free_vm();

    l_free_memory();
    
    if (result == INTERPRET_COMPILE_ERROR)
        return 65;

    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;

    return 0;
}

int _interpret_run(vm_config_t *config, const char* source) {
    
    InterpretResult result;
    
    l_init_memory();

    // start the VM and interpret the source
    l_init_vm(config);

    // interpret the source
    result = l_interpret(source);

    if (result == INTERPRET_COMPILE_ERROR) {
        l_free_vm();
        l_free_memory();
        return 65;
    }
    
    // run the vm
    result = l_run();

    //clean up
    l_free_vm();

    l_free_memory();

    if (result == INTERPRET_COMPILE_ERROR)
        return 65;
    if (result == INTERPRET_RUNTIME_ERROR)
        return 70;
    
    return 0;
}

int l_run_file(vm_config_t *config) {
    if (config->args.argc < 2) {
        fprintf(stderr, "Usage: sox [path] [optional: --serialise]\n");
        return 64;
    }
    const char* path = config->args.argv[1];

    if (path == NULL) {
        fprintf(stderr, "Usage: sox [path] [optional: --serialise]\n");
        return 64;
    }

    char* source = l_read_file(path);

    if (source == NULL) {
        printf("Could not read file: %s\n", path);
        return 74;
    }

    if ( source == NULL )
        return 74;

    if (config->suppress_print) {
        l_print_enable_suppress();
    }
    
    InterpretResult result;

    if (config->enable_serialisation == false) {
        result = _interpret_run(config, source);

    } else {
        char filename_bytecode[256];
        sprintf(&filename_bytecode[0], "%s.sbc", path);
        
        if (l_file_exists(&filename_bytecode[0])) {
            result = _deserialise_bytecode(config, path, source);
        } else {
            result = _interpret_serialise_bytecode(config, path, source);
        }
        
    }
    
    // clean up the source code
    free(source);
    
    return result;
}

bool l_file_delete(const char * path) {
    if (l_file_exists(path) == false)
        return false;

    return remove(path) == 0;
}
